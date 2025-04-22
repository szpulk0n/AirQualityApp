cmake_minimum_required(VERSION 3.14)
project(AirQualityApp VERSION 1.0.0 LANGUAGES CXX)

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatyczne znajdowanie wymaganych pakietów
find_package(Qt5 COMPONENTS Core Widgets Concurrent Charts REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)

# Ścieżki do plików nagłówkowych
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Pliki źródłowe
set(SOURCES
    main.cpp
    src/api_client.cpp
    src/main_window.cpp
)

# Pliki nagłówkowe
set(HEADERS
    include/api_client.hpp
    include/main_window.hpp
)

# Konfiguracja Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tworzenie katalogu dla plików danych
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)

# Tworzenie pliku wykonalnego
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Linkowanie z bibliotekami
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Charts
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Kopiowanie plików zasobów (jeśli istnieją)
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources/)