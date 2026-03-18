#include "exfor_client.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <fmt/base.h>
#include <spdlog/spdlog.h>

#include <format>

namespace {

auto parse_exfor_sections(nlohmann::json const& response_json)
    -> std::expected<macs::ExforSections, macs::ExforParseError>
{
  macs::ExforSections sections;

  for (auto const& sec : response_json.at("sections")) {
    try {
      sections.emplace_back(
          sec.at("Targ").get<std::string>(), sec.at("ZT").get<std::uint32_t>(),
          sec.at("AT").get<std::uint32_t>(), sec.at("NSUB").get<std::uint32_t>(),
          sec.at("MT").get<std::uint32_t>(), sec.at("MF").get<std::uint32_t>(),
          sec.at("R").get<std::string>(), sec.at("RC").get<std::string>(),
          sec.at("EvalID").get<std::uint32_t>(), sec.at("SectID").get<std::uint32_t>(),
          sec.at("PenSectID").get<std::uint32_t>(), sec.at("LibID").get<std::uint32_t>(),
          sec.at("LibName").get<std::string>(), sec.at("DATE").get<std::string>(),
          sec.at("AUTH").get<std::string>());
    } catch (nlohmann::json::exception const& e) {
      spdlog::error("parse_exfor_sections failed: {}", e.what());
      return std::unexpected(macs::ExforParseError::wrong_type);
    }
  }

  return sections;
}

auto filter_by_library(macs::ExforSections const& sections, std::string_view lib_name)
    -> macs::ExforSections
{
  macs::ExforSections filtered;
  std::ranges::copy_if(sections, std::back_inserter(filtered),
                       [&](macs::ExforSection const& sec) { return sec.lib_name == lib_name; });
  return filtered;
}

} // namespace

namespace macs {

[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>
{
  auto url =
      std::format("https://www-nds.iaea.org/exfor/e4list?Target={}&Reaction={}&Quantity={}&json",
                  target, reaction, quantity);

  spdlog::debug("Selected target: {}, reaction: {}, quantity: {}", target, reaction, quantity);
  spdlog::debug("Requesting URL: {}", url);

  auto response = cpr::Get(cpr::Url{url});

  if (response.status_code != 200) {
    spdlog::error("fetch_sections failed: HTTP {} {}", response.status_code,
                  response.error.message);
    return std::unexpected(ExforParseError::network_error);
  }

  spdlog::debug("Response status: {}", response.status_code);
  spdlog::debug("Elapsed time: {:0.1f}ms", response.elapsed * 1000);

  return parse_exfor_sections(nlohmann::json::parse(response.text));
}

} // namespace macs
