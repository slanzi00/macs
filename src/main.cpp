#include "exfor_client.hpp"
#include "macs_calculator.hpp"

#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

std::optional<double> atomic_mass_from_nuclide(std::string_view nuclide)
{
  auto pos = nuclide.find('-');
  if (pos == std::string_view::npos)
    return std::nullopt;
  try {
    return std::stod(std::string(nuclide.substr(pos + 1)));
  } catch (...) {
    return std::nullopt;
  }
}

void print_cross_section(std::string_view element, std::string_view library,
                         macs::CrossSectionDataset const& dataset)
{
  std::cout << std::format("# {} {} {}\n", element, library, dataset.reaction);
  std::cout << "# energy[eV]\tcross_section[barn]\td_cross_section[barn]\n";
  for (auto const& pt : dataset.points) {
    std::cout << std::format("{:.6e}\t{:.6e}\t{:.6e}\n", pt.energy, pt.cross_section, pt.d_sig);
  }
}

void print_macs(std::string_view element, std::string_view library, std::string_view reaction,
                std::vector<double> const& temperatures_kev,
                macs::CrossSectionDataset const& dataset, double atomic_mass)
{
  constexpr double EV_TO_MEV = 1e-6;

  std::vector<double> energies(dataset.points.size());
  std::vector<double> cross_sections(dataset.points.size());
  for (std::size_t idx = 0; idx < dataset.points.size(); ++idx) {
    energies[idx]       = dataset.points[idx].energy * EV_TO_MEV;
    cross_sections[idx] = dataset.points[idx].cross_section;
  }

  std::cout << std::format("# {} {} {}\n", element, library, reaction);
  std::cout << "# temperature[keV]\tMACS[mb]\n";
  for (double temp : temperatures_kev) {
    auto res = macs::calculate_macs(energies, cross_sections, atomic_mass, temp);
    if (!res) {
      spdlog::error("calculate_macs failed for T={} keV: {}", temp, res.error());
      continue;
    }
    std::cout << std::format("{:.1f}\t{:.6f}\n", temp, *res);
  }
}

} // namespace

int main(int argc, char** argv)
{
  std::vector<std::string> elements;
  std::vector<std::string> libraries;
  std::vector<double> temperatures;
  std::string reaction = "N,G";
  bool xs_only         = false;
  bool verbose         = false;
  bool show_help       = false;

  auto cli =
      lyra::cli() | lyra::help(show_help)
      | lyra::opt(elements,
                  "nuclide")["-e"]["--element"]("Nuclide to query, e.g. Er-166 (repeatable)")
      | lyra::opt(libraries, "library")["-l"]["--library"](
          "Nuclear data library, e.g. ENDF/B-VIII.1 (repeatable)")
      | lyra::opt(temperatures, "keV")["-t"]["--temperature"](
          "Temperature in keV for MACS computation (repeatable)")
      | lyra::opt(reaction, "reaction")["-r"]["--reaction"]("Reaction type [default: N,G]")
      | lyra::opt(xs_only)["--xs"]("Print cross-section table (energy vs sigma) instead of MACS")
      | lyra::opt(verbose)["-v"]["--verbose"]("Enable debug logging to stderr");

  auto parse_result = cli.parse({argc, argv});

  if (show_help) {
    std::cout << cli << '\n';
    return EXIT_SUCCESS;
  }

  if (!parse_result) {
    std::cerr << std::format("error: {}\n", parse_result.message());
    std::cerr << cli << '\n';
    return EXIT_FAILURE;
  }

  if (elements.empty()) {
    std::cerr << "error: at least one --element/-e required\n\n" << cli << '\n';
    return EXIT_FAILURE;
  }
  if (libraries.empty()) {
    std::cerr << "error: at least one --library/-l required\n\n" << cli << '\n';
    return EXIT_FAILURE;
  }
  if (!xs_only && temperatures.empty()) {
    std::cerr << "error: at least one --temperature/-t required (or use --xs)\n\n" << cli << '\n';
    return EXIT_FAILURE;
  }

  spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::warn);

  int exit_code = EXIT_SUCCESS;

  for (auto const& element : elements) {
    auto mass = atomic_mass_from_nuclide(element);
    if (!mass && !xs_only) {
      spdlog::error("cannot parse atomic mass from '{}' — expected format Symbol-A (e.g. Er-166)",
                    element);
      exit_code = EXIT_FAILURE;
      continue;
    }

    for (auto const& library : libraries) {
      auto data = macs::fetch_cross_section(element, reaction, library);
      if (!data) {
        spdlog::error("{}/{}: fetch failed — {}", element, library, data.error());
        exit_code = EXIT_FAILURE;
        continue;
      }
      if (data->datasets.empty()) {
        spdlog::error("{}/{}: no datasets returned", element, library);
        exit_code = EXIT_FAILURE;
        continue;
      }

      auto const& dataset = data->datasets[0];

      if (xs_only) {
        print_cross_section(element, library, dataset);
      } else {
        print_macs(element, library, reaction, temperatures, dataset, *mass);
      }
    }
  }

  return exit_code;
}
