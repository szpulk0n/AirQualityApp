/**
 * @file api_client.hpp
 * @brief Klasa klienta API do pobierania danych o jakości powietrza z GIOŚ
 */

 #ifndef API_CLIENT_HPP
 #define API_CLIENT_HPP
 
 #include <string>
 #include <vector>
 #include <memory>
 #include <functional>
 #include <nlohmann/json.hpp>
 
 using json = nlohmann::json;
 
 /**
  * @brief Struktura reprezentująca stację pomiarową
  */
 struct Station {
     int id;
     std::string name;
     double lat;
     double lon;
     std::string city;
     std::string address;
     std::string province;
 };
 
 /**
  * @brief Struktura reprezentująca czujnik pomiarowy
  */
 struct Sensor {
     int id;
     int stationId;
     std::string paramName;
     std::string paramFormula;
     std::string paramCode;
     int paramId;
 };
 
 /**
  * @brief Struktura reprezentująca pomiar
  */
 struct Measurement {
     std::string date;
     double value;
 };
 
 /**
  * @brief Klasa klienta API GIOŚ
  */
 class ApiClient {
 public:
     /**
      * @brief Konstruktor
      */
     ApiClient();
     
     /**
      * @brief Destruktor
      */
     ~ApiClient();
     
     /**
      * @brief Pobiera listę wszystkich stacji pomiarowych
      * @return Wektor struktur Station
      */
     std::vector<Station> getAllStations();
     
     /**
      * @brief Zapisuje stacje do pliku JSON
      * @param filename Nazwa pliku
      * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
      */
     bool saveStationsToFile(const std::string& filename = "data/stations.json");
     
     /**
      * @brief Wczytuje stacje z pliku JSON
      * @param filename Nazwa pliku
      * @return Wektor struktur Station
      */
     std::vector<Station> loadStationsFromFile(const std::string& filename = "data/stations.json");
     
     /**
      * @brief Pobiera listę czujników dla danej stacji
      * @param stationId ID stacji
      * @return Wektor struktur Sensor
      */
     std::vector<Sensor> getSensors(int stationId);
     
     /**
      * @brief Pobiera dane pomiarowe dla danego czujnika
      * @param sensorId ID czujnika
      * @return Wektor struktur Measurement
      */
     std::vector<Measurement> getMeasurements(int sensorId);
     
     /**
      * @brief Sprawdza czy połączenie z API jest dostępne
      * @return true jeśli API jest dostępne, false w przeciwnym wypadku
      */
     bool isApiAvailable();
 
 private:
     const std::string baseUrl = "http://api.gios.gov.pl/pjp-api/rest";
     
     /**
      * @brief Funkcja pomocnicza do wykonywania zapytań HTTP
      * @param endpoint Endpoint API
      * @return Odpowiedź w formacie JSON
      */
     json makeRequest(const std::string& endpoint);
 };
 
 #endif // API_CLIENT_HPP