#include "exfor_client.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>

int main()
{
  spdlog::set_level(spdlog::level::debug);

  std::string_view target{"Er-166"};
  std::string_view reaction{"N,G"};
  std::string_view lib_name{"ENDF/B-VIII.0"};

  auto result = macs::fetch_cross_section(target, reaction, lib_name);
  if (!result) {
    spdlog::error("fetch failed: {}", macs::to_string(result.error()));
    return EXIT_FAILURE;
  }

  for (auto const& [energy, cross_section, d_sig] : *result) {
    spdlog::info("E={:.4e} eV  σ={:.4e} barn  δσ={:.4e}", energy, cross_section, d_sig);
  }
}
