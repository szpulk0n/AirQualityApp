# Konfiguracja testów CTest z Google Test
enable_testing()

# Znajdź pakiet Google Test
find_package(GTest REQUIRED)
include(GoogleTest)

# Dodajemy katalogi z testami
add_subdirectory(api_client_test)
add_subdirectory(station_test)

# Informacja o znalezieniu Google Test
message(STATUS "Google Test found. Tests will use GTest framework.")