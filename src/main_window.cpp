/**
 * @file main_window.cpp
 * @brief Implementacja głównego okna aplikacji
 */

 #include "main_window.hpp"
 #include <QMessageBox>
 #include <QFileInfo>
 #include <QDir>
 #include <QFuture>
 #include <QSplitter>
 #include <QDateTime>
 #include <QTabWidget>
 #include <QtConcurrent/QtConcurrent>
 #include <algorithm>
 #include <iostream>
 
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
     
     // Filtry dat
     QHBoxLayout *dateFilterLayout = new QHBoxLayout();
     QLabel *startDateLabel = new QLabel("Data początkowa:", this);
     startDateEdit = new QDateEdit(QDate::currentDate().addDays(-7), this);
     startDateEdit->setCalendarPopup(true);
     QLabel *endDateLabel = new QLabel("Data końcowa:", this);
     endDateEdit = new QDateEdit(QDate::currentDate(), this);
     endDateEdit->setCalendarPopup(true);
     filterButton = new QPushButton("Filtruj", this);
     
     dateFilterLayout->addWidget(startDateLabel);
     dateFilterLayout->addWidget(startDateEdit);
     dateFilterLayout->addWidget(endDateLabel);
     dateFilterLayout->addWidget(endDateEdit);
     dateFilterLayout->addWidget(filterButton);
     
     // Przyciski
     QHBoxLayout *buttonLayout = new QHBoxLayout();
     refreshButton = new QPushButton("Odśwież dane", this);
     showChartButton = new QPushButton("Pokaż wykres", this);
     showChartButton->setEnabled(false);
     
     buttonLayout->addWidget(refreshButton);
     buttonLayout->addWidget(showChartButton);
     
     // Dodanie elementów do grupy wyboru
     selectionLayout->addLayout(stationLayout);
     selectionLayout->addLayout(sensorLayout);
     selectionLayout->addLayout(dateFilterLayout);
     selectionLayout->addLayout(buttonLayout);
     
     // Tabwidget dla tabeli i wykresu
     QTabWidget *tabWidget = new QTabWidget(this);
     
     // Tabela danych
     dataTable = new QTableWidget(this);
     dataTable->setColumnCount(2);
     dataTable->setHorizontalHeaderLabels({"Data i czas", "Wartość"});
     dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
     
     // Wykres
     QChart *chart = new QChart();
     chart->setTitle("Pomiary");
     chart->legend()->hide();
     
     chartView = new QChartView(chart, this);
     chartView->setRenderHint(QPainter::Antialiasing);
     
     // Dodanie zakładek
     tabWidget->addTab(dataTable, "Dane tabelaryczne");
     tabWidget->addTab(chartView, "Wykres");
     
     // Status
     statusLabel = new QLabel("Gotowy", this);
     statusBar()->addWidget(statusLabel);
     
     // Dodanie elementów do głównego layoutu
     mainLayout->addWidget(selectionGroup);
     mainLayout->addWidget(tabWidget);
     
     // Ustawienie widgetu centralnego
     setCentralWidget(centralWidget);
     
     // Połączenia sygnałów i slotów
     connect(stationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onStationSelected);
     connect(sensorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSensorSelected);
     connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshData);
     connect(showChartButton, &QPushButton::clicked, this, &MainWindow::showChart);
     connect(filterButton, &QPushButton::clicked, this, &MainWindow::onFilterClicked);
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
         showChartButton->setEnabled(false);
         return;
     }
     
     // Wypełnienie tabeli danymi
     fillDataTable();
     
     // Aktywacja przycisku wykresu
     showChartButton->setEnabled(true);
     
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
         QDateTime dateTime = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
         QTableWidgetItem *dateItem = new QTableWidgetItem(dateTime.toString("dd.MM.yyyy hh:mm"));
         dataTable->setItem(i, 0, dateItem);
         
         // Wartość
         QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(measurement.value, 'f', 2));
         dataTable->setItem(i, 1, valueItem);
     }
     
     // Sortowanie po dacie (malejąco)
     dataTable->sortItems(0, Qt::DescendingOrder);
     
     // Zaktualizuj zakres dat w kontrolkach
     if (!measurements.empty()) {
         // Znajdź najstarszą i najnowszą datę
         QDateTime oldestDate = QDateTime::currentDateTime();
         QDateTime newestDate = QDateTime::fromMSecsSinceEpoch(0);
         
         for (const auto& measurement : measurements) {
             QDateTime date = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
             if (date < oldestDate) {
                 oldestDate = date;
             }
             if (date > newestDate) {
                 newestDate = date;
             }
         }
         
         // Ustaw zakres dat w kontrolkach
         startDateEdit->setDateTime(oldestDate);
         endDateEdit->setDateTime(newestDate);
     }
 }
 
 void MainWindow::createChart() {
     if (measurements.empty()) {
         QMessageBox::information(this, "Informacja", "Brak danych do wyświetlenia na wykresie");
         return;
     }
     
     // Pobranie nazwy parametru
     int sensorIndex = sensorComboBox->currentIndex();
     if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensors.size())) {
         QMessageBox::warning(this, "Błąd", "Nie wybrano parametru");
         return;
     }
     
     // Sprawdź czy chartView jest poprawnie zainicjalizowany
     if (!chartView) {
         QMessageBox::critical(this, "Błąd", "Błąd komponentu wykresu");
         return;
     }
     
     QString paramName = QString::fromStdString(sensors[sensorIndex].paramName);
     QString paramFormula = QString::fromStdString(sensors[sensorIndex].paramFormula);
     
     // Utwórz wykres
     QChart *chart = new QChart();
     
     try {
         // Debug info
         std::cout << "Tworzenie wykresu dla parametru: " << sensors[sensorIndex].paramName 
                   << " (" << sensors[sensorIndex].paramFormula << ")" << std::endl;
         std::cout << "Liczba pomiarów: " << measurements.size() << std::endl;
         
         // Utworzenie serii danych
         QLineSeries *series = new QLineSeries();
         series->setName(paramName);
         
         // Wartości min i max do ustalenia zakresu osi Y
         double minValue = std::numeric_limits<double>::max();
         double maxValue = std::numeric_limits<double>::lowest();
         qint64 minTime = std::numeric_limits<qint64>::max();
         qint64 maxTime = std::numeric_limits<qint64>::lowest();
         
         // Wypełnienie serii danymi
         for (const auto& measurement : measurements) {
             // Konwersja daty na timestamp
             QDateTime dateTime = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
             qint64 timestamp = dateTime.toMSecsSinceEpoch();
             
             // Dodanie punktu do serii
             series->append(timestamp, measurement.value);
             
             // Debug info dla pierwszych 5 punktów
             if (series->count() <= 5) {
                 std::cout << "Punkt " << series->count() << ": " 
                           << dateTime.toString("yyyy-MM-dd hh:mm").toStdString() 
                           << ", wartość: " << measurement.value << std::endl;
             }
             
             // Aktualizacja wartości min/max
             minValue = std::min(minValue, measurement.value);
             maxValue = std::max(maxValue, measurement.value);
             minTime = std::min(minTime, timestamp);
             maxTime = std::max(maxTime, timestamp);
         }
         
         if (series->count() == 0) {
             QMessageBox::warning(this, "Błąd", "Nie udało się utworzyć punktów wykresu");
             delete series;
             delete chart;
             return;
         }
         
         // Dodanie marginesu do zakresu wartości
         double margin = (maxValue - minValue) * 0.1;
         if (margin < 0.001) margin = 0.1; // Minimalny margines
         minValue = minValue - margin;
         maxValue = maxValue + margin;
         
         // Dodanie serii do wykresu
         chart->addSeries(series);
         chart->setTitle(QString("Pomiary: %1 (%2)").arg(paramName).arg(paramFormula));
         
         // Osie wykresu
         QDateTimeAxis *axisX = new QDateTimeAxis;
         axisX->setTickCount(10);
         axisX->setFormat("dd.MM.yyyy\nhh:mm");
         axisX->setTitleText("Data i czas");
         
         QValueAxis *axisY = new QValueAxis;
         axisY->setLabelFormat("%.2f");
         axisY->setTitleText(paramFormula);
         axisY->setRange(minValue, maxValue);
         
         chart->addAxis(axisX, Qt::AlignBottom);
         chart->addAxis(axisY, Qt::AlignLeft);
         series->attachAxis(axisX);
         series->attachAxis(axisY);
         
         // Ustawienie zakresu czasu
         axisX->setRange(QDateTime::fromMSecsSinceEpoch(minTime), QDateTime::fromMSecsSinceEpoch(maxTime));
         
         // Ustawienie nowego wykresu w widoku
         chartView->setChart(chart);
         
         // Animacja
         chart->setAnimationOptions(QChart::SeriesAnimations);
         
         std::cout << "Wykres utworzony pomyślnie" << std::endl;
     }
     catch (const std::exception& e) {
         QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd podczas tworzenia wykresu: %1").arg(e.what()));
         delete chart;
     }
 }
 
 void MainWindow::showChart() {
     createChart();
     
     // Przełącz na zakładkę z wykresem
     // Znajdź QTabWidget i przełącz na zakładkę z wykresem (indeks 1)
     QList<QTabWidget*> tabWidgets = findChildren<QTabWidget*>();
     if (!tabWidgets.isEmpty()) {
         tabWidgets.first()->setCurrentIndex(1);
     }
     
     // Wyświetl informację w pasku statusu
     statusLabel->setText("Wykres został zaktualizowany");
 }
 
 void MainWindow::filterData() {
     if (measurements.empty()) {
         return;
     }
     
     QDateTime startDate = startDateEdit->dateTime();
     QDateTime endDate = endDateEdit->dateTime();
     
     // Filtrowanie pomiarów
     std::vector<Measurement> filteredMeasurements;
     
     for (const auto& measurement : measurements) {
         QDateTime measurementDate = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
         
         if (measurementDate >= startDate && measurementDate <= endDate) {
             filteredMeasurements.push_back(measurement);
         }
     }
     
     // Aktualizacja tabeli i wykresu
     if (filteredMeasurements.empty()) {
         QMessageBox::information(this, "Informacja", "Brak danych w wybranym zakresie dat");
         return;
     }
     
     // Tymczasowo zastąp measurements przefiltrowanymi danymi
     std::vector<Measurement> originalMeasurements = measurements;
     measurements = filteredMeasurements;
     
     // Aktualizacja tabeli
     fillDataTable();
     
     // Aktualizacja wykresu
     if (chartView->chart()->series().size() > 0) {
         createChart();
     }
     
     // Przywróć oryginalne dane
     measurements = originalMeasurements;
 }
 
 void MainWindow::onFilterClicked() {
     filterData();
 }