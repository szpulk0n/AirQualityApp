# Konfiguracja testu struktury Station z Google Test
add_executable(station_gtest station_test.cpp)

# Znajdź pakiet Google Test
find_package(GTest REQUIRED)

# Linkowanie z bibliotekami
target_link_libraries(station_gtest PRIVATE
    nlohmann_json::nlohmann_json
    GTest::GTest
    GTest::Main
)

# Dołączenie ścieżki do plików nagłówkowych
target_include_directories(station_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Dodanie testu do CTest
add_test(
    NAME StationGTest
    COMMAND station_gtest
)