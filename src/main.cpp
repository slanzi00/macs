#include "exfor_client.hpp"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <format>
#include <iostream>

int main()
{
  spdlog::set_level(spdlog::level::debug);

  auto const& response = macs::fetch_cross_section("Fe-56", "N,G", "ENDF/B-VIII.0").value();

  for (auto const& dst : response.datasets) {
    std::cout << std::format("  {} pts — {}\n", dst.n_pts, dst.reaction);
  }
}
