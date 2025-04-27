/**
 * @file station_test.cpp
 * @brief Testy struktury Station z wykorzystaniem Google Test
 */

 #include "api_client.hpp"
 #include <gtest/gtest.h>
 #include <nlohmann/json.hpp>
 
 using json = nlohmann::json;
 
 // Klasa testowa dla struktury Station
 class StationTest : public ::testing::Test {
 protected:
     // Konfiguracja przed każdym testem
     void SetUp() override {
         // Inicjalizacja przykładowej stacji
         station.id = 14;
         station.name = "Warszawa-Ursynów";
         station.lat = 52.161;
         station.lon = 21.036;
         station.city = "Warszawa";
         station.address = "ul. Wokalista 1";
         station.province = "mazowieckie";
     }
     
     Station station;
 };
 
 // Test tworzenia i inicjalizacji stacji
 TEST_F(StationTest, CreationAndInitialization) {
     EXPECT_EQ(station.id, 14) << "Nieprawidłowy ID stacji";
     EXPECT_EQ(station.name, "Warszawa-Ursynów") << "Nieprawidłowa nazwa stacji";
     EXPECT_DOUBLE_EQ(station.lat, 52.161) << "Nieprawidłowa szerokość geograficzna";
     EXPECT_DOUBLE_EQ(station.lon, 21.036) << "Nieprawidłowa długość geograficzna";
     EXPECT_EQ(station.city, "Warszawa") << "Nieprawidłowe miasto";
     EXPECT_EQ(station.address, "ul. Wokalista 1") << "Nieprawidłowy adres";
     EXPECT_EQ(station.province, "mazowieckie") << "Nieprawidłowe województwo";
 }
 
 // Test konwersji stacji do/z formatu JSON
 TEST_F(StationTest, JsonConversion) {
     // Konwersja stacji do JSON
     json stationJson;
     stationJson["id"] = station.id;
     stationJson["name"] = station.name;
     stationJson["lat"] = station.lat;
     stationJson["lon"] = station.lon;
     stationJson["city"] = station.city;
     stationJson["address"] = station.address;
     stationJson["province"] = station.province;
     
     // Sprawdzenie, czy JSON zawiera poprawne dane
     EXPECT_EQ(stationJson["id"], 14) << "Nieprawidłowy ID stacji w JSON";
     EXPECT_EQ(stationJson["name"], "Warszawa-Ursynów") << "Nieprawidłowa nazwa stacji w JSON";
     EXPECT_DOUBLE_EQ(stationJson["lat"].get<double>(), 52.161) << "Nieprawidłowa szerokość geograficzna w JSON";
     EXPECT_DOUBLE_EQ(stationJson["lon"].get<double>(), 21.036) << "Nieprawidłowa długość geograficzna w JSON";
     
     // Konwersja z JSON do stacji
     Station stationFromJson;
     stationFromJson.id = stationJson["id"];
     stationFromJson.name = stationJson["name"];
     stationFromJson.lat = stationJson["lat"];
     stationFromJson.lon = stationJson["lon"];
     stationFromJson.city = stationJson["city"];
     stationFromJson.address = stationJson["address"];
     stationFromJson.province = stationJson["province"];
     
     // Sprawdzenie, czy dane są poprawne po konwersji
     EXPECT_EQ(stationFromJson.id, station.id) << "Nieprawidłowy ID stacji po konwersji";
     EXPECT_EQ(stationFromJson.name, station.name) << "Nieprawidłowa nazwa stacji po konwersji";
     EXPECT_DOUBLE_EQ(stationFromJson.lat, station.lat) << "Nieprawidłowa szerokość geograficzna po konwersji";
     EXPECT_DOUBLE_EQ(stationFromJson.lon, station.lon) << "Nieprawidłowa długość geograficzna po konwersji";
 }
 
 // Test operacji na wektorze stacji
 TEST_F(StationTest, StationsVector) {
     std::vector<Station> stations;
     stations.push_back(station);
     
     // Utworzenie drugiej stacji
     Station station2;
     station2.id = 15;
     station2.name = "Kraków-Nowa Huta";
     station2.lat = 50.069;
     station2.lon = 20.053;
     station2.city = "Kraków";
     
     stations.push_back(station2);
     
     // Sprawdzenie rozmiaru wektora
     EXPECT_EQ(stations.size(), 2) << "Nieprawidłowy rozmiar wektora stacji";
     
     // Sprawdzenie danych w wektorze
     EXPECT_EQ(stations[0].id, 14) << "Nieprawidłowy ID pierwszej stacji w wektorze";
     EXPECT_EQ(stations[1].id, 15) << "Nieprawidłowy ID drugiej stacji w wektorze";
     EXPECT_EQ(stations[0].name, "Warszawa-Ursynów") << "Nieprawidłowa nazwa pierwszej stacji w wektorze";
     EXPECT_EQ(stations[1].name, "Kraków-Nowa Huta") << "Nieprawidłowa nazwa drugiej stacji w wektorze";
 }
 
 // Test porównywania stacji
 TEST_F(StationTest, StationComparison) {
     // Utworzenie kopii stacji
     Station stationCopy;
     stationCopy.id = station.id;
     stationCopy.name = station.name;
     stationCopy.lat = station.lat;
     stationCopy.lon = station.lon;
     stationCopy.city = station.city;
     stationCopy.address = station.address;
     stationCopy.province = station.province;
     
     // Porównanie pól stacji
     EXPECT_EQ(stationCopy.id, station.id) << "ID kopii nie zgadza się z oryginałem";
     EXPECT_EQ(stationCopy.name, station.name) << "Nazwa kopii nie zgadza się z oryginałem";
     EXPECT_DOUBLE_EQ(stationCopy.lat, station.lat) << "Szerokość geograficzna kopii nie zgadza się z oryginałem";
     EXPECT_DOUBLE_EQ(stationCopy.lon, station.lon) << "Długość geograficzna kopii nie zgadza się z oryginałem";
     EXPECT_EQ(stationCopy.city, station.city) << "Miasto kopii nie zgadza się z oryginałem";
     EXPECT_EQ(stationCopy.address, station.address) << "Adres kopii nie zgadza się z oryginałem";
     EXPECT_EQ(stationCopy.province, station.province) << "Województwo kopii nie zgadza się z oryginałem";
     
     // Modyfikacja kopii
     stationCopy.name = "Zmodyfikowana stacja";
     
     // Sprawdzenie czy oryginał nie został zmieniony
     EXPECT_NE(station.name, stationCopy.name) << "Modyfikacja kopii nie powinna wpływać na oryginał";
     EXPECT_EQ(station.name, "Warszawa-Ursynów") << "Oryginalna nazwa stacji nie powinna się zmienić";
 }
 
