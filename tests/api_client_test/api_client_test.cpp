/**
 * @file api_client_test.cpp
 * @brief Testy klienta API z wykorzystaniem Google Test
 */

 #include "api_client.hpp"
 #include <gtest/gtest.h>
 #include <fstream>
 #include <cstdlib>
 
 // Funkcja sprawdzająca czy plik istnieje
 bool fileExists(const std::string& filename) {
     std::ifstream file(filename);
     return file.good();
 }
 
 // Funkcja tworząca przykładowy plik JSON ze stacjami
 bool createSampleStationsFile(const std::string& filename) {
     std::ofstream file(filename);
     if (!file.is_open()) {
         return false;
     }
     
     file << R"([
         {
             "id": 1,
             "name": "Testowa stacja 1",
             "lat": 52.2297,
             "lon": 21.0122,
             "city": "Warszawa",
             "address": "ul. Przykładowa 1",
             "province": "mazowieckie"
         },
         {
             "id": 2,
             "name": "Testowa stacja 2",
             "lat": 50.0647,
             "lon": 19.9450,
             "city": "Kraków",
             "address": "ul. Testowa 2",
             "province": "małopolskie"
         }
     ])";
     
     file.close();
     return true;
 }
 
 // Klasa testowa dla ApiClient
 class ApiClientTest : public ::testing::Test {
 protected:
     // Konfiguracja przed każdym testem
     void SetUp() override {
         client.setVerbose(false); // Wyłącz komunikaty dla testów
         testDir = "test_data_gtest";
         system(("mkdir -p " + testDir).c_str());
         testFile = testDir + "/test_stations.json";
     }
     
     // Sprzątanie po każdym teście
     void TearDown() override {
         system(("rm -rf " + testDir).c_str());
     }
     
     ApiClient client;
     std::string testDir;
     std::string testFile;
 };
 
 // Test sprawdzania dostępności API
 TEST_F(ApiClientTest, ApiAvailability) {
     bool apiAvailable = client.isApiAvailable();
     // Tylko wyświetlamy wynik, nie asercja, gdyż API może być niedostępne podczas testów
     std::cout << "Status API: " << (apiAvailable ? "dostępne" : "niedostępne") << std::endl;
     
     // Jeśli API jest niedostępne, test przechodzi, ale warto o tym wiedzieć
     if (!apiAvailable) {
         SUCCEED() << "API jest niedostępne, ale test nie jest uznany za niepowodzenie";
     }
 }
 
 // Test zapisywania i odczytywania z pliku JSON
 TEST_F(ApiClientTest, LoadAndSaveStations) {
     // Utwórz przykładowy plik JSON ze stacjami
     ASSERT_TRUE(createSampleStationsFile(testFile)) << "Nie można utworzyć przykładowego pliku JSON";
     ASSERT_TRUE(fileExists(testFile)) << "Plik testowy nie został utworzony";
     
     // Wczytywanie stacji z pliku
     std::vector<Station> stations = client.loadStationsFromFile(testFile);
     ASSERT_FALSE(stations.empty()) << "Wczytanie stacji z pliku nie powiodło się";
     ASSERT_EQ(stations.size(), 2) << "Nieprawidłowa liczba stacji wczytanych z pliku";
     
     // Sprawdzanie danych stacji
     EXPECT_EQ(stations[0].id, 1) << "Nieprawidłowy ID pierwszej stacji";
     EXPECT_EQ(stations[0].name, "Testowa stacja 1") << "Nieprawidłowa nazwa pierwszej stacji";
     EXPECT_EQ(stations[0].city, "Warszawa") << "Nieprawidłowe miasto pierwszej stacji";
     
     EXPECT_EQ(stations[1].id, 2) << "Nieprawidłowy ID drugiej stacji";
     EXPECT_EQ(stations[1].name, "Testowa stacja 2") << "Nieprawidłowa nazwa drugiej stacji";
     EXPECT_EQ(stations[1].city, "Kraków") << "Nieprawidłowe miasto drugiej stacji";
 }
 
 // Test mechanizmu cache
 TEST_F(ApiClientTest, CacheMechanism) {
     // Utwórz przykładowy plik JSON ze stacjami
     ASSERT_TRUE(createSampleStationsFile(testFile));
     
     // Pierwsze wczytanie stacji
     std::vector<Station> stations = client.loadStationsFromFile(testFile);
     ASSERT_FALSE(stations.empty());
     
     // Pobranie tych samych stacji ponownie (powinno użyć cache)
     std::vector<Station> cachedStations = client.loadStationsFromFile(testFile);
     EXPECT_EQ(cachedStations.size(), stations.size()) << "Cache nie działa prawidłowo";
     
     // Czyszczenie cache i ponowne wczytanie
     client.clearCache();
     std::vector<Station> reloadedStations = client.loadStationsFromFile(testFile);
     EXPECT_EQ(reloadedStations.size(), stations.size()) << "Przeładowanie po wyczyszczeniu cache nie działa";
 }
 
 // Test ustawień verbose
 TEST_F(ApiClientTest, VerboseSettings) {
     // Ustawienie verbose na false już zostało wykonane w SetUp()
     EXPECT_FALSE(client.isVerbose()) << "Wartość verbose powinna być false po ustawieniu";
     
     // Zmiana wartości verbose
     client.setVerbose(true);
     EXPECT_TRUE(client.isVerbose()) << "Wartość verbose powinna być true po zmianie";
     
     // Ponowna zmiana wartości verbose
     client.setVerbose(false);
     EXPECT_FALSE(client.isVerbose()) << "Wartość verbose powinna wrócić do false";
 }
 
 // Test walidacji ścieżki pliku
 TEST_F(ApiClientTest, FilePathValidation) {
     // Sprawdzenie zachowania dla nieistniejącego pliku
     std::string nonExistentFile = testDir + "/non_existent_file.json";
     
     // Usunięcie pliku, jeśli istnieje
     std::remove(nonExistentFile.c_str());
     
     // Próba wczytania nieistniejącego pliku
     std::vector<Station> emptyStations = client.loadStationsFromFile(nonExistentFile);
     EXPECT_TRUE(emptyStations.empty()) << "Wczytywanie nieistniejącego pliku powinno zwrócić pusty wektor";
     
     // Sprawdzenie zachowania dla pustej ścieżki
     std::vector<Station> defaultPathStations = client.loadStationsFromFile("");
     // Nie sprawdzamy konkretnego wyniku, bo zależy to od implementacji
     // Po prostu sprawdzamy czy nie ma wyjątków
     SUCCEED() << "Pusta ścieżka nie powoduje wyjątków";
 }
 
 // Main dla Google Test
 int main(int argc, char **argv) {
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
 }