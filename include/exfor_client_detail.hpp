#ifndef MACS_EXFOR_CLIENT_DETAIL_HPP
#define MACS_EXFOR_CLIENT_DETAIL_HPP

// @note Internal header — not part of the public API.
//       Include only in tests or internal translation units.

#include "exfor_client.hpp"

#include <nlohmann/json.hpp>

namespace macs::detail {

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

} // namespace macs::detail

#endif // MACS_EXFOR_CLIENT_DETAIL_HPP
