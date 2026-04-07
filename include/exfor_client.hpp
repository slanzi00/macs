#ifndef MACS_EXFOR_CLIENT_HPP
#define MACS_EXFOR_CLIENT_HPP

#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace macs {

// @brief Error codes for Exfor parsing errors
enum class ExforParseError : std::uint8_t
{
  missing_field,
  wrong_type,
  network_error
};

// @brief Returns a string representation of the given ExforParseError
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

// @brief Represents a cross section point
struct CrossSectionPoint
{
  double energy;        // [eV]
  double cross_section; // [barn]
  double d_sig;         // uncertainty [barn]
};

// @typedef CrossSection
// @brief A vector of CrossSectionPoint
using CrossSection = std::vector<CrossSectionPoint>;

// @brief Represents a request to fetch a cross section
struct FetchRequest
{
  std::string_view target;
  std::string_view reaction;
  std::string_view lib_name;
};

// @brief Represents the result of a cross section fetch request
struct FetchResult
{
  FetchRequest request;
  std::expected<CrossSection, ExforParseError> data;
};

// @brief Fetches a cross section for the given target, reaction, and library name
// @param target The target nuclide (e.g. "Er-166")
// @param reaction The reaction (e.g. "N,G")
// @param lib_name The library name (e.g. "ENDF/B-VIII.1")
// @return A cross section on success, or an error code on failure
[[nodiscard]] auto fetch_cross_section(std::string_view target, std::string_view reaction,
                                       std::string_view lib_name)
    -> std::expected<CrossSection, ExforParseError>;

// @brief Fetches cross sections for multiple (target, reaction, library) pairs concurrently
// @param requests Span of FetchRequest, each specifying a target nuclide, reaction, and library
// @return A vector of FetchResult in the same order as the input requests. Each result holds
//         either the fetched CrossSection or an ExforParseError — failures do not abort the batch
[[nodiscard]] auto fetch_cross_sections(std::span<FetchRequest const> requests)
    -> std::vector<FetchResult>;

} // namespace macs

#endif // MACS_EXFOR_CLIENT_HPP
