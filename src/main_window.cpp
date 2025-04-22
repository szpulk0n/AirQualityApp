/**
 * @file main_window.cpp
 * @brief Implementacja głównego okna aplikacji
 */

 #include "main_window.hpp"
 #include <QMessageBox>
 #include <QFileInfo>
 #include <QDir>
 #include <QFuture>
 #include <QHeaderView>
 #include <QtConcurrent/QtConcurrent>
 
 MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
     setWindowTitle("Monitor Jakości Powietrza");
     resize(800, 600);
     
     // Inicjalizacja klienta API
     apiClient = std::make_unique<ApiClient>();
     
     // Inicjalizacja UI
     initUI();
     
     // Wczytanie stacji pomiarowych
     loadStations();
 }
 
 MainWindow::~MainWindow() {
 }
 
 void MainWindow::initUI() {
     // Główny widget i layout
     QWidget *centralWidget = new QWidget(this);
     QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
     
     // Grupa wyboru stacji i czujnika
     QGroupBox *selectionGroup = new QGroupBox("Wybór stacji i parametru", this);
     QVBoxLayout *selectionLayout = new QVBoxLayout(selectionGroup);
     
     // Komponent wyboru stacji
     QHBoxLayout *stationLayout = new QHBoxLayout();
     QLabel *stationLabel = new QLabel("Stacja pomiarowa:", this);
     stationComboBox = new QComboBox(this);
     stationComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
     stationLayout->addWidget(stationLabel);
     stationLayout->addWidget(stationComboBox);
     
     // Komponent wyboru czujnika
     QHBoxLayout *sensorLayout = new QHBoxLayout();
     QLabel *sensorLabel = new QLabel("Parametr:", this);
     sensorComboBox = new QComboBox(this);
     sensorComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
     sensorLayout->addWidget(sensorLabel);
     sensorLayout->addWidget(sensorComboBox);
     
     // Przycisk odświeżania
     refreshButton = new QPushButton("Odśwież dane", this);
     
     // Dodanie elementów do grupy wyboru
     selectionLayout->addLayout(stationLayout);
     selectionLayout->addLayout(sensorLayout);
     selectionLayout->addWidget(refreshButton);
     
     // Tabela danych
     dataTable = new QTableWidget(this);
     dataTable->setColumnCount(2);
     dataTable->setHorizontalHeaderLabels({"Data i czas", "Wartość"});
     dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
     
     // Status
     statusLabel = new QLabel("Gotowy", this);
     statusBar()->addWidget(statusLabel);
     
     // Dodanie elementów do głównego layoutu
     mainLayout->addWidget(selectionGroup);
     mainLayout->addWidget(dataTable);
     
     // Ustawienie widgetu centralnego
     setCentralWidget(centralWidget);
     
     // Połączenia sygnałów i slotów
     connect(stationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onStationSelected);
     connect(sensorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSensorSelected);
     connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshData);
 }
 
 void MainWindow::loadStations() {
     statusLabel->setText("Ładowanie stacji...");
     
     // Sprawdzenie dostępności API
     if (!apiClient->isApiAvailable()) {
         statusLabel->setText("API niedostępne, próba wczytania lokalnych danych");
         
         // Sprawdzenie czy plik z danymi istnieje
         QFileInfo checkFile("data/stations.json");
         if (checkFile.exists() && checkFile.isFile()) {
             stations = apiClient->loadStationsFromFile();
             
             if (stations.empty()) {
                 QMessageBox::warning(this, "Błąd", "Nie udało się wczytać danych z pliku lokalnego");
                 statusLabel->setText("Błąd wczytywania danych");
                 return;
             }
         } else {
             QMessageBox::warning(this, "Błąd", "API niedostępne, a brak lokalnego pliku z danymi");
             statusLabel->setText("Brak danych");
             return;
         }
     } else {
         // Pobranie stacji z API
         stations = apiClient->getAllStations();
         
         if (stations.empty()) {
             QMessageBox::warning(this, "Błąd", "Nie udało się pobrać stacji pomiarowych");
             statusLabel->setText("Błąd pobierania danych");
             return;
         }
         
         // Upewnienie się że katalog istnieje
         QDir().mkpath("data");
         
         // Zapisanie stacji do pliku
         apiClient->saveStationsToFile();
     }
     
     // Wypełnienie ComboBox'a stacjami
     stationComboBox->clear();
     for (const auto& station : stations) {
         QString displayText = QString("%1 (%2, %3)").arg(
             QString::fromStdString(station.name),
             QString::fromStdString(station.city),
             QString::fromStdString(station.province)
         );
         stationComboBox->addItem(displayText);
     }
     
     statusLabel->setText("Gotowy");
 }
 
 void MainWindow::onStationSelected(int index) {
     if (index < 0 || index >= static_cast<int>(stations.size())) {
         return;
     }
     
     // Pobranie ID wybranej stacji
     int stationId = stations[index].id;
     
     statusLabel->setText("Ładowanie czujników...");
     
     // Pobranie czujników dla wybranej stacji
     sensors = apiClient->getSensors(stationId);
     
     if (sensors.empty()) {
         QMessageBox::warning(this, "Błąd", "Nie udało się pobrać czujników dla wybranej stacji");
         statusLabel->setText("Błąd pobierania czujników");
         return;
     }
     
     // Wypełnienie ComboBox'a czujnikami
     sensorComboBox->clear();
     for (const auto& sensor : sensors) {
         QString displayText = QString("%1 (%2)").arg(
             QString::fromStdString(sensor.paramName),
             QString::fromStdString(sensor.paramFormula)
         );
         sensorComboBox->addItem(displayText);
     }
     
     statusLabel->setText("Gotowy");
 }
 
 void MainWindow::onSensorSelected(int index) {
     if (index < 0 || index >= static_cast<int>(sensors.size())) {
         return;
     }
     
     // Pobranie ID wybranego czujnika
     int sensorId = sensors[index].id;
     
     statusLabel->setText("Ładowanie pomiarów...");
     
     // Pobranie pomiarów dla wybranego czujnika
     measurements = apiClient->getMeasurements(sensorId);
     
     if (measurements.empty()) {
         QMessageBox::warning(this, "Informacja", "Brak pomiarów dla wybranego czujnika");
         dataTable->setRowCount(0);
         statusLabel->setText("Brak danych pomiarowych");
         return;
     }
     
     // Wypełnienie tabeli danymi
     fillDataTable();
     
     statusLabel->setText("Gotowy");
 }
 
 void MainWindow::refreshData() {
     // Odświeżenie danych dla aktualnie wybranego czujnika
     int sensorIndex = sensorComboBox->currentIndex();
     if (sensorIndex >= 0 && sensorIndex < static_cast<int>(sensors.size())) {
         onSensorSelected(sensorIndex);
     }
 }
 
 void MainWindow::fillDataTable() {
     // Czyszczenie tabeli
     dataTable->setRowCount(0);
     
     // Ustawienie liczby wierszy
     dataTable->setRowCount(measurements.size());
     
     // Wypełnienie tabeli danymi
     for (size_t i = 0; i < measurements.size(); ++i) {
         const auto& measurement = measurements[i];
         
         // Data i czas
         QTableWidgetItem *dateItem = new QTableWidgetItem(QString::fromStdString(measurement.date));
         dataTable->setItem(i, 0, dateItem);
         
         // Wartość
         QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(measurement.value));
         dataTable->setItem(i, 1, valueItem);
     }
     
     // Sortowanie po dacie (malejąco)
     dataTable->sortItems(0, Qt::DescendingOrder);
 }