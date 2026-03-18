#ifndef MACS_EXFOR_CLIENT_DETAIL_HPP
#define MACS_EXFOR_CLIENT_DETAIL_HPP

#include "exfor_client.hpp"

#include <nlohmann/json.hpp>

namespace macs::detail {

[[nodiscard]] auto parse_exfor_sections(nlohmann::json const& json)
    -> std::expected<ExforSections, ExforParseError>;

[[nodiscard]] auto parse_cross_section(nlohmann::json const& json)
    -> std::expected<CrossSection, ExforParseError>;

} // namespace macs::detail

#endif // MACS_EXFOR_CLIENT_DETAIL_HPP
