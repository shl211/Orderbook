file(GLOB TEST_SOURCES CONFIGURE_DEPENDS *.cpp)
add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests
    PUBLIC
        orderbook-lib
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)