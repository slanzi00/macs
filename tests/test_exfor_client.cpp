#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "exfor_client.hpp"

#include <doctest/doctest.h>

namespace {

macs::Section make_section(std::uint32_t lib_id, std::string lib_name)
{
  macs::Section sec{};
  sec.lib_id   = lib_id;
  sec.lib_name = std::move(lib_name);
  return sec;
}

} // namespace

TEST_SUITE("extract_libraries")
{
  TEST_CASE("empty input returns empty list")
  {
    CHECK(macs::extract_libraries({}).empty());
  }

  TEST_CASE("single section returns one library")
  {
    auto constexpr SECTION{800};
    auto libs = macs::extract_libraries({make_section(SECTION, "ENDF/B-VIII.0")});
    REQUIRE(libs.size() == 1);
    CHECK(libs[0].id == SECTION);
    CHECK(libs[0].name == "ENDF/B-VIII.0");
  }

  TEST_CASE("duplicate lib_id appears only once")
  {
    auto constexpr SECTION{800};

    std::vector<macs::Section> sections = {
        make_section(SECTION, "ENDF/B-VIII.0"),
        make_section(SECTION, "ENDF/B-VIII.0"),
        make_section(SECTION, "ENDF/B-VIII.0"),
    };
    CHECK(extract_libraries(sections).size() == 1);
  }

  TEST_CASE("distinct lib_ids all appear")
  {
    auto constexpr SECTION_1{800};
    auto constexpr SECTION_2{75};
    auto constexpr SECTION_3{3300};
    std::vector<macs::Section> sections = {
        make_section(SECTION_1, "ENDF/B-VIII.0"),
        make_section(SECTION_2, "ENDF/B-VII.1"),
        make_section(SECTION_3, "JEFF-3.3"),
    };
    CHECK(extract_libraries(sections).size() == 3);
  }

  TEST_CASE("order preserved")
  {
    auto constexpr SECTION_1{800};
    auto constexpr SECTION_3{3300};

    std::vector<macs::Section> sections = {make_section(SECTION_3, "JEFF-3.3"),
                                           make_section(SECTION_1, "ENDF/B-VIII.0"),
                                           make_section(SECTION_3, "JEFF-3.3")};

    auto libs = extract_libraries(sections);
    REQUIRE(libs.size() == 2);
    CHECK(libs[0].name == "JEFF-3.3");
    CHECK(libs[1].name == "ENDF/B-VIII.0");
  }
}
