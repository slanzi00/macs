add_executable(macs-tests test_exfor_client.cpp)

target_link_libraries(macs-tests
  PRIVATE
    macs
    macs-compiler-warnings
    doctest::doctest
)

add_test(NAME macs-tests COMMAND macs-tests)
