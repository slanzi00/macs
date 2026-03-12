#ifndef MACS_CALCULATOR_HPP
#define MACS_CALCULATOR_HPP

#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace macs {

// clang-format off
struct MeV { double value; };
struct Barn { double value; };
struct KeV { double value; };
struct Millibarn { double value; };
// clang-format on

std::expected<Millibarn, std::string> calculate_macs(std::span<MeV const> energies,
                                                     std::span<Barn const> cross_sections,
                                                     double atomic_mass, KeV temperature);

std::expected<std::vector<std::pair<MeV, std::vector<Millibarn>>>, std::string>
calculate_cumulative_macs(std::span<MeV const> energies, std::span<Barn const> cross_sections,
                          double atomic_mass, std::span<KeV const> temperatures);

} // namespace macs

#endif
