add_library(macs exfor_client.cpp)

add_executable(macs-run main.cpp)

target_include_directories(macs PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(macs
  PRIVATE
    macs-compiler-warnings
    macs-sanitizers
    spdlog::spdlog
    cpr::cpr
    nlohmann_json::nlohmann_json
)

target_link_libraries(macs-run
  PRIVATE
    macs
    macs-compiler-warnings
    cpr::cpr
    nlohmann_json::nlohmann_json
)
