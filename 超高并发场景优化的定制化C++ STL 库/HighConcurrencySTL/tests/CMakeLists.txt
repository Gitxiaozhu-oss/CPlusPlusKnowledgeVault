enable_testing()

add_executable(container_tests
    container_tests.cpp
)

target_link_libraries(container_tests
    PRIVATE
    hcstl
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(container_tests) 