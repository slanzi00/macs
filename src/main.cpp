#include "exfor_client.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdlib>

int main()
{
  spdlog::set_level(spdlog::level::debug);

  constexpr std::array requests{
      macs::FetchRequest{.target = "Er-166", .reaction = "N,G", .lib_name = "ENDF/B-VIII.1"},
      macs::FetchRequest{.target = "Er-166", .reaction = "N,G", .lib_name = "ENDF/B-VIII.0"},
  };

  bool any_error = false;
  for (auto const& [req, data] : macs::fetch_cross_sections(requests)) {
    auto const code =
        data.transform([&](macs::CrossSection const& xsec) {
              for (auto const& [e, sig, dsig] : xsec) {
                spdlog::info("[{}] E={:.4e} eV  σ={:.4e} barn  δσ={:.4e}", req.lib_name, e, sig,
                             dsig);
              }
              return EXIT_SUCCESS;
            })
            .or_else([&](macs::ExforParseError err) -> std::expected<int, macs::ExforParseError> {
              spdlog::error("[{}] fetch failed: {}", req.lib_name, macs::to_string(err));
              return EXIT_FAILURE;
            })
            .value_or(EXIT_FAILURE);
    if (code == EXIT_FAILURE) {
      any_error = true;
    }
  }

  return any_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
