#include "exfor_client.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <format>
#include <ranges>

namespace macs::detail {

auto parse_exfor_sections(nlohmann::json const& json)
    -> std::expected<ExforSections, ExforParseError>
{
  try {
    return json.at("sections") | std::views::transform([](nlohmann::json const& sec) {
             return ExforSection{
                 .target      = sec.at("Targ").get<std::string>(),
                 .a           = sec.at("AT").get<std::uint32_t>(),
                 .lib_name    = sec.at("LibName").get<std::string>(),
                 .sect_id     = sec.at("SectID").get<std::uint32_t>(),
                 .pen_sect_id = sec.at("PenSectID").get<std::uint32_t>(),
             };
           })
         | std::ranges::to<ExforSections>();
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("parse_exfor_sections: {}", e.what());
    return std::unexpected(ExforParseError::wrong_type);
  }
}

auto parse_cross_section(nlohmann::json const& json) -> std::expected<CrossSection, ExforParseError>
{
  try {
    return json.at("datasets").at(0).at("pts")
         | std::views::transform([](nlohmann::json const& point) {
             return CrossSectionPoint{
                 .energy        = point.at("E").get<double>(),
                 .cross_section = point.at("Sig").get<double>(),
                 .d_sig         = point.value("dSig", 0.0) // not present in all datasets
             };
           })
         | std::ranges::to<CrossSection>();
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("parse_cross_section: {}", e.what());
    return std::unexpected(ExforParseError::wrong_type);
  }
}

} // namespace macs::detail

namespace {

constexpr std::int64_t HTTP_OK{200};

// GCOV_EXCL_START — network I/O, not unit-testable without HTTP mocking
auto http_get(std::string const& url) -> cpr::Response
{
  spdlog::debug("GET {}", url);
  auto resp = cpr::Get(cpr::Url{url});
  if (resp.error) {
    spdlog::error("network error: {}", resp.error.message);
  } else {
    spdlog::debug("status={} elapsed={:.0f}ms", resp.status_code, resp.elapsed * 1000.0);
  }
  return resp;
}

auto check_status(cpr::Response resp) -> std::expected<cpr::Response, macs::ExforParseError>
{
  if (resp.status_code != HTTP_OK) {
    spdlog::error("HTTP {}", resp.status_code);
    return std::unexpected(macs::ExforParseError::network_error);
  }
  return resp;
}

auto parse_body(cpr::Response const& resp) -> std::expected<nlohmann::json, macs::ExforParseError>
{
  try {
    return nlohmann::json::parse(resp.text);
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("invalid JSON — {}", e.what());
    return std::unexpected(macs::ExforParseError::wrong_type);
  }
}

auto http_get_json(std::string const& url) -> std::expected<nlohmann::json, macs::ExforParseError>
{
  return check_status(http_get(url)).and_then(parse_body);
}
// GCOV_EXCL_STOP

} // namespace

// GCOV_EXCL_START — network I/O, not unit-testable without HTTP mocking
namespace macs {

[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>
{
  return http_get_json(
             std::format(
                 "https://www-nds.iaea.org/exfor/e4list?Target={}&Reaction={}&Quantity={}&json",
                 target, reaction, quantity))
      .and_then(detail::parse_exfor_sections);
}

[[nodiscard]] auto fetch_cross_section(std::string_view target, std::string_view reaction,
                                       std::string_view lib_name)
    -> std::expected<CrossSection, ExforParseError>
{
  auto find_section =
      [&](ExforSections const& sections) -> std::expected<ExforSection, ExforParseError> {
    auto itr = std::ranges::find(sections, lib_name, &ExforSection::lib_name);
    if (itr == sections.end()) {
      spdlog::error("library '{}' not found", lib_name);
      return std::unexpected(ExforParseError::missing_field);
    }
    return *itr;
  };

  auto fetch_xs = [](ExforSection const& sec) {
    return http_get_json(
               std::format("https://www-nds.iaea.org/exfor/e4sig?SectID={}&PenSectID={}&json",
                           sec.sect_id, sec.pen_sect_id))
        .and_then(detail::parse_cross_section);
  };

  return fetch_exfor_sections(target, reaction, "SIG").and_then(find_section).and_then(fetch_xs);
}

} // namespace macs
// GCOV_EXCL_STOP
