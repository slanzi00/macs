#ifndef MACS_EXFOR_CLIENT_HPP
#define MACS_EXFOR_CLIENT_HPP

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace macs {

enum class ExforParseError : std::uint8_t
{
  missing_field,
  wrong_type,
  network_error
};

constexpr std::string_view to_string(ExforParseError err) noexcept
{
  switch (err) {
  case ExforParseError::missing_field:
    return "missing_field";
  case ExforParseError::wrong_type:
    return "wrong_type";
  case ExforParseError::network_error:
    return "network_error";
  }
  std::unreachable();
}

struct ExforSection
{
  std::string target;        // nuclide (e.g. "Er-166")
  std::uint32_t a;           // mass number
  std::string lib_name;      // library name (e.g. "ENDF/B-VIII.1")
  std::uint32_t sect_id;     // section ID for data fetch
  std::uint32_t pen_sect_id; // penultimate section ID for data fetch
};

using ExforSections = std::vector<ExforSection>;

[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>;

struct CrossSectionPoint
{
  double energy;        // [eV]
  double cross_section; // [barn]
  double d_sig;         // uncertainty [barn]
};

using CrossSection = std::vector<CrossSectionPoint>;

[[nodiscard]] auto fetch_cross_section(std::string_view target, std::string_view reaction,
                                       std::string_view lib_name)
    -> std::expected<CrossSection, ExforParseError>;

} // namespace macs

#endif // MACS_EXFOR_CLIENT_HPP
