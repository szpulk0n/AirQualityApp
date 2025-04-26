/**
 * @file main_window.hpp
 * @brief Główne okno aplikacji do monitorowania jakości powietrza
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
#include <memory>
#include <map>
#include <QColor>

QT_CHARTS_USE_NAMESPACE

#include "api_client.hpp"

/**
 * @brief Klasa reprezentująca główne okno aplikacji
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor
     * @param parent Wskaźnik na rodzica (domyślnie nullptr)
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destruktor
     */
    ~MainWindow();

private slots:
    /**
     * @brief Wczytuje stacje pomiarowe
     */
    void loadStations();
    
    /**
     * @brief Slot wywoływany po wybraniu stacji
     * @param index Indeks wybranej stacji
     */
    void onStationSelected(int index);
    
    /**
     * @brief Slot wywoływany po wybraniu czujnika
     * @param index Indeks wybranego czujnika
     */
    void onSensorSelected(int index);
    
    /**
     * @brief Odświeża dane pomiarowe
     */
    void refreshData();
  
    /**
     * @brief Zapisuje dane pomiarowe do pliku JSON
     */
    void saveMeasurements();
    
    /**
     * @brief Zapisuje dane pomiarowe do pliku JSON
     * @param filename Nazwa pliku
     * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
     */
    bool saveMeasurementsToJSON(const QString& filename);
    
    /**
     * @brief Otwiera panel z zapisanymi pomiarami
     */
    void openSavedMeasurements();
    
    /**
     * @brief Wczytuje zapisane pomiary z wybranego pliku
     * @param item Element z listy plików
     */
    void loadSavedMeasurement(QListWidgetItem* item);

private:
    // Komponenty UI
    QComboBox *stationComboBox;
    QComboBox *sensorComboBox;
    QPushButton *refreshButton;
    QPushButton *saveButton;
    QPushButton *openSavedButton;
    QTableWidget *dataTable;
    QLabel *statusLabel;
    QChartView *chartView;
    
    // Dane
    std::unique_ptr<ApiClient> apiClient;
    std::vector<Station> stations;
    std::vector<Sensor> sensors;
    std::vector<Measurement> measurements;
    std::map<int, QColor> sensorColors; // Mapa ID czujnika na kolor
    
    // Ścieżka do zapisu pomiarów
    QString exportPath;
    
    /**
     * @brief Inicjalizuje interfejs użytkownika
     */
    void initUI();
    
    /**
     * @brief Wypełnia tabelę danymi pomiarowymi
     */
    void fillDataTable();
    
    /**
     * @brief Tworzy i wyświetla wykres na podstawie danych pomiarowych
     * @param paramName Nazwa parametru
     * @param paramFormula Wzór parametru
     * @param switchToChartTab Czy przełączyć się na zakładkę z wykresem
     * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
     */
    bool displayChart(const QString& paramName, const QString& paramFormula, bool switchToChartTab = false);
    
    /**
     * @brief Tworzy dialog do przeglądania zapisanych pomiarów
     * @return Dialog z listą zapisanych plików
     */
    QDialog* createSavedMeasurementsDialog();
    
    /**
     * @brief Wczytuje pomiary z pliku JSON
     * @param filePath Ścieżka do pliku
     * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
     */
    bool loadMeasurementsFromJSON(const QString& filePath);
};

#endif // MAIN_WINDOW_HPP