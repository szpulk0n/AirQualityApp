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
 #include <memory>
 
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
      * @brief Wyświetla wykres danych
      */
     void showChart();

     /**
      * @brief Zapisuje dane pomiarowe do pliku
      */
     void saveMeasurements();

     /**
      * @brief Zapisuje dane pomiarowe do pliku CSV
      * @param filename Nazwa pliku
      * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
      */
     bool saveMeasurementsToCSV(const QString& filename);

     /**
      * @brief Zapisuje dane pomiarowe do pliku JSON
      * @param filename Nazwa pliku
      * @return true jeśli operacja się powiodła, false w przeciwnym wypadku
      */
     bool saveMeasurementsToJSON(const QString& filename);
 
 private:
     // Komponenty UI
     QComboBox *stationComboBox;
     QComboBox *sensorComboBox;
     QPushButton *refreshButton;
     QPushButton *saveButton;
     QTableWidget *dataTable;
     QLabel *statusLabel;
     QChartView *chartView;
     QPushButton *showChartButton;
     
     // Dane
     std::unique_ptr<ApiClient> apiClient;
     std::vector<Station> stations;
     std::vector<Sensor> sensors;
     std::vector<Measurement> measurements;
     
     /**
      * @brief Inicjalizuje interfejs użytkownika
      */
     void initUI();
     
     /**
      * @brief Wypełnia tabelę danymi pomiarowymi
      */
     void fillDataTable();
     
     /**
      * @brief Tworzy wykres z danych pomiarowych
      */
     void createChart();
 };
 
 #endif // MAIN_WINDOW_HPP