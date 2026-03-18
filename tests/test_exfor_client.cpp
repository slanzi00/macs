#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "exfor_client_detail.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

namespace {

json make_sections_json(std::vector<json> sections)
{
  return json{{"sections", sections}};
}

json valid_section()
{
  return {
      {"Targ", "Er-166"}, {"AT", 166},          {"LibName", "ENDF/B-VIII.1"},
      {"SectID", 12345},  {"PenSectID", 12346},
  };
}

json make_xs_json(std::vector<json> pts)
{
  return json{{"datasets", json::array({json{{"pts", pts}}})}};
}

json valid_point(double energy = 1e4, double sig = 2.5, double dsig = 0.1)
{
  return {{"E", energy}, {"Sig", sig}, {"dSig", dsig}};
}
} // namespace

// ---------------------------------------------------------------------------
// macs::detail::parse_exfor_sections
// ---------------------------------------------------------------------------

TEST_SUITE("macs::detail::parse_exfor_sections")
{
  TEST_CASE("empty sections array returns empty vector")
  {
    auto res = macs::detail::parse_exfor_sections(make_sections_json(json::array()));
    REQUIRE(res.has_value());
    CHECK(res->empty());
  }

  TEST_CASE("single valid section is parsed correctly")
  {
    auto res = macs::detail::parse_exfor_sections(make_sections_json({valid_section()}));
    REQUIRE(res.has_value());
    REQUIRE(res->size() == 1);

    auto const& sec = res->front();
    CHECK(sec.target == "Er-166");
    CHECK(sec.a == 166);
    CHECK(sec.lib_name == "ENDF/B-VIII.1");
    CHECK(sec.sect_id == 12345);
    CHECK(sec.pen_sect_id == 12346);
  }

  TEST_CASE("multiple sections are all parsed")
  {
    auto sec2       = valid_section();
    sec2["Targ"]    = "Fe-56";
    sec2["AT"]      = 56;
    sec2["LibName"] = "JEFF-4.0";

    auto res = macs::detail::parse_exfor_sections(make_sections_json({valid_section(), sec2}));
    REQUIRE(res.has_value());
    CHECK(res->size() == 2);
    CHECK((*res)[1].lib_name == "JEFF-4.0");
  }

  TEST_CASE("missing required field returns wrong_type")
  {
    auto bad = valid_section();
    bad.erase("SectID");
    auto res = macs::detail::parse_exfor_sections(make_sections_json({bad}));
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }

  TEST_CASE("field with wrong type returns wrong_type")
  {
    auto bad  = valid_section();
    bad["AT"] = "not-a-number";
    auto res  = macs::detail::parse_exfor_sections(make_sections_json({bad}));
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }

  TEST_CASE("missing top-level sections key returns wrong_type")
  {
    auto res = macs::detail::parse_exfor_sections(json::object());
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }
}

// ---------------------------------------------------------------------------
// macs::detail::parse_cross_section
// ---------------------------------------------------------------------------

TEST_SUITE("macs::detail::parse_cross_section")
{
  TEST_CASE("single point is parsed correctly")
  {
    auto res = macs::detail::parse_cross_section(make_xs_json({valid_point(1e4, 2.5, 0.1)}));
    REQUIRE(res.has_value());
    REQUIRE(res->size() == 1);

    auto const& point = res->front();
    CHECK(point.energy == doctest::Approx(1e4));
    CHECK(point.cross_section == doctest::Approx(2.5));
    CHECK(point.d_sig == doctest::Approx(0.1));
  }

  TEST_CASE("missing dSig defaults to 0.0")
  {
    json point = {{"E", 1e4}, {"Sig", 3.0}};
    auto res   = macs::detail::parse_cross_section(make_xs_json({point}));
    REQUIRE(res.has_value());
    CHECK(res->front().d_sig == doctest::Approx(0.0));
  }

  TEST_CASE("multiple points are all parsed")
  {
    auto res = macs::detail::parse_cross_section(make_xs_json({
        valid_point(1e3, 10.0),
        valid_point(1e4, 2.5),
        valid_point(1e5, 0.5),
    }));
    REQUIRE(res.has_value());
    CHECK(res->size() == 3);
    CHECK((*res)[2].energy == doctest::Approx(1e5));
  }

  TEST_CASE("empty pts array returns empty cross section")
  {
    auto res = macs::detail::parse_cross_section(make_xs_json(json::array()));
    REQUIRE(res.has_value());
    CHECK(res->empty());
  }

  TEST_CASE("missing E field returns wrong_type")
  {
    json point = {{"Sig", 2.5}};
    auto res   = macs::detail::parse_cross_section(make_xs_json({point}));
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }

  TEST_CASE("missing Sig field returns wrong_type")
  {
    json point = {{"E", 1e4}};
    auto res   = macs::detail::parse_cross_section(make_xs_json({point}));
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }

  TEST_CASE("missing datasets key returns wrong_type")
  {
    auto res = macs::detail::parse_cross_section(json::object());
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }

  TEST_CASE("empty datasets array returns wrong_type")
  {
    auto res = macs::detail::parse_cross_section(json{{"datasets", json::array()}});
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == macs::ExforParseError::wrong_type);
  }
}

// ---------------------------------------------------------------------------
// to_string(ExforParseError)
// ---------------------------------------------------------------------------

TEST_SUITE("to_string(ExforParseError)")
{
  TEST_CASE("all enum values have a string representation")
  {
    CHECK(macs::to_string(macs::ExforParseError::missing_field) == "missing_field");
    CHECK(macs::to_string(macs::ExforParseError::wrong_type) == "wrong_type");
    CHECK(macs::to_string(macs::ExforParseError::network_error) == "network_error");
  }
}
