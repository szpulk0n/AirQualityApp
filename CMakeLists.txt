cmake_minimum_required(VERSION 3.14)
project(AirQualityApp VERSION 1.0.0 LANGUAGES CXX)

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dodanie polityki dla starszych wersji Google Test
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Włączenie testowania
enable_testing()

# Opcja do kompilacji testów
option(BUILD_TESTS "Build the tests" ON)

# Automatyczne znajdowanie wymaganych pakietów
find_package(Qt5 COMPONENTS Core Widgets Concurrent Charts REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)

# Jeśli włączone testy, znajdź Google Test
if(BUILD_TESTS)
    # Dodanie zewnętrznych zależności (Google Test)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0  # Używamy najnowszej wersji
    )
    # Ustawienie zmiennych konfiguracyjnych dla Google Test
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    FetchContent_MakeAvailable(googletest)
endif()

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

# Tworzenie katalogu dla zapisywanych danych
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/export)

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

# Dodanie katalogu z testami, jeśli są włączone
if(BUILD_TESTS)
    add_subdirectory(tests)
    message(STATUS "Google Tests enabled. Build with -DBUILD_TESTS=OFF to disable.")
else()
    message(STATUS "Google Tests disabled. Build with -DBUILD_TESTS=ON to enable.")
endif()

# ======== Konfiguracja Doxygen ========

# Opcja włączenia/wyłączenia generowania dokumentacji
option(BUILD_DOCS "Build documentation" ON)

if(BUILD_DOCS)
    # Wyszukiwanie narzędzia Doxygen
    find_package(Doxygen)
    
    if(DOXYGEN_FOUND)
        # Konfiguracja ścieżek wejściowych dla Doxygen
        set(DOXYGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src")
        set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
        set(DOXYGEN_INDEX_FILE "${DOXYGEN_OUTPUT_DIR}/html/index.html")
        set(DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
        set(DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
        
        # Sprawdzenie czy plik Doxyfile.in istnieje
        if(NOT EXISTS ${DOXYFILE_IN})
            # Jeśli nie, utwórz go z konfiguracyjnymi wartościami
            file(WRITE ${DOXYFILE_IN}
                "PROJECT_NAME           = \"Monitor Jakości Powietrza\"\n"
                "PROJECT_NUMBER         = ${PROJECT_VERSION}\n"
                "PROJECT_BRIEF          = \"Aplikacja do monitorowania jakości powietrza korzystająca z API GIOŚ\"\n"
                "OUTPUT_DIRECTORY       = @DOXYGEN_OUTPUT_DIR@\n"
                "INPUT                  = @DOXYGEN_INPUT_DIR@\n"
                "RECURSIVE              = YES\n"
                "EXTRACT_ALL            = YES\n"
                "EXTRACT_PRIVATE        = YES\n"
                "EXTRACT_STATIC         = YES\n"
                "FILE_PATTERNS          = *.cpp *.hpp\n"
                "GENERATE_HTML          = YES\n"
                "GENERATE_LATEX         = NO\n"
                "USE_MDFILE_AS_MAINPAGE = README.md\n"
                "HAVE_DOT               = YES\n"
                "UML_LOOK               = YES\n"
                "CALL_GRAPH             = YES\n"
                "CALLER_GRAPH           = YES\n"
                "OUTPUT_LANGUAGE        = Polish\n"
                "OPTIMIZE_OUTPUT_FOR_C  = NO\n"
                "GENERATE_TREEVIEW      = YES\n"
                "INLINE_SOURCES         = YES\n"
                "SOURCE_BROWSER         = YES\n"
                "HTML_DYNAMIC_SECTIONS  = YES\n"
                "GENERATE_LEGEND        = YES\n"
            )
        endif()
        
        # Zastąp zmienne w pliku Doxyfile.in i utwórz Doxyfile
        configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)
        
        # Dodaj cel dokumentacji
        add_custom_command(
            OUTPUT ${DOXYGEN_INDEX_FILE}
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
            MAIN_DEPENDENCY ${DOXYFILE} ${DOXYFILE_IN}
            COMMENT "Generowanie dokumentacji API z Doxygen"
            VERBATIM
        )
        
        # Dodaj cel "docs" do budowania w CMake
        add_custom_target(docs ALL DEPENDS ${DOXYGEN_INDEX_FILE})
        
        # Informacja o sukcesie
        message(STATUS "Doxygen znaleziony. Cel 'docs' zostanie utworzony.")
        
        # Instalacja dokumentacji (opcjonalnie)
        install(DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html
                DESTINATION share/doc/${PROJECT_NAME}
                COMPONENT docs
                OPTIONAL
        )
    else()
        # Jeśli Doxygen nie jest zainstalowany
        message(STATUS "Doxygen nie znaleziony. Dokumentacja nie zostanie wygenerowana.")
    endif()
else()
    # Jeśli opcja BUILD_DOCS jest wyłączona
    message(STATUS "Generowanie dokumentacji wyłączone.")
endif()

# ======== Koniec konfiguracji Doxygen ========