# Konfiguracja testu klienta API z Google Test
add_executable(api_client_gtest api_client_test.cpp)

# Znajdź pakiet Google Test
find_package(GTest REQUIRED)

# Linkowanie z bibliotekami
target_link_libraries(api_client_gtest PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    GTest::GTest
    GTest::Main
)

# Dołączenie ścieżki do plików nagłówkowych
target_include_directories(api_client_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Dodanie ścieżki do plików źródłowych
target_sources(api_client_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/api_client.cpp
)

# Dodanie testu do CTest
add_test(
    NAME ApiClientGTest
    COMMAND api_client_gtest
)