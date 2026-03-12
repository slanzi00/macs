#include "exfor_client.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <format>

namespace {

constexpr auto HTTP_OK    = 200L;
constexpr auto MS_PER_SEC = 1000.0;

struct E4Response
{
  std::string format;
  std::string now;
  std::string program;
  std::uint32_t req{};
  std::vector<macs::Section> sections;
};

E4Response parse_e4_response(nlohmann::json const& json)
{
  E4Response result;
  result.format  = json.at("format").get<std::string>();
  result.now     = json.at("now").get<std::string>();
  result.program = json.at("program").get<std::string>();
  result.req     = json.at("req").get<std::uint32_t>();

  for (auto const& sec_json : json.at("sections")) {
    macs::Section sec;
    sec.target      = sec_json.at("Targ").get<std::string>();
    sec.z           = sec_json.at("ZT").get<std::uint32_t>();
    sec.a           = sec_json.at("AT").get<std::uint32_t>();
    sec.nsub        = sec_json.at("NSUB").get<std::uint32_t>();
    sec.mt          = sec_json.at("MT").get<std::uint32_t>();
    sec.mf          = sec_json.at("MF").get<std::uint32_t>();
    sec.r           = sec_json.at("R").get<std::string>();
    sec.rc          = sec_json.at("RC").get<std::string>();
    sec.eval_id     = sec_json.at("EvalID").get<std::uint32_t>();
    sec.sect_id     = sec_json.at("SectID").get<std::uint32_t>();
    sec.pen_sect_id = sec_json.at("PenSectID").get<std::uint32_t>();
    sec.lib_id      = sec_json.at("LibID").get<std::uint32_t>();
    sec.lib_name    = sec_json.at("LibName").get<std::string>();
    sec.date        = sec_json.at("DATE").get<std::string>();
    sec.auth        = sec_json.at("AUTH").get<std::string>();
    result.sections.push_back(std::move(sec));
  }

  return result;
}

std::vector<macs::Section> filter_by_library(std::vector<macs::Section> const& sections,
                                             std::string_view lib_name)
{
  std::vector<macs::Section> filtered;
  std::ranges::copy_if(sections, std::back_inserter(filtered),
                       [&](macs::Section const& sec) { return sec.lib_name == lib_name; });
  return filtered;
}

macs::CrossSectionResponse parse_cross_section_response(nlohmann::json const& json)
{
  macs::CrossSectionResponse result;
  result.format  = json.at("format").get<std::string>();
  result.now     = json.at("now").get<std::string>();
  result.program = json.at("program").get<std::string>();

  for (auto const& dst : json.at("datasets")) {
    macs::CrossSectionDataset data;
    data.id                    = dst.at("id").get<std::string>();
    data.file                  = dst.at("FILE").get<std::string>();
    data.data_type             = dst.at("dataType").get<std::string>();
    data.library               = dst.at("LIBRARY").get<std::string>();
    data.target                = dst.at("TARGET").get<std::string>();
    data.temp                  = dst.at("TEMP").get<double>();
    data.nsub                  = dst.at("NSUB").get<std::uint32_t>();
    data.mat                   = dst.at("MAT").get<std::uint32_t>();
    data.mf                    = dst.at("MF").get<std::uint32_t>();
    data.mt                    = dst.at("MT").get<std::uint32_t>();
    data.reaction              = dst.at("REACTION").get<std::string>();
    data.columns               = dst.at("COLUMNS").get<std::vector<std::string>>();
    data.default_interpolation = dst.at("defaultInterpolation").get<std::string>();
    data.n_pts                 = dst.at("nPts").get<std::uint32_t>();
    for (auto const& point : dst.at("pts")) {
      data.points.push_back({
          point.at("E").get<double>(),
          point.at("Sig").get<double>(),
          point.value("dSig", 0.0),
      });
    }
    result.datasets.push_back(std::move(data));
  }

  return result;
}

} // namespace

namespace macs {

std::expected<std::vector<Section>, std::string>
fetch_sections(std::string_view target, std::string_view reaction, std::string_view quantity)
{
  auto url =
      std::format("https://www-nds.iaea.org/exfor/e4list?Target={}&Reaction={}&Quantity={}&json",
                  target, reaction, quantity);

  spdlog::debug("fetch_sections GET {}", url);
  auto resp = cpr::Get(cpr::Url{url});
  spdlog::debug("fetch_sections status={} elapsed={:.0f}ms", resp.status_code,
                resp.elapsed * MS_PER_SEC);

  if (resp.status_code != HTTP_OK) {
    spdlog::error("fetch_sections failed: HTTP {} {}", resp.status_code, resp.error.message);
    return std::unexpected{std::format("HTTP {}: {}", resp.status_code, resp.error.message)};
  }

  try {
    auto parsed = parse_e4_response(nlohmann::json::parse(resp.text));
    spdlog::info("fetch_sections ok: {} sections for {}/{}/{}", parsed.sections.size(), target,
                 reaction, quantity);
    return parsed.sections;
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("fetch_sections JSON parse error: {}", e.what());
    return std::unexpected{std::format("JSON parse error: {}", e.what())};
  }
}

std::vector<Library> extract_libraries(std::vector<Section> const& sections)
{
  std::vector<Library> libs;
  for (auto const& sec : sections) {
    auto itr = std::ranges::find_if(libs, [&](Library const& lib) { return lib.id == sec.lib_id; });
    if (itr == libs.end()) {
      libs.push_back({sec.lib_id, sec.lib_name});
    }
  }
  return libs;
}

std::expected<CrossSectionResponse, std::string>
fetch_cross_section(std::string_view target, std::string_view reaction, std::string_view lib_name)
{
  auto sections = fetch_sections(target, reaction, "SIG");
  if (!sections) {
    return std::unexpected{sections.error()};
  }

  auto filtered = filter_by_library(*sections, lib_name);
  if (filtered.empty()) {
    auto msg = std::format("no sections found for library '{}'", lib_name);
    spdlog::error("fetch_cross_section: {}", msg);
    return std::unexpected{msg};
  }

  auto const& sec = filtered.front();
  auto url        = std::format("https://www-nds.iaea.org/exfor/e4sig?SectID={}&PenSectID={}&json",
                                sec.sect_id, sec.pen_sect_id);

  spdlog::debug("fetch_cross_section GET {}", url);
  auto resp = cpr::Get(cpr::Url{url});
  spdlog::debug("fetch_cross_section status={} elapsed={:.0f}ms", resp.status_code,
                resp.elapsed * MS_PER_SEC);

  if (resp.status_code != HTTP_OK) {
    spdlog::error("fetch_cross_section failed: HTTP {} {}", resp.status_code, resp.error.message);
    return std::unexpected{std::format("HTTP {}: {}", resp.status_code, resp.error.message)};
  }

  try {
    auto result = parse_cross_section_response(nlohmann::json::parse(resp.text));
    spdlog::info("fetch_cross_section ok: {} datasets for {}/{} [{}]", result.datasets.size(),
                 target, reaction, lib_name);
    return result;
  } catch (nlohmann::json::exception const& e) {
    spdlog::error("fetch_cross_section JSON parse error: {}", e.what());
    return std::unexpected{std::format("JSON parse error: {}", e.what())};
  }
}

} // namespace macs
