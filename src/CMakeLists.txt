add_library(macs exfor_client.cpp)

add_executable(macs-run main.cpp)

target_include_directories(macs PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(macs
  PRIVATE
    macs-compiler-warnings
    macs-sanitizers
    macs-coverage
    spdlog::spdlog
    cpr::cpr
    nlohmann_json::nlohmann_json
)

target_link_libraries(macs-run
  PRIVATE
    macs
    macs-compiler-warnings
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    bfg::lyra
)

install(TARGETS macs-run
  RUNTIME DESTINATION bin
)
