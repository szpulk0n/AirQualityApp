/**
 * @file api_client.cpp
 * @brief Implementacja klienta API do pobierania danych o jakości powietrza z GIOŚ
 */

 #include "api_client.hpp"
 #include <curl/curl.h>
 #include <fstream>
 #include <iostream>
 #include <stdexcept>
 
 // Funkcja pomocnicza do zapisywania odpowiedzi z libcurl
 static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
     ((std::string*)userp)->append((char*)contents, size * nmemb);
     return size * nmemb;
 }
 
 ApiClient::ApiClient() {
     // Inicjalizacja libcurl
     curl_global_init(CURL_GLOBAL_DEFAULT);
 }
 
 ApiClient::~ApiClient() {
     // Czyszczenie zasobów libcurl
     curl_global_cleanup();
 }
 
 json ApiClient::makeRequest(const std::string& endpoint) {
     CURL* curl;
     CURLcode res;
     std::string readBuffer;
     json responseJson;
 
     curl = curl_easy_init();
     if (!curl) {
         throw std::runtime_error("Błąd inicjalizacji libcurl");
     }
 
     std::string url = baseUrl + endpoint;
     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10); // Timeout 10 sekund
     
     // Dodanie User-Agent
     curl_easy_setopt(curl, CURLOPT_USERAGENT, "AirQualityApp/1.0");
     
     // Opcjonalnie wyłączenie weryfikacji SSL, jeśli używamy HTTPS
     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
     
     // Wykonanie zapytania
     res = curl_easy_perform(curl);
     
     // Sprawdzenie czy zapytanie się powiodło
     if (res != CURLE_OK) {
         curl_easy_cleanup(curl);
         throw std::runtime_error("Błąd podczas wykonywania zapytania: " + std::string(curl_easy_strerror(res)));
     }
     
     // Czyszczenie zasobów
     curl_easy_cleanup(curl);
     
     try {
         responseJson = json::parse(readBuffer);
     } catch (const json::parse_error& e) {
         throw std::runtime_error("Błąd parsowania JSON: " + std::string(e.what()));
     }
     
     return responseJson;
 }
 
 bool ApiClient::isApiAvailable() {
     try {
         // Próba pobrania listy stacji jako test dostępności API
         makeRequest("/station/findAll");
         return true;
     } catch (const std::exception& e) {
         return false;
     }
 }
 
 std::vector<Station> ApiClient::getAllStations() {
     std::vector<Station> stations;
     
     try {
         json response = makeRequest("/station/findAll");
         
         for (const auto& item : response) {
             Station station;
             station.id = item["id"];
             station.name = item["stationName"];
             station.lat = item["gegrLat"];
             station.lon = item["gegrLon"];
             
             // Pobieranie informacji o mieście i adresie
             if (!item["city"].is_null()) {
                 station.city = item["city"]["name"];
                 
                 if (!item["city"]["commune"].is_null()) {
                     station.province = item["city"]["commune"]["provinceName"];
                 }
             }
             
             if (!item["addressStreet"].is_null()) {
                 station.address = item["addressStreet"];
             }
             
             stations.push_back(station);
         }
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas pobierania stacji: " << e.what() << std::endl;
     }
     
     return stations;
 }
 
 bool ApiClient::saveStationsToFile(const std::string& filename) {
     try {
         std::vector<Station> stations = getAllStations();
         json stationsJson = json::array();
         
         for (const auto& station : stations) {
             json stationJson;
             stationJson["id"] = station.id;
             stationJson["name"] = station.name;
             stationJson["lat"] = station.lat;
             stationJson["lon"] = station.lon;
             stationJson["city"] = station.city;
             stationJson["address"] = station.address;
             stationJson["province"] = station.province;
             
             stationsJson.push_back(stationJson);
         }
         
         std::ofstream file(filename);
         if (!file.is_open()) {
             std::cerr << "Nie można otworzyć pliku do zapisu: " << filename << std::endl;
             return false;
         }
         
         file << stationsJson.dump(4); // Pretty print z wcięciem 4 spacji
         file.close();
         
         return true;
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas zapisywania stacji do pliku: " << e.what() << std::endl;
         return false;
     }
 }
 
 std::vector<Station> ApiClient::loadStationsFromFile(const std::string& filename) {
     std::vector<Station> stations;
     
     try {
         std::ifstream file(filename);
         if (!file.is_open()) {
             std::cerr << "Nie można otworzyć pliku: " << filename << std::endl;
             return stations;
         }
         
         json stationsJson;
         file >> stationsJson;
         
         for (const auto& item : stationsJson) {
             Station station;
             station.id = item["id"];
             station.name = item["name"];
             station.lat = item["lat"];
             station.lon = item["lon"];
             station.city = item["city"];
             station.address = item["address"];
             station.province = item["province"];
             
             stations.push_back(station);
         }
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas wczytywania stacji z pliku: " << e.what() << std::endl;
     }
     
     return stations;
 }
 
 std::vector<Sensor> ApiClient::getSensors(int stationId) {
     std::vector<Sensor> sensors;
     
     try {
         json response = makeRequest("/station/sensors/" + std::to_string(stationId));
         
         for (const auto& item : response) {
             Sensor sensor;
             sensor.id = item["id"];
             sensor.stationId = item["stationId"];
             
             if (!item["param"].is_null()) {
                 sensor.paramName = item["param"]["paramName"];
                 sensor.paramFormula = item["param"]["paramFormula"];
                 sensor.paramCode = item["param"]["paramCode"];
                 sensor.paramId = item["param"]["idParam"];
             }
             
             sensors.push_back(sensor);
         }
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas pobierania czujników: " << e.what() << std::endl;
     }
     
     return sensors;
 }
 
 std::vector<Measurement> ApiClient::getMeasurements(int sensorId) {
     std::vector<Measurement> measurements;
     
     try {
         json response = makeRequest("/data/getData/" + std::to_string(sensorId));
         
         if (response.contains("values") && response["values"].is_array()) {
             for (const auto& item : response["values"]) {
                 if (!item["value"].is_null()) {
                     Measurement measurement;
                     measurement.date = item["date"];
                     measurement.value = item["value"];
                     measurements.push_back(measurement);
                 }
             }
         }
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas pobierania pomiarów: " << e.what() << std::endl;
     }
     
     return measurements;
 }