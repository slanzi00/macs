#include "macs_calculator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr double K_B               = 8.617e-11; // Boltzmann constant [MeV/K]
constexpr double KEV_TO_MEV        = 1e-3;      // 1 keV = 1e-3 MeV
constexpr double BARN_TO_MILLIBARN = 1000.0;    // 1 barn = 1000 mb
constexpr double TRAPEZOID_WEIGHT  = 0.5;       // trapezoidal rule coefficient
constexpr double NORM_PREFACTOR    = 2.0;       // prefactor in MACS normalization

double trapezoid_area(auto fun, double x_1, double x_2, double y_1, double y_2)
{
  return TRAPEZOID_WEIGHT * (fun(x_1, y_1) + fun(x_2, y_2)) * (x_2 - x_1);
}

} // namespace

namespace macs {

std::expected<Millibarn, std::string> calculate_macs(std::span<MeV const> energies,
                                                     std::span<Barn const> cross_sections,
                                                     double atomic_mass, KeV temperature)
{
  if (energies.size() != cross_sections.size()) {
    return std::unexpected("energy and cross-section spans must have equal length");
  }
  if (energies.empty()) {
    return std::unexpected("input spans cannot be empty");
  }
  if (temperature.value <= 0.0) {
    return std::unexpected("temperature must be positive");
  }

  // T[keV] → T[K] → kT[MeV]
  double temp_k = (temperature.value * KEV_TO_MEV) / K_B; // [K]
  double kth    = K_B * temp_k;                           // thermal energy [MeV]
  double rmu    = atomic_mass / (1.0 + atomic_mass);      // reduced mass factor

  // Integrand: σ[barn] · E[MeV] · exp(−rmu·E / kT)
  auto integrand = [rmu, kth](double e_mev, double cs_barn) {
    return cs_barn * e_mev * std::exp(-(rmu * e_mev) / kth);
  };

  // Sum trapezoid areas over all adjacent (E, σ) pairs  [barn·MeV]
  double integral = 0.0;
  for (std::size_t idx = 1; idx < energies.size(); ++idx) {
    integral += trapezoid_area(integrand, energies[idx - 1].value, energies[idx].value,
                               cross_sections[idx - 1].value, cross_sections[idx].value);
  }

  // Normalization: 2·rmu² / (√π · kth²)  [MeV⁻²]
  double norm = (NORM_PREFACTOR * rmu * rmu) / (std::sqrt(std::numbers::pi) * kth * kth);

  // norm [MeV⁻²] · integral [barn·MeV] → [barn] → [mb]
  return Millibarn{norm * integral * BARN_TO_MILLIBARN};
}

std::expected<std::vector<std::pair<MeV, std::vector<Millibarn>>>, std::string>
calculate_cumulative_macs(std::span<MeV const> energies, std::span<Barn const> cross_sections,
                          double atomic_mass, std::span<KeV const> temperatures)
{
  if (energies.size() != cross_sections.size()) {
    return std::unexpected("energy and cross-section spans must have equal length");
  }
  if (energies.empty()) {
    return std::unexpected("input spans cannot be empty");
  }

  double rmu = atomic_mass / (1.0 + atomic_mass); // reduced mass factor

  // Precompute thermal energies kT[MeV] and normalization factors for each temperature
  std::vector<double> kths(temperatures.size());
  std::transform(temperatures.begin(), temperatures.end(), kths.begin(), [](KeV tmp) {
    double t_k = (tmp.value * KEV_TO_MEV) / K_B; // keV → K
    return K_B * t_k;                            // K → kT [MeV]
  });

  std::vector<double> norms(temperatures.size());
  std::transform(kths.begin(), kths.end(), norms.begin(), [rmu](double kth) {
    return (NORM_PREFACTOR * rmu * rmu) / (std::sqrt(std::numbers::pi) * kth * kth);
  });

  std::size_t n_pts   = energies.size();
  std::size_t n_temps = temperatures.size();

  // running[t] accumulates the trapezoidal sum for temperature t  [barn·MeV]
  std::vector<double> running(n_temps, 0.0);
  std::vector<std::pair<MeV, std::vector<Millibarn>>> rows;
  rows.reserve(n_pts);

  // First point: partial integral from 0 to E[0] is 0
  rows.emplace_back(energies[0], std::vector<Millibarn>(n_temps, Millibarn{0.0}));

  // Sequential: each step accumulates into running[], so no parallelism here
  for (std::size_t idx = 1; idx < n_pts; ++idx) {
    double en1 = energies[idx - 1].value;
    double en2 = energies[idx].value;
    double cs1 = cross_sections[idx - 1].value;
    double cs2 = cross_sections[idx].value;

    for (std::size_t tid = 0; tid < n_temps; ++tid) {
      double kth = kths[tid];
      double fv1 = cs1 * en1 * std::exp(-(rmu * en1) / kth);
      double fv2 = cs2 * en2 * std::exp(-(rmu * en2) / kth);
      running[tid] += TRAPEZOID_WEIGHT * (fv1 + fv2) * (en2 - en1);
    }

    std::vector<Millibarn> cum(n_temps);
    std::transform(
        norms.begin(), norms.end(), running.begin(), cum.begin(),
        [](double norm, double acc) { return Millibarn{norm * acc * BARN_TO_MILLIBARN}; });

    rows.emplace_back(energies[idx], std::move(cum));
  }

  return rows;
}

} // namespace macs
