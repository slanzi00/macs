#ifndef MACS_EXFOR_CLIENT_DETAIL_HPP
#define MACS_EXFOR_CLIENT_DETAIL_HPP

// @note Internal header — not part of the public API.
//       Include only in tests or internal translation units.

#include "exfor_client.hpp"

#include <nlohmann/json.hpp>

namespace macs {

// @brief Represents a section of an Exfor record
struct ExforSection
{
  std::string target;        // nuclide (e.g. "Er-166")
  std::uint32_t a;           // mass number
  std::string lib_name;      // library name (e.g. "ENDF/B-VIII.1")
  std::uint32_t sect_id;     // section ID for data fetch
  std::uint32_t pen_sect_id; // penultimate section ID for data fetch
};

// @typedef ExforSections
// @brief A vector of ExforSection
using ExforSections = std::vector<ExforSection>;

// @brief Fetches Exfor sections for the given target, reaction, and quantity
// @param target The target nuclide (e.g. "Er-166")
// @param reaction The reaction (e.g. "N,G")
// @param quantity The quantity (e.g. "SIG")
// @return A vector of Exfor sections on success, or an error code on failure
[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>;

namespace detail {

// @brief Parses an Exfor JSON response into a vector of sections
// @param json The JSON response
// @return A vector of sections on success, or an error code on failure
[[nodiscard]] auto parse_exfor_sections(nlohmann::json const& json)
    -> std::expected<ExforSections, ExforParseError>;

// @brief Parses a cross section JSON response into a vector of points
// @param json The JSON response
// @return A cross section on success, or an error code on failure
[[nodiscard]] auto parse_cross_section(nlohmann::json const& json)
    -> std::expected<CrossSection, ExforParseError>;

} // namespace detail

} // namespace macs

#endif // MACS_EXFOR_CLIENT_DETAIL_HPP
