# Konfiguracja CTest dla projektu AirQualityApp
set(CTEST_PROJECT_NAME "AirQualityApp")
set(CTEST_NIGHTLY_START_TIME "00:00:00 CET")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "my.cdash.org")
set(CTEST_DROP_LOCATION "/submit.php?project=AirQualityApp")
set(CTEST_DROP_SITE_CDASH TRUE)

# Ustawienie liczby wątków do testowania
set(CTEST_PARALLEL_LEVEL 4)

# Ustawienie timeoutów
set(CTEST_TEST_TIMEOUT 120)

# Konfiguracja wysyłania powiadomień email (opcjonalne)
#set(CTEST_SITE "nazwa_hosta")
#set(CTEST_BUILD_NAME "Linux-gcc")