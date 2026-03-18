#include "exfor_client.hpp"

#include <spdlog/spdlog.h>

int main()
{
  spdlog::set_level(spdlog::level::debug);

  std::string_view target{"Er-166"};
  std::string_view reaction{"N,G"};
  std::string_view quantity{"SIG"};

  auto result = macs::fetch_exfor_sections(target, reaction, quantity);
}
