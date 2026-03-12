#include "exfor_client.hpp"

#include <doctest/doctest.h>

using namespace macs;

static Section make_section(std::uint32_t lib_id, std::string lib_name)
{
  Section sec{};
  sec.lib_id   = lib_id;
  sec.lib_name = std::move(lib_name);
  return sec;
}

TEST_SUITE("extract_libraries")
{
  TEST_CASE("empty input returns empty list")
  {
    CHECK(extract_libraries({}).empty());
  }

  TEST_CASE("single section returns one library")
  {
    auto libs = extract_libraries({make_section(800, "ENDF/B-VIII.0")});
    REQUIRE(libs.size() == 1);
    CHECK(libs[0].id == 800);
    CHECK(libs[0].name == "ENDF/B-VIII.0");
  }

  TEST_CASE("duplicate lib_id appears only once")
  {
    std::vector<Section> sections = {
      make_section(800, "ENDF/B-VIII.0"),
      make_section(800, "ENDF/B-VIII.0"),
      make_section(800, "ENDF/B-VIII.0"),
    };
    CHECK(extract_libraries(sections).size() == 1);
  }

  TEST_CASE("distinct lib_ids all appear")
  {
    std::vector<Section> sections = {
      make_section(800, "ENDF/B-VIII.0"),
      make_section(75,  "ENDF/B-VII.1"),
      make_section(3300, "JEFF-3.3"),
    };
    CHECK(extract_libraries(sections).size() == 3);
  }

  TEST_CASE("order preserved")
  {
    std::vector<Section> sections = {
      make_section(3300, "JEFF-3.3"),
      make_section(800,  "ENDF/B-VIII.0"),
      make_section(3300, "JEFF-3.3"),  // duplicate
    };
    auto libs = extract_libraries(sections);
    REQUIRE(libs.size() == 2);
    CHECK(libs[0].name == "JEFF-3.3");
    CHECK(libs[1].name == "ENDF/B-VIII.0");
  }
}
