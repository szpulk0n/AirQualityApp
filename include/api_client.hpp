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
#include <unordered_map>
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
    bool saveStationsToFile(const std::string& filename = "../data/stations.json");
    
    /**
     * @brief Wczytuje stacje z pliku JSON
     * @param filename Nazwa pliku
     * @return Wektor struktur Station
     */
    std::vector<Station> loadStationsFromFile(const std::string& filename = "../data/stations.json");
    
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
    
    /**
     * @brief Włącza lub wyłącza tryb gadatliwy (wyświetlanie komunikatów)
     * @param enabled Wartość true włącza komunikaty, false wyłącza
     */
    void setVerbose(bool enabled);
    
    /**
     * @brief Sprawdza, czy tryb gadatliwy jest włączony
     * @return true jeśli komunikaty są włączone, false w przeciwnym przypadku
     */
    bool isVerbose() const;
    
    /**
     * @brief Czyści wszystkie cache
     */
    void clearCache();

private:
    std::string baseUrl;
    bool verbose; // Flaga określająca tryb wyświetlania komunikatów
    
    // Mechanizmy cachowania
    std::unordered_map<std::string, json> responseCache; // Cache dla odpowiedzi z API
    std::vector<Station> cachedStations; // Cache dla stacji
    std::unordered_map<int, std::vector<Sensor>> sensorCache; // Cache dla czujników (klucz: ID stacji)
    std::unordered_map<int, std::vector<Measurement>> measurementCache; // Cache dla pomiarów (klucz: ID czujnika)
    
    /**
     * @brief Funkcja pomocnicza do wykonywania zapytań HTTP
     * @param endpoint Endpoint API
     * @return Odpowiedź w formacie JSON
     */
    json makeRequest(const std::string& endpoint);
};

#endif // API_CLIENT_HPP