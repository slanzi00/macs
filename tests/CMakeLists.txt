add_executable(macs-tests test_exfor_client.cpp)


target_link_libraries(macs-tests
  PRIVATE
    macs
    macs-compiler-warnings
    macs-coverage
    doctest::doctest
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

include(doctest)
doctest_discover_tests(macs-tests)
