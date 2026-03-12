#include "macs_calculator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

namespace {

constexpr double KEV_TO_MEV        = 1e-3;   // 1 keV = 1e-3 MeV
constexpr double BARN_TO_MILLIBARN = 1000.0; // 1 barn = 1000 mb

double thermal_energy(double temperature_kev)
{
  return temperature_kev * KEV_TO_MEV;
}

double macs_norm(double rmu, double kth)
{
  return (2.0 * rmu * rmu) / (std::sqrt(std::numbers::pi) * kth * kth);
}

double trapezoid_segment(double rmu, double kth, double en1, double cs1, double en2, double cs2)
{
  double const fv1 = cs1 * en1 * std::exp(-(rmu * en1) / kth);
  double const fv2 = cs2 * en2 * std::exp(-(rmu * en2) / kth);
  return 0.5 * (fv1 + fv2) * (en2 - en1);
}

std::optional<std::string> validate_spans(std::size_t energies_size,
                                          std::size_t cross_sections_size)
{
  if (energies_size != cross_sections_size) {
    return "energy and cross-section spans must have equal length";
  }
  if (energies_size == 0) {
    return "input spans cannot be empty";
  }
  return std::nullopt;
}

struct ThermalParams
{
  std::vector<double> kths;
  std::vector<double> norms;
};

ThermalParams precompute_thermal(std::span<double const> temperatures_kev, double rmu)
{
  std::vector<double> kths(temperatures_kev.size());
  std::transform(temperatures_kev.begin(), temperatures_kev.end(), kths.begin(), thermal_energy);

  std::vector<double> norms(kths.size());
  std::transform(kths.begin(), kths.end(), norms.begin(),
                 [rmu](double kth) { return macs_norm(rmu, kth); });

  return {std::move(kths), std::move(norms)};
}

} // namespace

namespace macs {

std::expected<double, std::string> calculate_macs(std::span<double const> energies_mev,
                                                  std::span<double const> cross_sections_barn,
                                                  double atomic_mass, double temperature_kev)
{
  if (auto err = validate_spans(energies_mev.size(), cross_sections_barn.size())) {
    return std::unexpected(*err);
  }
  if (temperature_kev <= 0.0) {
    return std::unexpected("temperature must be positive");
  }

  double const kth = thermal_energy(temperature_kev);
  double const rmu = atomic_mass / (1.0 + atomic_mass);

  double integral = 0.0;
  for (std::size_t idx = 1; idx < energies_mev.size(); ++idx) {
    integral += trapezoid_segment(rmu, kth, energies_mev[idx - 1], cross_sections_barn[idx - 1],
                                  energies_mev[idx], cross_sections_barn[idx]);
  }

  return macs_norm(rmu, kth) * integral * BARN_TO_MILLIBARN;
}

std::expected<CumulativeMacsData, std::string>
calculate_cumulative_macs(std::span<double const> energies_mev,
                          std::span<double const> cross_sections_barn, double atomic_mass,
                          std::span<double const> temperatures_kev)
{
  if (auto err = validate_spans(energies_mev.size(), cross_sections_barn.size())) {
    return std::unexpected(*err);
  }

  double const rmu          = atomic_mass / (1.0 + atomic_mass);
  auto const thermal        = precompute_thermal(temperatures_kev, rmu);
  std::size_t const n_pts   = energies_mev.size();
  std::size_t const n_temps = temperatures_kev.size();

  std::vector<double> running(n_temps, 0.0);
  CumulativeMacsData rows;
  rows.reserve(n_pts);
  rows.emplace_back(energies_mev[0], std::vector<double>(n_temps, 0.0));

  for (std::size_t idx = 1; idx < n_pts; ++idx) {
    for (std::size_t tid = 0; tid < n_temps; ++tid) {
      running[tid] += trapezoid_segment(rmu, thermal.kths[tid], energies_mev[idx - 1],
                                        cross_sections_barn[idx - 1], energies_mev[idx],
                                        cross_sections_barn[idx]);
    }

    std::vector<double> cum(n_temps);
    std::transform(thermal.norms.begin(), thermal.norms.end(), running.begin(), cum.begin(),
                   [](double norm, double acc) { return norm * acc * BARN_TO_MILLIBARN; });

    rows.emplace_back(energies_mev[idx], std::move(cum));
  }

  return rows;
}

} // namespace macs