// Test stacji z niepełnymi danymi
TEST_F(StationTest, MinimalStationData) {
    // Utworzenie stacji z minimalnymi danymi i inicjalizacją pozostałych pól
    Station minimalStation = {};  // Inicjalizacja wszystkich pól na 0/puste
    minimalStation.id = 100;
    minimalStation.name = "Minimalna stacja";
    
    // Sprawdzenie, czy stacja z minimalnymi danymi jest poprawna
    EXPECT_EQ(minimalStation.id, 100) << "Nieprawidłowy ID minimalnej stacji";
    EXPECT_EQ(minimalStation.name, "Minimalna stacja") << "Nieprawidłowa nazwa minimalnej stacji";
    
    // Domyślne wartości dla nieprzypisanych pól
    EXPECT_DOUBLE_EQ(minimalStation.lat, 0.0) << "Domyślna wartość lat powinna być 0.0";
    EXPECT_DOUBLE_EQ(minimalStation.lon, 0.0) << "Domyślna wartość lon powinna być 0.0";
    EXPECT_TRUE(minimalStation.city.empty()) << "Domyślna wartość city powinna być pusta";
    EXPECT_TRUE(minimalStation.address.empty()) << "Domyślna wartość address powinna być pusta";
    EXPECT_TRUE(minimalStation.province.empty()) << "Domyślna wartość province powinna być pusta";
     
     // Konwersja minimalnej stacji do JSON
     json minimalJson;
     minimalJson["id"] = minimalStation.id;
     minimalJson["name"] = minimalStation.name;
     
     // Sprawdzenie konwersji minimalnej stacji
     EXPECT_EQ(minimalJson["id"], 100) << "Nieprawidłowy ID minimalnej stacji w JSON";
     EXPECT_EQ(minimalJson["name"], "Minimalna stacja") << "Nieprawidłowa nazwa minimalnej stacji w JSON";
 }
 
 // Main dla Google Test
 int main(int argc, char **argv) {
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
 }