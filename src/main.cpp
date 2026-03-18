#include "exfor_client.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>

int main()
{
  spdlog::set_level(spdlog::level::debug);

  constexpr std::string_view target{"Er-166"};
  constexpr std::string_view reaction{"N,G"};
  constexpr std::string_view lib_name{"ENDF/B-VIII.0"};

  return macs::fetch_cross_section(target, reaction, lib_name)
      .transform([](macs::CrossSection const& xsec) {
        for (auto const& [e, sig, dsig] : xsec) {
          spdlog::info("E={:.4e} eV  σ={:.4e} barn  δσ={:.4e}", e, sig, dsig);
        }
        return EXIT_SUCCESS;
      })
      .or_else([](macs::ExforParseError err) -> std::expected<int, macs::ExforParseError> {
        spdlog::error("fetch failed: {}", macs::to_string(err));
        return EXIT_FAILURE;
      })
      .value_or(EXIT_FAILURE);
}
