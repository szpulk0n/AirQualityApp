/**
 * @file api_client.cpp
 * @brief Implementacja klienta API do pobierania danych o jakości powietrza z GIOŚ
 */

 #include "api_client.hpp"
 #include <curl/curl.h>
 #include <fstream>
 #include <iostream>
 #include <stdexcept>
 #include <unordered_map>
 
 // Definicje kodów kolorów ANSI
 #define COLOR_RESET   "\033[0m"
 #define COLOR_RED     "\033[31m"
 #define COLOR_GREEN   "\033[32m"
 #define COLOR_YELLOW  "\033[33m"
 #define COLOR_BLUE    "\033[34m"
 #define COLOR_MAGENTA "\033[35m"
 #define COLOR_CYAN    "\033[36m"
 
 // Funkcja pomocnicza do zapisywania odpowiedzi z libcurl
 static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp){
     ((std::string*)userp)->append((char*)contents, size * nmemb);
     return size * nmemb;
 }
 
 ApiClient::ApiClient() : verbose(true){
     curl_global_init(CURL_GLOBAL_DEFAULT);
     baseUrl = "http://api.gios.gov.pl/pjp-api/rest";
     if (verbose) std::cout << COLOR_CYAN << "Inicjalizacja API z URL: " << baseUrl << COLOR_RESET << std::endl;
 }
 
 ApiClient::~ApiClient(){
     curl_global_cleanup();
 }

 void ApiClient::setVerbose(bool enabled){
     verbose = enabled;
 }
 
 bool ApiClient::isVerbose() const{
     return verbose;
 }
 
 json ApiClient::makeRequest(const std::string& endpoint){
     // Sprawdź cache
     auto cacheIt = responseCache.find(endpoint);
     if (cacheIt != responseCache.end()) {
         if (verbose) std::cout << COLOR_BLUE << "Uzywam danych z cache dla: " << endpoint << COLOR_RESET << std::endl;
         return cacheIt->second;
     }
     
     CURL* curl;
     CURLcode res;
     std::string readBuffer;
     json responseJson;
 
     curl = curl_easy_init();
     if (!curl) {
         throw std::runtime_error("Blad inicjalizacji libcurl");
     }
 
     std::string url = baseUrl + endpoint;
     if (verbose) std::cout << COLOR_BLUE << "Wykonywanie zapytania do: " << url << COLOR_RESET << std::endl;
     
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
         std::string errorMsg = "Blad podczas wykonywania zapytania: " + std::string(curl_easy_strerror(res));
         std::cerr << COLOR_RED << errorMsg << COLOR_RESET << std::endl;
         curl_easy_cleanup(curl);
         throw std::runtime_error(errorMsg);
     }
     
     // Wyświetlenie odpowiedzi
     if (verbose) std::cout << COLOR_GREEN << "Otrzymana odpowiedz (pierwsze 50 znakow): " << COLOR_RESET 
               << readBuffer.substr(0, 50) << "..." << std::endl;
     
     // Czyszczenie zasobów
     curl_easy_cleanup(curl);
     
     try {
         if (readBuffer.empty()) {
             throw std::runtime_error("Pusta odpowiedz z API");
         }
         responseJson = json::parse(readBuffer);
         
         // Zapisz do cache
         responseCache[endpoint] = responseJson;
         
     } catch (const json::parse_error& e) {
         std::string errorMsg = "Blad parsowania JSON: " + std::string(e.what()) + "\nOdpowiedz: " + readBuffer;
         std::cerr << COLOR_RED << errorMsg << COLOR_RESET << std::endl;
         throw std::runtime_error(errorMsg);
     }
     
     return responseJson;
 }
 
 bool ApiClient::isApiAvailable() {
     try {
         if (verbose) std::cout << COLOR_CYAN << "Sprawdzanie dostepnosci API..." << COLOR_RESET << std::endl;
         
         // Inicjalizacja CURL
         CURL* curl = curl_easy_init();
         if (!curl) {
             std::cerr << COLOR_RED << "Nie mozna zainicjalizowac CURL" << COLOR_RESET << std::endl;
             return false;
         }
         
         // Przygotowanie zapytania
         std::string url = baseUrl + "/station/findAll";
         if (verbose) std::cout << COLOR_CYAN << "Testowanie URL: " << url << COLOR_RESET << std::endl;
         
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
         if (verbose) {
             if (success) {
                 std::cout << COLOR_GREEN << "API dostepne: TAK, kod HTTP: " << http_code << COLOR_RESET << std::endl;
             } else {
                 std::cout << COLOR_RED << "API niedostepne, kod HTTP: " << http_code << COLOR_RESET << std::endl;
             }
         }
         return success;
     } catch (const std::exception& e) {
         std::cerr << COLOR_RED << "Blad podczas sprawdzania dostepnosci API: " << e.what() << COLOR_RESET << std::endl;
         return false;
     }
 }
 
 void ApiClient::clearCache() {
     responseCache.clear();
     cachedStations.clear();
     sensorCache.clear();
     measurementCache.clear();
 }
 
 std::vector<Station> ApiClient::getAllStations() {
     // Jeśli mamy w cache, zwróć od razu
     if (!cachedStations.empty()) {
         if (verbose) std::cout << COLOR_CYAN << "Uzywam zachowanych stacji z cache (" << cachedStations.size() << " stacji)" << COLOR_RESET << std::endl;
         return cachedStations;
     }
     
     std::vector<Station> stations;
     
     try {
         if (verbose) std::cout << COLOR_CYAN << "Proba pobrania stacji pomiarowych..." << COLOR_RESET << std::endl;
         json response = makeRequest("/station/findAll");
         if (verbose) std::cout << COLOR_GREEN << "Otrzymano odpowiedz. Liczba stacji: " << response.size() << COLOR_RESET << std::endl;
         
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
         
         if (verbose) std::cout << COLOR_GREEN << "Przetworzono " << stations.size() << " stacji pomiarowych" << COLOR_RESET << std::endl;
         
         // Zapisz do cache
         cachedStations = stations;
     } catch (const std::exception& e) {
         std::cerr << COLOR_RED << "Blad podczas pobierania stacji: " << e.what() << COLOR_RESET << std::endl;
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
        
        std::ofstream file(filename.empty() ? "../data/stations.json" : filename); // Zmiana ścieżki
        if (!file.is_open()) {
            std::cerr << COLOR_RED << "Nie mozna otworzyc pliku do zapisu: " << filename << COLOR_RESET << std::endl;
            return false;
        }
        
        file << stationsJson.dump(4);
        file.close();
        
        std::cout << COLOR_GREEN << "Zapisano dane stacji do pliku: " << (filename.empty() ? "../data/stations.json" : filename) << COLOR_RESET << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "Blad podczas zapisywania stacji do pliku: " << e.what() << COLOR_RESET << std::endl;
        return false;
    }
}

