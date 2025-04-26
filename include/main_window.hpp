/**
 * @file main_window.hpp
 * @brief Definicja klasy głównego okna aplikacji do monitorowania jakości powietrza
 */
#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP
#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QThread>
#include <QDateEdit>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QListWidget>
#include <QDialog>
#include <QFutureWatcher>
#include <memory>
#include <map>
#include <QColor>

QT_CHARTS_USE_NAMESPACE

#include "api_client.hpp"

/**
 * @brief Klasa reprezentująca główne okno aplikacji do monitorowania jakości powietrza
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor głównego okna
     * @param parent Wskaźnik na rodzica (domyślnie nullptr)
     * Inicjalizuje główne okno aplikacji, ustawia klienta API i podstawowe parametry.
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destruktor
     * Zwalnia zasoby używane przez główne okno.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Wczytuje listę stacji pomiarowych
     * Pobiera stacje z API lub lokalnego pliku i wypełnia ComboBox stacjami.
     */
    void loadStations();
    
    /**
     * @brief Obsługuje wybór stacji z ComboBox
     * @param index Indeks wybranej stacji
     * Pobiera czujniki dla wybranej stacji i aktualizuje ComboBox czujników.
     */
    void onStationSelected(int index);
    
    /**
     * @brief Obsługuje wybór czujnika z ComboBox
     * @param index Indeks wybranego czujnika
     * Pobiera pomiary dla wybranego czujnika (lub wszystkich, jeśli wybrano "Wszystkie") i wyświetla dane.
     */
    void onSensorSelected(int index);
    
    /**
     * @brief Odświeża dane pomiarowe
     * Ponownie pobiera pomiary dla aktualnie wybranego czujnika i aktualizuje widok.
     */
    void refreshData();
  
    /**
     * @brief Zapisuje pomiary do pliku JSON
     * Wywołuje funkcję zapisu danych pomiarowych do pliku w formacie JSON.
     */
    void saveMeasurements();
    
    /**
     * @brief Zapisuje pomiary do pliku JSON
     * @param filename Nazwa pliku
     * @return true jeśli zapis się powiódł, false w przeciwnym razie
     * Zapisuje dane pomiarowe, metadane stacji i czujnika oraz kolory do pliku JSON.
     */
    bool saveMeasurementsToJSON(const QString& filename);
    
    /**
     * @brief Otwiera okno z zapisanymi pomiarami
     * Wyświetla dialog z listą zapisanych plików JSON do wczytania.
     */
    void openSavedMeasurements();
    
    /**
     * @brief Wczytuje zapisane pomiary z wybranego pliku
     * @param item Element z listy plików
     * Wczytuje dane z wybranego pliku JSON i aktualizuje interfejs.
     */
    void loadSavedMeasurement(QListWidgetItem* item);

    /**
     * @brief Obsługuje zakończenie asynchronicznego wczytywania stacji
     * Aktualizuje interfejs po zakończeniu operacji pobierania stacji.
     */
    void onStationsLoaded();

private:
    // Komponenty interfejsu użytkownika
    QComboBox *stationComboBox;      ///< ComboBox do wyboru stacji
    QComboBox *sensorComboBox;       ///< ComboBox do wyboru czujnika
    QPushButton *refreshButton;      ///< Przycisk odświeżania danych
    QPushButton *saveButton;         ///< Przycisk zapisu danych
    QPushButton *openSavedButton;    ///< Przycisk otwierania zapisanych danych
    QTableWidget *dataTable;         ///< Tabela wyświetlająca pomiary
    QLabel *statusLabel;             ///< Etykieta statusu w pasku stanu
    QChartView *chartView;           ///< Widok wykresu z pomiarami
    
    // Dane aplikacji
    std::unique_ptr<ApiClient> apiClient;       ///< Klient API do pobierania danych
    std::vector<Station> stations;              ///< Lista stacji pomiarowych
    std::vector<Sensor> sensors;                ///< Lista czujników dla wybranej stacji
    std::vector<Measurement> measurements;      ///< Lista pomiarów dla wybranego czujnika
    std::map<int, QColor> sensorColors;         ///< Mapa ID czujnika na kolor wykresu
    
    // Ścieżka do zapisu pomiarów
    QString exportPath;                         ///< Katalog do zapisu plików JSON
    
    // Obiekt do śledzenia asynchronicznych operacji
    QFutureWatcher<std::vector<Station>> stationsWatcher; ///< Obserwator dla asynchronicznego wczytywania stacji
    
    /**
     * @brief Inicjalizuje interfejs użytkownika
     * Tworzy i konfiguruje wszystkie elementy UI, ustala połączenia sygnałów i slotów.
     */
    void initUI();
    
    /**
     * @brief Wypełnia tabelę danymi pomiarowymi
     * Aktualizuje tabelę dataTable danymi z wektora measurements.
     */
    void fillDataTable();
    
    /**
     * @brief Tworzy i wyświetla wykres pomiarów
     * @param paramName Nazwa parametru
     * @param paramFormula Wzór parametru
     * @param switchToChartTab Czy przełączyć na zakładkę z wykresem
     * @return true jeśli wykres został utworzony, false w przeciwnym razie
     * Generuje wykres dla pojedynczego parametru lub wszystkich parametrów.
     */
    bool displayChart(const QString& paramName, const QString& paramFormula, bool switchToChartTab = false);
    
    /**
     * @brief Tworzy dialog do przeglądania zapisanych pomiarów
     * @return Wskaźnik na dialog z listą zapisanych plików
     * Tworzy okno dialogowe z listą plików JSON w katalogu exportPath.
     */
    QDialog* createSavedMeasurementsDialog();
    
    /**
     * @brief Wczytuje pomiary z pliku JSON
     * @param filePath Ścieżka do pliku
     * @return true jeśli wczytanie się powiodło, false w przeciwnym razie
     * Parsuje plik JSON i aktualizuje dane aplikacji (stacje, czujniki, pomiary, kolory).
     */
    bool loadMeasurementsFromJSON(const QString& filePath);
};

#endif // MAIN_WINDOW_HPP