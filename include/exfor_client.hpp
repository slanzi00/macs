#ifndef MACS_EXFOR_CLIENT_HPP
#define MACS_EXFOR_CLIENT_HPP

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace macs {

struct Section
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

std::expected<std::vector<Section>, std::string>
fetch_sections(std::string_view target, std::string_view reaction, std::string_view quantity);

struct Library
{
  std::uint32_t id;
  std::string name;
};

std::vector<Library> extract_libraries(std::vector<Section> const& sections);

struct CrossSectionPoint
{
  double energy;        // "E" in eV
  double cross_section; // "Sig" in barn
  double d_sig;         // "dSig" uncertainty in barn
};

struct CrossSectionDataset
{
  std::string id;
  std::string file;                      // "FILE"
  std::string data_type;                 // "dataType"
  std::string library;                   // "LIBRARY"
  std::string target;                    // "TARGET"
  double temp;                           // "TEMP"
  std::uint32_t nsub;                    // "NSUB"
  std::uint32_t mat;                     // "MAT"
  std::uint32_t mf;                      // "MF"
  std::uint32_t mt;                      // "MT"
  std::string reaction;                  // "REACTION"
  std::vector<std::string> columns;      // "COLUMNS"
  std::string default_interpolation;     // "defaultInterpolation"
  std::uint32_t n_pts;                   // "nPts"
  std::vector<CrossSectionPoint> points; // "pts"
};

struct CrossSectionResponse
{
  std::string format;
  std::string now;
  std::string program;
  std::vector<CrossSectionDataset> datasets;
};

std::expected<CrossSectionResponse, std::string>
fetch_cross_section(std::string_view target, std::string_view reaction, std::string_view lib_name);

} // namespace macs

#endif
