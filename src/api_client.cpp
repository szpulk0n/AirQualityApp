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
     
     // Spróbujmy znaleźć działający URL API
     for (const auto& url : apiUrls) {
         std::cout << "Testowanie API URL: " << url << std::endl;
         baseUrl = url;
         
         if (isApiAvailable()) {
             std::cout << "Znaleziono działający URL API: " << url << std::endl;
             return;
         }
     }
     
     // Jeśli żaden URL nie działa, użyjmy domyślnego
     baseUrl = "http://api.gios.gov.pl/pjp-api/rest";
     std::cout << "Żaden URL API nie działa. Ustawiono domyślny: " << baseUrl << std::endl;
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
     std::cout << "Wykonywanie zapytania do: " << url << std::endl;
     
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
         std::string errorMsg = "Błąd podczas wykonywania zapytania: " + std::string(curl_easy_strerror(res));
         std::cerr << errorMsg << std::endl;
         curl_easy_cleanup(curl);
         throw std::runtime_error(errorMsg);
     }
     
     // Wyświetlenie odpowiedzi
     std::cout << "Otrzymana odpowiedź (pierwsze 200 znaków): " << readBuffer.substr(0, 200) << "..." << std::endl;
     
     // Czyszczenie zasobów
     curl_easy_cleanup(curl);
     
     try {
         if (readBuffer.empty()) {
             throw std::runtime_error("Pusta odpowiedź z API");
         }
         responseJson = json::parse(readBuffer);
     } catch (const json::parse_error& e) {
         std::string errorMsg = "Błąd parsowania JSON: " + std::string(e.what()) + "\nOdpowiedź: " + readBuffer;
         std::cerr << errorMsg << std::endl;
         throw std::runtime_error(errorMsg);
     }
     
     return responseJson;
 }
 
 bool ApiClient::isApiAvailable() {
     try {
         std::cout << "Sprawdzanie dostępności API..." << std::endl;
         
         // Inicjalizacja CURL
         CURL* curl = curl_easy_init();
         if (!curl) {
             std::cerr << "Nie można zainicjalizować CURL" << std::endl;
             return false;
         }
         
         // Przygotowanie zapytania
         std::string url = baseUrl + "/station/findAll";
         std::cout << "Testowanie URL: " << url << std::endl;
         
         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
         curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // Tylko nagłówek odpowiedzi
         curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // 5 sekund timeout
         curl_easy_setopt(curl, CURLOPT_USERAGENT, "AirQualityApp/1.0");
         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
         
         // Wykonanie zapytania
         CURLcode res = curl_easy_perform(curl);
         
         // Sprawdzenie kodu odpowiedzi HTTP
         long http_code = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
         
         // Zwolnienie zasobów
         curl_easy_cleanup(curl);
         
         // Sprawdzenie wyniku
         bool success = (res == CURLE_OK && http_code == 200);
         std::cout << "API dostępne: " << (success ? "TAK" : "NIE") << ", kod HTTP: " << http_code << std::endl;
         return success;
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas sprawdzania dostępności API: " << e.what() << std::endl;
         return false;
     }
 }
 
 std::vector<Station> ApiClient::getAllStations() {
     std::vector<Station> stations;
     
     try {
         std::cout << "Próba pobrania stacji pomiarowych..." << std::endl;
         json response = makeRequest("/station/findAll");
         std::cout << "Otrzymano odpowiedź. Liczba stacji: " << response.size() << std::endl;
         
         for (const auto& item : response) {
             Station station;
             
             // Konwersja id na liczbę
             station.id = item["id"].get<int>();
             station.name = item["stationName"].get<std::string>();
             
             // Konwersja współrzędnych geograficznych ze stringów na liczby
             if (!item["gegrLat"].is_null()) {
                 station.lat = std::stod(item["gegrLat"].get<std::string>());
             }
             if (!item["gegrLon"].is_null()) {
                 station.lon = std::stod(item["gegrLon"].get<std::string>());
             }
             
             // Pobieranie informacji o mieście i adresie
             if (!item["city"].is_null()) {
                 station.city = item["city"]["name"].get<std::string>();
                 
                 if (!item["city"]["commune"].is_null()) {
                     station.province = item["city"]["commune"]["provinceName"].get<std::string>();
                 }
             }
             
             if (!item["addressStreet"].is_null()) {
                 station.address = item["addressStreet"].get<std::string>();
             }
             
             stations.push_back(station);
         }
         
         std::cout << "Przetworzono " << stations.size() << " stacji pomiarowych" << std::endl;
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
             sensor.id = item["id"].get<int>();
             sensor.stationId = item["stationId"].get<int>();
             
             if (!item["param"].is_null()) {
                 sensor.paramName = item["param"]["paramName"].get<std::string>();
                 sensor.paramFormula = item["param"]["paramFormula"].get<std::string>();
                 sensor.paramCode = item["param"]["paramCode"].get<std::string>();
                 sensor.paramId = item["param"]["idParam"].get<int>();
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
                     measurement.date = item["date"].get<std::string>();
                     
                     // Konwersja wartości ze stringa na liczbę
                     try {
                         if (item["value"].is_string()) {
                             measurement.value = std::stod(item["value"].get<std::string>());
                         } else {
                             measurement.value = item["value"].get<double>();
                         }
                         measurements.push_back(measurement);
                     } catch (const std::exception& e) {
                         std::cerr << "Błąd konwersji wartości: " << e.what() << std::endl;
                     }
                 }
             }
         }
     } catch (const std::exception& e) {
         std::cerr << "Błąd podczas pobierania pomiarów: " << e.what() << std::endl;
     }
     
     return measurements;
 }