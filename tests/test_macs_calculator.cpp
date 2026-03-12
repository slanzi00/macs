#include "exfor_client.hpp"
#include "macs_calculator.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace {

using CsGrid = std::pair<std::vector<double>, std::vector<double>>;

// Build a uniform energy grid [e_min, e_max] with n_pts points,
// all with constant cross-section cs_barn.
CsGrid make_constant_cs(double e_min_mev, double e_max_mev, std::size_t n_pts, double cs_barn)
{
  std::vector<double> energies(n_pts);
  std::vector<double> cross_sections(n_pts, cs_barn);
  double const step = (e_max_mev - e_min_mev) / static_cast<double>(n_pts - 1);
  for (std::size_t idx = 0; idx < n_pts; ++idx) {
    energies[idx] = e_min_mev + static_cast<double>(idx) * step;
  }
  return {std::move(energies), std::move(cross_sections)};
}

} // namespace

TEST_SUITE("calculate_macs: error handling")
{
  TEST_CASE("empty spans return error")
  {
    CHECK_FALSE(macs::calculate_macs({}, {}, 166.0, 30.0).has_value());
  }

  TEST_CASE("mismatched spans return error")
  {
    std::vector<double> energies       = {0.001, 0.002};
    std::vector<double> cross_sections = {1.0};
    CHECK_FALSE(macs::calculate_macs(energies, cross_sections, 166.0, 30.0).has_value());
  }

  TEST_CASE("zero temperature returns error")
  {
    std::vector<double> energies       = {0.001, 0.002};
    std::vector<double> cross_sections = {1.0, 1.0};
    CHECK_FALSE(macs::calculate_macs(energies, cross_sections, 166.0, 0.0).has_value());
  }

  TEST_CASE("negative temperature returns error")
  {
    std::vector<double> energies       = {0.001, 0.002};
    std::vector<double> cross_sections = {1.0, 1.0};
    CHECK_FALSE(macs::calculate_macs(energies, cross_sections, 166.0, -1.0).has_value());
  }
}

TEST_SUITE("calculate_macs: analytical")
{
  // For constant σ = C barn over [0, ∞]:
  //   MACS = 2000·C/√π  mb  (independent of temperature and atomic mass)
  //
  // Derivation:
  //   integral = C · (kT/rmu)²
  //   norm     = 2·rmu² / (√π · kT²)
  //   MACS     = norm · integral · 1000 = 2000·C/√π  mb

  TEST_CASE("constant sigma = 1 barn gives 2000/sqrt(pi) mb at all temperatures")
  {
    constexpr double SIGMA_BARN = 1.0;
    double const EXPECTED       = 2000.0 * SIGMA_BARN / std::sqrt(std::numbers::pi);
    constexpr double TOL        = 0.001; // 0.1%
    constexpr std::size_t N_PTS = 100'000;

    auto [energies, cross_sections] = make_constant_cs(1e-6, 2.0, N_PTS, SIGMA_BARN);

    for (double temp_kev : {8.0, 25.0, 30.0, 90.0}) {
      auto res = macs::calculate_macs(energies, cross_sections, 166.0, temp_kev);
      REQUIRE(res.has_value());
      CHECK(*res == doctest::Approx(EXPECTED).epsilon(TOL));
    }
  }

  TEST_CASE("MACS scales linearly with cross-section magnitude")
  {
    constexpr std::size_t N_PTS = 10'000;

    auto [energies_1, cs_1] = make_constant_cs(1e-6, 2.0, N_PTS, 1.0);
    auto [energies_2, cs_2] = make_constant_cs(1e-6, 2.0, N_PTS, 2.0);

    auto res_1 = macs::calculate_macs(energies_1, cs_1, 166.0, 30.0);
    auto res_2 = macs::calculate_macs(energies_2, cs_2, 166.0, 30.0);

    REQUIRE(res_1.has_value());
    REQUIRE(res_2.has_value());
    CHECK(*res_2 == doctest::Approx(2.0 * *res_1).epsilon(1e-9));
  }
}

TEST_SUITE("calculate_macs: Er-166(n,g) benchmarks [.integration]")
{
  // These tests require network access to fetch real cross-section data.
  // Run with: macs-tests --test-suite="*integration*"
  //
  // Benchmark values for Er-166(n,g), atomic mass A = 166:
  //   library          8 keV      25 keV     30 keV     90 keV   [mb]
  //   ENDF/B-VIII.1   1199.06    665.82     606.11     324.18
  //   JEFF-4.0        1295.41    726.31     659.52     346.84
  //   JENDL-5         1226.08    737.90     681.55     346.62
  //   TENDL-2025      1220.28    672.79     610.47     323.87

  struct BenchmarkCase
  {
    const char* library;
    double macs_8kev;
    double macs_25kev;
    double macs_30kev;
    double macs_90kev;
  };

  constexpr std::array<BenchmarkCase, 4> BENCHMARKS = {{
      {"ENDF/B-VIII.1", 1199.056606, 665.818215, 606.106442, 324.175037},
      {"JEFF-4.0", 1295.414988, 726.308738, 659.520829, 346.835682},
      {"JENDL-5", 1226.079232, 737.903934, 681.549681, 346.620545},
      {"TENDL-2025", 1220.276769, 672.785887, 610.474632, 323.871958},
  }};

  constexpr double ATOMIC_MASS = 166.0;
  constexpr double TOL         = 0.01; // 1%
  constexpr double EV_TO_MEV   = 1e-6; // API returns eV

  TEST_CASE("ENDF/B-VIII.1")
  {
    auto const& b_m = BENCHMARKS[0];
    auto data       = macs::fetch_cross_section("Er-166", "N,G", b_m.library);
    REQUIRE(data.has_value());
    REQUIRE_FALSE(data->datasets.empty());

    auto const& pts = data->datasets[0].points;
    std::vector<double> energies(pts.size());
    std::vector<double> cross_sections(pts.size());
    for (std::size_t idx = 0; idx < pts.size(); ++idx) {
      energies[idx]       = pts[idx].energy * EV_TO_MEV;
      cross_sections[idx] = pts[idx].cross_section;
    }

    for (auto [temp_kev, expected] :
         {std::pair{8.0, b_m.macs_8kev}, std::pair{25.0, b_m.macs_25kev},
          std::pair{30.0, b_m.macs_30kev}, std::pair{90.0, b_m.macs_90kev}}) {
      auto res = macs::calculate_macs(energies, cross_sections, ATOMIC_MASS, temp_kev);
      REQUIRE(res.has_value());
      CHECK(*res == doctest::Approx(expected).epsilon(TOL));
    }
  }
}
