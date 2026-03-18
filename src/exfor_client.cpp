#include "exfor_client.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <format>

namespace macs::detail {

auto parse_exfor_sections(nlohmann::json const& json)
    -> std::expected<ExforSections, ExforParseError>
{
  ExforSections sections;
  try {
    for (auto const& sec : json.at("sections")) {
      sections.emplace_back(sec.at("Targ").get<std::string>(), sec.at("AT").get<std::uint32_t>(),
                            sec.at("LibName").get<std::string>(),
                            sec.at("SectID").get<std::uint32_t>(),
                            sec.at("PenSectID").get<std::uint32_t>());
    }
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("parse_exfor_sections: {}", e.what());
    return std::unexpected(ExforParseError::wrong_type);
  }
  return sections;
}

auto parse_cross_section(nlohmann::json const& json) -> std::expected<CrossSection, ExforParseError>
{
  CrossSection xsec;
  try {
    for (auto const& point : json.at("datasets").at(0).at("pts")) {
      xsec.push_back({
          point.at("E").get<double>(), point.at("Sig").get<double>(),
          point.value("dSig", 0.0), // not present in all datasets
      });
    }
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("parse_cross_section: {}", e.what());
    return std::unexpected(ExforParseError::wrong_type);
  }
  return xsec;
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

auto http_get_json(std::string const& url) -> std::expected<nlohmann::json, macs::ExforParseError>
{
  auto resp = http_get(url);
  if (resp.status_code != HTTP_OK) {
    spdlog::error("HTTP {}", resp.status_code);
    return std::unexpected(macs::ExforParseError::network_error);
  }
  try {
    return nlohmann::json::parse(resp.text);
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("invalid JSON — {}", e.what());
    return std::unexpected(macs::ExforParseError::wrong_type);
  }
}
// GCOV_EXCL_STOP

} // namespace

// GCOV_EXCL_START — network I/O, not unit-testable without HTTP mocking
namespace macs {

[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>
{
  auto url =
      std::format("https://www-nds.iaea.org/exfor/e4list?Target={}&Reaction={}&Quantity={}&json",
                  target, reaction, quantity);
  auto json = http_get_json(url);
  if (!json) {
    return std::unexpected(json.error());
  }
  return detail::parse_exfor_sections(*json);
}

[[nodiscard]] auto fetch_cross_section(std::string_view target, std::string_view reaction,
                                       std::string_view lib_name)
    -> std::expected<CrossSection, ExforParseError>
{
  auto sections = fetch_exfor_sections(target, reaction, "SIG");
  if (!sections) {
    return std::unexpected(sections.error());
  }

  auto itr = std::ranges::find_if(
      *sections, [&](ExforSection const& sec) { return sec.lib_name == lib_name; });
  if (itr == sections->end()) {
    spdlog::error("library '{}' not found", lib_name);
    return std::unexpected(ExforParseError::missing_field);
  }

  auto url  = std::format("https://www-nds.iaea.org/exfor/e4sig?SectID={}&PenSectID={}&json",
                          itr->sect_id, itr->pen_sect_id);
  auto json = http_get_json(url);
  if (!json) {
    return std::unexpected(json.error());
  }
  return detail::parse_cross_section(*json);
}

} // namespace macs
// GCOV_EXCL_STOP