std::vector<Station> ApiClient::loadStationsFromFile(const std::string& filename) {
    std::vector<Station> stations;
    
    try {
        std::ifstream file(filename.empty() ? "../data/stations.json" : filename); // Zmiana ścieżki
        if (!file.is_open()) {
            std::cerr << COLOR_RED << "Nie mozna otworzyc pliku: " << filename << COLOR_RESET << std::endl;
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
        
        cachedStations = stations;
        
        std::cout << COLOR_GREEN << "Wczytano " << stations.size() << " stacji z pliku: " << (filename.empty() ? "../data/stations.json" : filename) << COLOR_RESET << std::endl;
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "Blad podczas wczytywania stacji z pliku: " << e.what() << COLOR_RESET << std::endl;
    }
    
    return stations;
}
 
 std::vector<Sensor> ApiClient::getSensors(int stationId) {
     // Sprawdź cache
     auto cacheIt = sensorCache.find(stationId);
     if (cacheIt != sensorCache.end()) {
         if (verbose) std::cout << COLOR_CYAN << "Uzywam zachowanych czujnikow z cache dla stacji ID: " << stationId << COLOR_RESET << std::endl;
         return cacheIt->second;
     }
     
     std::vector<Sensor> sensors;
     
     try {
         if (verbose) std::cout << COLOR_CYAN << "Pobieranie czujnikow dla stacji ID: " << stationId << COLOR_RESET << std::endl;
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
         
         // Zapisz do cache
         sensorCache[stationId] = sensors;
         
         if (verbose) std::cout << COLOR_GREEN << "Znaleziono " << sensors.size() << " czujnikow" << COLOR_RESET << std::endl;
     } catch (const std::exception& e) {
         std::cerr << COLOR_RED << "Blad podczas pobierania czujnikow: " << e.what() << COLOR_RESET << std::endl;
     }
     
     return sensors;
 }
 
 std::vector<Measurement> ApiClient::getMeasurements(int sensorId) {
     // Sprawdź cache
     auto cacheIt = measurementCache.find(sensorId);
     if (cacheIt != measurementCache.end()) {
         if (verbose) std::cout << COLOR_CYAN << "Uzywam zachowanych pomiarow z cache dla czujnika ID: " << sensorId << COLOR_RESET << std::endl;
         return cacheIt->second;
     }
     
     std::vector<Measurement> measurements;
     
     try {
         if (verbose) std::cout << COLOR_CYAN << "Pobieranie pomiarow dla czujnika ID: " << sensorId << COLOR_RESET << std::endl;
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
                         std::cerr << COLOR_RED << "Blad konwersji wartosci: " << e.what() << COLOR_RESET << std::endl;
                     }
                 }
             }
         }
         
         // Zapisz do cache
         measurementCache[sensorId] = measurements;
         
         if (verbose) std::cout << COLOR_GREEN << "Znaleziono " << measurements.size() << " pomiarow" << COLOR_RESET << std::endl;
     } catch (const std::exception& e) {
         std::cerr << COLOR_RED << "Blad podczas pobierania pomiarow: " << e.what() << COLOR_RESET << std::endl;
     }
     
     return measurements;
 }