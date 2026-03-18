#ifndef MACS_EXFOR_CLIENT_HPP
#define MACS_EXFOR_CLIENT_HPP

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace macs {

// @struct ExforSection
// @brief Represents a section of an EXFOR entry.
struct ExforSection
{
  std::string target;        // "Targ"
  std::uint32_t z;           // "ZT"
  std::uint32_t a;           // "AT"
  std::uint32_t nsub;        // "NSUB"
  std::uint32_t mt;          // "MT"
  std::uint32_t mf;          // "MF"
  std::string r;             // "R"
  std::string rc;            // "RC"
  std::uint32_t eval_id;     // "EvalID"
  std::uint32_t sect_id;     // "SectID"
  std::uint32_t pen_sect_id; // "PenSectID"
  std::uint32_t lib_id;      // "LibID"
  std::string lib_name;      // "LibName"
  std::string date;          // "DATE"
  std::string auth;          // "AUTH"
};

// @typedef ExforSections
// @brief Represents a vector of ExforSection objects.
using ExforSections = std::vector<ExforSection>;

// @enum ExforParseError
// @brief Represents an error that occurred while parsing an EXFOR entry.
enum class ExforParseError : std::uint8_t
{
  missing_field,
  wrong_type,
  network_error
};

// @fn fetch_exfor_sections
// @brief Fetches EXFOR sections for a given target, reaction, and quantity.
// @param target The target element symbol.
// @param reaction The reaction string.
// @param quantity The quantity string.
// @return A std::expected containing the ExforSections on success, or an ExforParseError on
// failure.
[[nodiscard]] auto fetch_exfor_sections(std::string_view target, std::string_view reaction,
                                        std::string_view quantity)
    -> std::expected<ExforSections, ExforParseError>;

// @struct CrossSectionPoint
// @brief Represents a point in a cross section dataset.
struct CrossSectionPoint
{
  double energy;        // "E" in eV
  double cross_section; // "Sig" in barn
  double d_sig;         // "dSig" uncertainty in barn
};

// @using CrossSection
// @brief Represents a cross section dataset as a vector of CrossSectionPoint.
using CrossSection = std::vector<CrossSectionPoint>;

// @struct CrossSectionDataset
// @brief Represents a cross section dataset.
struct CrossSectionDataset
{
  std::string id;
  std::string file;                  // "FILE"
  std::string data_type;             // "dataType"
  std::string library;               // "LIBRARY"
  std::string target;                // "TARGET"
  double temp;                       // "TEMP"
  std::uint32_t nsub;                // "NSUB"
  std::uint32_t mat;                 // "MAT"
  std::uint32_t mf;                  // "MF"
  std::uint32_t mt;                  // "MT"
  std::string reaction;              // "REACTION"
  std::vector<std::string> columns;  // "COLUMNS"
  std::string default_interpolation; // "defaultInterpolation"
  std::uint32_t n_pts;               // "nPts"
  CrossSection xsec;
};

[[nodiscard]] auto fetch_cross_section(std::string_view target, std::string_view reaction,
                                       std::string_view lib_name)
    -> std::expected<CrossSectionDataset, ExforParseError>;

} // namespace macs

#endif // MACS_EXFOR_CLIENT_HPP
