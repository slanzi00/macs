#ifndef MACS_CALCULATOR_HPP
#define MACS_CALCULATOR_HPP

#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace macs {

using CumulativeMacsRow  = std::pair<double, std::vector<double>>;
using CumulativeMacsData = std::vector<CumulativeMacsRow>;

// energies_mev        : energy grid [MeV]
// cross_sections_barn : cross-section values [barn]
// atomic_mass         : atomic mass number A
// temperature_kev     : temperature kT [keV]
// returns             : MACS [mb]
std::expected<double, std::string> calculate_macs(std::span<double const> energies_mev,
                                                  std::span<double const> cross_sections_barn,
                                                  double atomic_mass, double temperature_kev);

// returns: vector of (energy_mev, cumulative_macs_mb[i] for each temperature)
std::expected<CumulativeMacsData, std::string>
calculate_cumulative_macs(std::span<double const> energies_mev,
                          std::span<double const> cross_sections_barn, double atomic_mass,
                          std::span<double const> temperatures_kev);

} // namespace macs

#endif
