#include "exfor_client.hpp"

#include <lyra/lyra.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <expected>
#include <format>
#include <fstream>
#include <string>
#include <vector>

namespace {

auto parse_config(std::string const& path)
    -> std::expected<std::vector<macs::FetchRequest>, std::string>
{
  std::ifstream file{path};
  if (!file) {
    return std::unexpected(std::format("cannot open '{}'", path));
  }

  try {
    std::vector<macs::FetchRequest> requests;
    for (auto const& entry : nlohmann::json::parse(file)) {
      requests.push_back({
          .target   = entry.at("target").get<std::string>(),
          .reaction = entry.at("reaction").get<std::string>(),
          .lib_name = entry.at("lib_name").get<std::string>(),
      });
    }
    return requests;
  } catch (nlohmann::json::exception const& e) {
    return std::unexpected(std::format("invalid config: {}", e.what()));
  }
}

auto sanitize(std::string_view str) -> std::string
{
  std::string out{str};
  for (auto& chr : out) {
    if (chr == '/' || chr == ' ') {
      chr = '-';
    }
  }
  return out;
}

auto write_csv(std::string const& output_dir, macs::FetchRequest const& req,
               macs::CrossSection const& xsec) -> std::expected<void, std::string>
{
  auto const filename = std::format("{}/{}_{}_{}.csv", output_dir, sanitize(req.target),
                                    sanitize(req.reaction), sanitize(req.lib_name));

  std::ofstream file{filename};
  if (!file) {
    return std::unexpected(std::format("cannot write '{}'", filename));
  }

  file << "energy_eV,cross_section_barn,d_sig_barn\n";
  for (auto const& [e, sig, dsig] : xsec) {
    file << std::format("{:.6e},{:.6e},{:.6e}\n", e, sig, dsig);
  }

  return {};
}

} // namespace

int main(int argc, char** argv)
{
  spdlog::set_level(spdlog::level::debug);

  std::string config_path;
  std::string output_dir{"."};

  auto cli = lyra::cli()
           | lyra::arg(config_path, "config").required()("JSON config file with fetch requests")
           | lyra::opt(output_dir, "dir")["-o"]["--output-dir"]("output directory for CSV files");

  if (auto res = cli.parse({argc, argv}); !res) {
    spdlog::error("{}", res.message());
    return EXIT_FAILURE;
  }

  auto requests = parse_config(config_path);
  if (!requests) {
    spdlog::error("{}", requests.error());
    return EXIT_FAILURE;
  }

  bool any_error = false;
  for (auto const& [req, data] : macs::fetch_cross_sections(*requests)) {
    auto const code =
        data.transform([&](macs::CrossSection const& xsec) {
              spdlog::info("[{}] {} points", req.lib_name, xsec.size());
              if (auto res = write_csv(output_dir, req, xsec); !res) {
                spdlog::error("{}", res.error());
              }
              return EXIT_SUCCESS;
            })
            .or_else([&](macs::ExforParseError err) -> std::expected<int, macs::ExforParseError> {
              spdlog::error("[{}] fetch failed: {}", req.lib_name, macs::to_string(err));
              return EXIT_FAILURE;
            })
            .value_or(EXIT_FAILURE);
    if (code == EXIT_FAILURE) {
      any_error = true;
    }
  }

  return any_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
