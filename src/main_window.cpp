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
 #include <QFileDialog>
 #include <QDialog>
 #include <QVBoxLayout>
 #include <QListWidget>
 #include <QPushButton>
 #include <QLabel>
 #include <fstream>
 #include <algorithm>
 #include <iostream>
 #include <QColor>
 
 MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
     setWindowTitle("Monitor Jakości Powietrza");
     resize(800, 600);
     
     // Inicjalizacja klienta API
     apiClient = std::make_unique<ApiClient>();
     
     // Ustawienie ścieżki eksportu na główny folder aplikacji
     exportPath = "../export";
     
     // Upewnij się, że katalog eksportu istnieje
     QDir().mkpath(exportPath);
     
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
     sensorComboBox->addItem("Wszystkie");
     sensorLayout->addWidget(sensorLabel);
     sensorLayout->addWidget(sensorComboBox);
     
     // Przyciski
     QHBoxLayout *buttonLayout = new QHBoxLayout();
     refreshButton = new QPushButton("Odśwież dane", this);
     saveButton = new QPushButton("Zapisz dane", this);
     openSavedButton = new QPushButton("Przeglądaj zapisane dane", this);
     saveButton->setEnabled(false);
     
     buttonLayout->addWidget(refreshButton);
     buttonLayout->addWidget(saveButton);
     buttonLayout->addWidget(openSavedButton);
     
     // Dodanie elementów do grupy wyboru
     selectionLayout->addLayout(stationLayout);
     selectionLayout->addLayout(sensorLayout);
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
     connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveMeasurements);
     connect(openSavedButton, &QPushButton::clicked, this, &MainWindow::openSavedMeasurements);
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
     
     // Przydzielanie kolorów czujnikom
     sensorColors.clear();
     QList<QColor> colors = {
         Qt::blue, Qt::red, Qt::green, Qt::magenta, Qt::cyan,
         Qt::darkYellow, Qt::darkCyan, Qt::darkMagenta
     };
     for (size_t i = 0; i < sensors.size(); ++i) {
         sensorColors[sensors[i].id] = colors[i % colors.size()];
     }
     
     // Wypełnienie ComboBox'a czujnikami
     sensorComboBox->clear();
     sensorComboBox->addItem("Wszystkie");
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
     if (index < 0 || index >= static_cast<int>(sensors.size()) + 1) {
         return;
     }
     
     statusLabel->setText("Ładowanie pomiarów...");
     
     if (index == 0) { // "Wszystkie" wybrano
         measurements.clear();
         bool anyMeasurements = false;
         
         // Pobierz pomiary dla wszystkich czujników
         for (const auto& sensor : sensors) {
             auto sensorMeasurements = apiClient->getMeasurements(sensor.id);
             if (!sensorMeasurements.empty()) {
                 measurements.insert(measurements.end(), sensorMeasurements.begin(), sensorMeasurements.end());
                 anyMeasurements = true;
             }
         }
         
         if (!anyMeasurements) {
             QMessageBox::warning(this, "Informacja", "Brak pomiarów dla wszystkich czujników");
             dataTable->setRowCount(0);
             statusLabel->setText("Brak danych pomiarowych");
             saveButton->setEnabled(false);
             return;
         }
         
         // Wypełnienie tabeli danymi
         fillDataTable();
         
         // Wyświetl wykres dla wszystkich parametrów
         displayChart("Wszystkie parametry", "", false);
     } else {
         // Pobranie ID wybranego czujnika
         int sensorId = sensors[index - 1].id;
         
         // Pobranie pomiarów dla wybranego czujnika
         measurements = apiClient->getMeasurements(sensorId);
         
         if (measurements.empty()) {
             QMessageBox::warning(this, "Informacja", "Brak pomiarów dla wybranego czujnika");
             dataTable->setRowCount(0);
             statusLabel->setText("Brak danych pomiarowych");
             saveButton->setEnabled(false);
             return;
         }
         
         // Wypełnienie tabeli danymi
         fillDataTable();
         
         // Pobierz informacje o wybranym czujniku i stwórz wykres
         QString paramName = QString::fromUtf8(sensors[index - 1].paramName.c_str());
         QString paramFormula = QString::fromUtf8(sensors[index - 1].paramFormula.c_str());
         displayChart(paramName, paramFormula, false);
     }
     
     // Aktywacja przycisków
     saveButton->setEnabled(true);
     
     statusLabel->setText("Gotowy");
 }
 
 void MainWindow::refreshData() {
     // Odświeżenie danych dla aktualnie wybranego czujnika
     int sensorIndex = sensorComboBox->currentIndex();
     if (sensorIndex >= 0) {
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
 }
 
 bool MainWindow::displayChart(const QString& paramName, const QString& paramFormula, bool switchToChartTab) {
     if (measurements.empty()) {
         QMessageBox::information(this, "Informacja", "Brak danych do wyświetlenia na wykresie");
         return false;
     }
     
     // Sprawdź czy chartView jest poprawnie zainicjalizowany
     if (!chartView) {
         QMessageBox::critical(this, "Błąd", "Błąd komponentu wykresu");
         return false;
     }
     
     // Utwórz wykres
     QChart *chart = new QChart();
     
     try {
         // Debug info
         qDebug() << "Tworzenie wykresu dla parametru:" << paramName 
                  << "(" << paramFormula << ")";
         qDebug() << "Liczba pomiarow:" << measurements.size();
         
         // Wartości min i max do ustalenia zakresu osi Y
         double minValue = std::numeric_limits<double>::max();
         double maxValue = std::numeric_limits<double>::lowest();
         qint64 minTime = std::numeric_limits<qint64>::max();
         qint64 maxTime = std::numeric_limits<qint64>::lowest();
         
         if (paramName == "Wszystkie parametry") {
             // Pokazanie legendy dla wielu parametrów
             chart->legend()->show();
             
             // Grupowanie pomiarów według czujników
             std::map<int, std::vector<Measurement>> sensorMeasurements;
             for (const auto& sensor : sensors) {
                 auto measurements = apiClient->getMeasurements(sensor.id);
                 if (!measurements.empty()) {
                     sensorMeasurements[sensor.id] = measurements;
                 }
             }
             
             for (const auto& sensor : sensors) {
                 auto it = sensorMeasurements.find(sensor.id);
                 if (it == sensorMeasurements.end()) {
                     continue; // Brak danych dla tego czujnika
                 }
                 
                 QLineSeries *series = new QLineSeries();
                 QString sensorParamName = QString::fromStdString(sensor.paramName);
                 QString sensorParamFormula = QString::fromStdString(sensor.paramFormula);
                 series->setName(QString("%1 (%2)").arg(sensorParamName, sensorParamFormula));
                 
                 // Ustawienie koloru serii na podstawie mapy sensorColors
                 auto colorIt = sensorColors.find(sensor.id);
                 if (colorIt != sensorColors.end()) {
                     series->setPen(QPen(colorIt->second, 2));
                 } else {
                     series->setPen(QPen(Qt::black, 2)); // Domyślny kolor
                 }
                 
                 // Wypełnienie serii danymi
                 for (const auto& measurement : it->second) {
                     QDateTime dateTime = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
                     qint64 timestamp = dateTime.toMSecsSinceEpoch();
                     series->append(timestamp, measurement.value);
                     
                     // Aktualizacja wartości min/max
                     minValue = std::min(minValue, measurement.value);
                     maxValue = std::max(maxValue, measurement.value);
                     minTime = std::min(minTime, timestamp);
                     maxTime = std::max(maxTime, timestamp);
                 }
                 
                 if (series->count() > 0) {
                     chart->addSeries(series);
                 } else {
                     delete series;
                 }
             }
         } else {
             // Pojedynczy parametr - ukryj legendę
             chart->legend()->hide();
             
             QLineSeries *series = new QLineSeries();
             series->setName(paramName);
             
             // Znajdź ID czujnika na podstawie paramName i paramFormula
             int sensorId = -1;
             for (const auto& sensor : sensors) {
                 if (QString::fromStdString(sensor.paramName) == paramName &&
                     QString::fromStdString(sensor.paramFormula) == paramFormula) {
                     sensorId = sensor.id;
                     break;
                 }
             }
             
             // Ustawienie koloru serii
             auto colorIt = sensorColors.find(sensorId);
             if (colorIt != sensorColors.end()) {
                 series->setPen(QPen(colorIt->second, 2));
             } else {
                 series->setPen(QPen(Qt::blue, 2)); // Domyślny kolor
             }
             
             // Wypełnienie serii danymi
             for (const auto& measurement : measurements) {
                 QDateTime dateTime = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
                 qint64 timestamp = dateTime.toMSecsSinceEpoch();
                 series->append(timestamp, measurement.value);
                 
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
                 return false;
             }
             
             chart->addSeries(series);
         }
         
         if (chart->series().isEmpty()) {
             QMessageBox::warning(this, "Błąd", "Brak danych do wyświetlenia na wykresie");
             delete chart;
             return false;
         }
         
         // Dodanie marginesu do zakresu wartości
         double margin = (maxValue - minValue) * 0.1;
         if (margin < 0.001) margin = 0.1; // Minimalny margines
         minValue = minValue - margin;
         maxValue = maxValue + margin;
         
         // Określenie zakresu dat dla tytułu
         QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(minTime);
         QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(maxTime);
         QString dateRangeStr = QString("Okres: %1 - %2").arg(
             minDateTime.toString("dd.MM.yyyy"),
             maxDateTime.toString("dd.MM.yyyy")
         );
         
         // Ustawienie tytułu wykresu
         QString title = paramName == "Wszystkie parametry" ?
             QString("Pomiary wszystkich parametrów\n%1").arg(dateRangeStr) :
             QString("Pomiary: %1 (%2)\n%3").arg(paramName, paramFormula, dateRangeStr);
         chart->setTitle(title);
         
         // Osie wykresu
         QDateTimeAxis *axisX = new QDateTimeAxis;
         axisX->setTickCount(8);
         axisX->setFormat("dd.MM.yy hh:mm");
         axisX->setLabelsAngle(-45);
         axisX->setTitleText("Czas pomiaru");
 
         QValueAxis *axisY = new QValueAxis;
         axisY->setLabelFormat("%.2f");
         axisY->setTitleText(paramName == "Wszystkie parametry" ? "Wartości" : paramFormula);
         axisY->setRange(minValue, maxValue);
         
         chart->addAxis(axisX, Qt::AlignBottom);
         chart->addAxis(axisY, Qt::AlignLeft);
         
         // Dołącz osie do wszystkich serii
         for (auto series : chart->series()) {
             series->attachAxis(axisX);
             series->attachAxis(axisY);
         }
         
         // Ustawienie zakresu czasu
         axisX->setRange(QDateTime::fromMSecsSinceEpoch(minTime), QDateTime::fromMSecsSinceEpoch(maxTime));
         
         // Ustawienie nowego wykresu w widoku
         chartView->setChart(chart);
         
         // Animacja
         chart->setAnimationOptions(QChart::SeriesAnimations);
         
         // Przełącz na zakładkę z wykresem jeśli wymagane
         if (switchToChartTab) {
             QList<QTabWidget*> tabWidgets = findChildren<QTabWidget*>();
             if (!tabWidgets.isEmpty()) {
                 tabWidgets.first()->setCurrentIndex(1);
             }
         }
         
         // Wyświetl informację w pasku statusu
         statusLabel->setText("Wykres został zaktualizowany");
         
         qDebug() << "Wykres utworzony pomyślnie";
         return true;
     }
     catch (const std::exception& e) {
         QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd podczas tworzenia wykresu: %1").arg(e.what()));
         delete chart;
         return false;
     }
 }
 
 void MainWindow::saveMeasurements() {
     if (measurements.empty()) {
         QMessageBox::warning(this, "Ostrzeżenie", "Brak danych do zapisania");
         return;
     }
     
     int stationIndex = stationComboBox->currentIndex();
     int sensorIndex = sensorComboBox->currentIndex();
     
     if (stationIndex < 0 || stationIndex >= static_cast<int>(stations.size())) {
         QMessageBox::warning(this, "Błąd", "Nie wybrano stacji");
         return;
     }
     
     QString defaultFileName;
     if (sensorIndex == 0) {
         defaultFileName = QString("pomiary_%1_wszystkie_%2")
             .arg(QString::fromStdString(stations[stationIndex].name).simplified().replace(" ", "_"))
             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));
     } else {
         if (sensorIndex - 1 >= static_cast<int>(sensors.size())) {
             QMessageBox::warning(this, "Błąd", "Nie wybrano prawidłowego czujnika");
             return;
         }
         defaultFileName = QString("pomiary_%1_%2_%3")
             .arg(QString::fromStdString(stations[stationIndex].name).simplified().replace(" ", "_"))
             .arg(QString::fromStdString(sensors[sensorIndex - 1].paramFormula))
             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));
     }
     
     QString filePath = exportPath + "/" + defaultFileName + ".json";
     
     bool success = saveMeasurementsToJSON(filePath);
     
     if (success) {
         statusLabel->setText(QString("Zapisano dane do pliku: %1").arg(filePath));
         QMessageBox::information(this, "Informacja", 
             QString("Dane zostały zapisane pomyślnie do pliku:\n%1").arg(filePath));
     }
     else {
         statusLabel->setText("Błąd podczas zapisywania danych");
     }
 }
 
 bool MainWindow::saveMeasurementsToJSON(const QString& filename) {
     try {
         int stationIndex = stationComboBox->currentIndex();
         int sensorIndex = sensorComboBox->currentIndex();
         
         json jsonData;
         
         auto qstringToUtf8 = [](const QString& str) -> std::string {
             return str.toUtf8().toStdString();
         };
         
         jsonData["metadata"]["station"]["id"] = stations[stationIndex].id;
         jsonData["metadata"]["station"]["name"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].name));
         jsonData["metadata"]["station"]["city"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].city));
         jsonData["metadata"]["station"]["province"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].province));
         jsonData["metadata"]["station"]["location"]["lat"] = stations[stationIndex].lat;
         jsonData["metadata"]["station"]["location"]["lon"] = stations[stationIndex].lon;
         
         if (sensorIndex == 0) {
             jsonData["metadata"]["sensor"]["id"] = -1;
             jsonData["metadata"]["sensor"]["paramName"] = "Wszystkie";
             jsonData["metadata"]["sensor"]["paramFormula"] = "Wszystkie";
             jsonData["metadata"]["sensor"]["paramCode"] = "Wszystkie";
             // Zapisz kolory wszystkich czujników
             json sensorColorsJson = json::array();
             for (const auto& sensor : sensors) {
                 json sensorColor;
                 sensorColor["id"] = sensor.id;
                 sensorColor["paramName"] = qstringToUtf8(QString::fromStdString(sensor.paramName));
                 sensorColor["paramFormula"] = qstringToUtf8(QString::fromStdString(sensor.paramFormula));
                 auto colorIt = sensorColors.find(sensor.id);
                 if (colorIt != sensorColors.end()) {
                     sensorColor["color"] = qstringToUtf8(colorIt->second.name());
                 }
                 sensorColorsJson.push_back(sensorColor);
             }
             jsonData["metadata"]["sensorColors"] = sensorColorsJson;
         } else {
             jsonData["metadata"]["sensor"]["id"] = sensors[sensorIndex - 1].id;
             jsonData["metadata"]["sensor"]["paramName"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex - 1].paramName));
             jsonData["metadata"]["sensor"]["paramFormula"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex - 1].paramFormula));
             jsonData["metadata"]["sensor"]["paramCode"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex - 1].paramCode));
             // Zapisz kolor dla pojedynczego czujnika
             auto colorIt = sensorColors.find(sensors[sensorIndex - 1].id);
             if (colorIt != sensorColors.end()) {
                 jsonData["metadata"]["sensor"]["color"] = qstringToUtf8(colorIt->second.name());
             }
         }
         
         jsonData["metadata"]["exportDate"] = qstringToUtf8(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
         
         json measurementsArray = json::array();
         for (const auto& measurement : measurements) {
             json item;
             item["date"] = measurement.date;
             item["value"] = measurement.value;
             measurementsArray.push_back(item);
         }
         
         jsonData["measurements"] = measurementsArray;
         
         QFileInfo fileInfo(filename);
         QDir().mkpath(fileInfo.path());
         
         QFile file(filename);
         if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
             QMessageBox::critical(this, "Błąd", 
                 QString("Nie można otworzyć pliku do zapisu: %1").arg(filename));
             return false;
         }
         
         QTextStream out(&file);
         out.setCodec("UTF-8");
         out << QString::fromStdString(jsonData.dump(4));
         file.close();
         
         qDebug() << "Zapisano dane do pliku JSON:" << filename;
         qDebug() << "Nazwa stacji:" << QString::fromStdString(stations[stationIndex].name);
         qDebug() << "Miasto:" << QString::fromStdString(stations[stationIndex].city);
         
         return true;
     }
     catch (const std::exception& e) {
         QMessageBox::critical(this, "Błąd", 
             QString("Wystąpił błąd podczas zapisywania do pliku JSON: %1").arg(e.what()));
         return false;
     }
 }
 
 void MainWindow::openSavedMeasurements() {
     QDialog* dialog = createSavedMeasurementsDialog();
     dialog->exec();
     delete dialog;
 }
 
 QDialog* MainWindow::createSavedMeasurementsDialog() {
     QDialog* dialog = new QDialog(this);
     dialog->setWindowTitle("Zapisane pomiary");
     dialog->resize(500, 400);
     
     QVBoxLayout* layout = new QVBoxLayout(dialog);
     
     QLabel* infoLabel = new QLabel("Wybierz plik z zapisanymi pomiarami:", dialog);
     layout->addWidget(infoLabel);
     
     QListWidget* fileList = new QListWidget(dialog);
     layout->addWidget(fileList);
     
     QHBoxLayout* buttonLayout = new QHBoxLayout();
     QPushButton* closeButton = new QPushButton("Zamknij", dialog);
     buttonLayout->addStretch();
     buttonLayout->addWidget(closeButton);
     layout->addLayout(buttonLayout);
     
     auto fillFileList = [this, fileList]() {
         fileList->clear();
         
         QDir dir(exportPath);
         QStringList filters;
         filters << "*.json";
         dir.setNameFilters(filters);
         
         QFileInfoList fileInfos = dir.entryInfoList(QDir::Files, QDir::Time);
         
         for (const QFileInfo& fileInfo : fileInfos) {
             QListWidgetItem* item = new QListWidgetItem(fileInfo.fileName());
             item->setData(Qt::UserRole, fileInfo.filePath());
             fileList->addItem(item);
         }
         
         if (fileList->count() == 0) {
             QListWidgetItem* noFilesItem = new QListWidgetItem("Brak zapisanych plików");
             noFilesItem->setFlags(Qt::NoItemFlags);
             fileList->addItem(noFilesItem);
         }
     };
     
     fillFileList();
     
     connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
     connect(fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::loadSavedMeasurement);
     
     return dialog;
 }
 
 void MainWindow::loadSavedMeasurement(QListWidgetItem* item) {
     if (!item || item->flags() == Qt::NoItemFlags) {
         return;
     }
     
     QString filePath = item->data(Qt::UserRole).toString();
     if (filePath.isEmpty()) {
         return;
     }
     
     item->listWidget()->window()->close();
     
     dataTable->setRowCount(0);
     
     QFile file(filePath);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         QMessageBox::critical(this, "Błąd", 
             QString("Nie można otworzyć pliku: %1").arg(filePath));
         return;
     }
     
     QTextStream in(&file);
     in.setCodec("UTF-8");
     QString jsonString = in.readAll();
     file.close();
     
     try {
         json jsonData = json::parse(jsonString.toStdString());
         
         QString stationName;
         QString stationCity;
         QString stationProvince;
         int stationId = -1;
         double stationLat = 0.0;
         double stationLon = 0.0;
         
         if (jsonData.contains("metadata") && jsonData["metadata"].contains("station")) {
             auto& station = jsonData["metadata"]["station"];
             if (station.contains("id")) stationId = station["id"].get<int>();
             if (station.contains("name")) stationName = QString::fromUtf8(station["name"].get<std::string>().c_str());
             if (station.contains("city")) stationCity = QString::fromUtf8(station["city"].get<std::string>().c_str());
             if (station.contains("province")) stationProvince = QString::fromUtf8(station["province"].get<std::string>().c_str());
             
             if (station.contains("location")) {
                 auto& location = station["location"];
                 if (location.contains("lat")) stationLat = location["lat"].get<double>();
                 if (location.contains("lon")) stationLon = location["lon"].get<double>();
             }
         }
         
         QString paramName;
         QString paramFormula;
         QString paramCode;
         int sensorId = -1;
         
         if (jsonData.contains("metadata") && jsonData["metadata"].contains("sensor")) {
             auto& sensor = jsonData["metadata"]["sensor"];
             if (sensor.contains("id")) sensorId = sensor["id"].get<int>();
             if (sensor.contains("paramName")) paramName = QString::fromUtf8(sensor["paramName"].get<std::string>().c_str());
             if (sensor.contains("paramFormula")) paramFormula = QString::fromUtf8(sensor["paramFormula"].get<std::string>().c_str());
             if (sensor.contains("paramCode")) paramCode = QString::fromUtf8(sensor["paramCode"].get<std::string>().c_str());
             
             // Wczytaj kolor dla pojedynczego czujnika
             if (sensor.contains("color") && sensorId != -1) {
                 QString colorName = QString::fromUtf8(sensor["color"].get<std::string>().c_str());
                 sensorColors[sensorId] = QColor(colorName);
             }
         }
         
         // Wczytaj kolory dla wszystkich czujników, jeśli dostępne
         if (jsonData["metadata"].contains("sensorColors") && jsonData["metadata"]["sensorColors"].is_array()) {
             for (const auto& sensorColor : jsonData["metadata"]["sensorColors"]) {
                 int id = sensorColor["id"].get<int>();
                 QString colorName = QString::fromUtf8(sensorColor["color"].get<std::string>().c_str());
                 sensorColors[id] = QColor(colorName);
             }
         }
         
         measurements.clear();
         
         if (jsonData.contains("measurements") && jsonData["measurements"].is_array()) {
             for (const auto& item : jsonData["measurements"]) {
                 Measurement measurement;
                 measurement.date = item["date"].get<std::string>();
                 measurement.value = item["value"].get<double>();
                 measurements.push_back(measurement);
             }
         }
         
         if (measurements.empty()) {
             QMessageBox::warning(this, "Ostrzeżenie", "Plik nie zawiera żadnych pomiarów");
             return;
         }
         
         Station tempStation;
         tempStation.id = stationId;
         tempStation.name = stationName.toUtf8().toStdString();
         tempStation.city = stationCity.toUtf8().toStdString();
         tempStation.province = stationProvince.toUtf8().toStdString();
         tempStation.lat = stationLat;
         tempStation.lon = stationLon;
         
         Sensor tempSensor;
         tempSensor.id = sensorId;
         tempSensor.paramName = paramName.toUtf8().toStdString();
         tempSensor.paramFormula = paramFormula.toUtf8().toStdString();
         tempSensor.paramCode = paramCode.toUtf8().toStdString();
         
         int stationIndex = -1;
         for (size_t i = 0; i < stations.size(); i++) {
             if (stations[i].id == stationId) {
                 stationIndex = i;
                 break;
             }
         }
         
         if (stationIndex == -1) {
             stations.push_back(tempStation);
             stationIndex = stations.size() - 1;
         }
         
         int sensorIndex = -1;
         for (size_t i = 0; i < sensors.size(); i++) {
             if (sensors[i].id == sensorId) {
                 sensorIndex = i;
                 break;
             }
         }
         
         if (sensorIndex == -1 && sensorId != -1) {
             sensors.push_back(tempSensor);
             sensorIndex = sensors.size() - 1;
         }
         
         stationComboBox->blockSignals(true);
         sensorComboBox->blockSignals(true);
         
         int stationComboIndex = -1;
         for (int i = 0; i < stationComboBox->count(); i++) {
             if (stationComboBox->itemText(i).contains(stationName)) {
                 stationComboIndex = i;
                 break;
             }
         }
         
         if (stationComboIndex == -1) {
             QString displayText = QString("%1 (%2, %3) [WCZYTANE Z PLIKU]").arg(
                 stationName, stationCity, stationProvince);
             stationComboBox->addItem(displayText);
             stationComboIndex = stationComboBox->count() - 1;
         }
         
         stationComboBox->setCurrentIndex(stationComboIndex);
         
         sensorComboBox->clear();
         sensorComboBox->addItem("Wszystkie");
         for (const auto& sensor : sensors) {
             QString displayText = QString("%1 (%2)").arg(
                 QString::fromUtf8(sensor.paramName.c_str()),
                 QString::fromUtf8(sensor.paramFormula.c_str())
             );
             sensorComboBox->addItem(displayText);
         }
         
         if (paramName == "Wszystkie") {
             sensorComboBox->setCurrentIndex(0);
         } else {
             sensorComboBox->setCurrentIndex(sensorIndex + 1);
         }
         
         stationComboBox->blockSignals(false);
         sensorComboBox->blockSignals(false);
         
         dataTable->setRowCount(measurements.size());
         
         for (size_t i = 0; i < measurements.size(); ++i) {
             const auto& measurement = measurements[i];
             
             QDateTime dateTime = QDateTime::fromString(QString::fromStdString(measurement.date), Qt::ISODate);
             QTableWidgetItem *dateItem = new QTableWidgetItem(dateTime.toString("dd.MM.yyyy hh:mm"));
             dataTable->setItem(i, 0, dateItem);
             
             QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(measurement.value, 'f', 2));
             dataTable->setItem(i, 1, valueItem);
         }
         
         dataTable->sortItems(0, Qt::DescendingOrder);
         
         if (displayChart(paramName, paramFormula, true)) {
             saveButton->setEnabled(true);
             statusLabel->setText(QString("Wczytano dane z pliku: %1").arg(filePath));
         }
     }
     catch (const std::exception& e) {
         QMessageBox::critical(this, "Błąd", 
             QString("Wystąpił błąd podczas wczytywania pliku JSON: %1").arg(e.what()));
     }
 }
 
 bool MainWindow::loadMeasurementsFromJSON(const QString& filePath) {
     try {
         QFile file(filePath);
         if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
             QMessageBox::critical(this, "Błąd", 
                 QString("Nie można otworzyć pliku: %1").arg(filePath));
             return false;
         }
         
         QTextStream in(&file);
         in.setCodec("UTF-8");
         QString jsonString = in.readAll();
         file.close();
         
         json jsonData = json::parse(jsonString.toStdString());
         
         QString stationName;
         QString stationCity;
         QString stationProvince;
         int stationId = -1;
         double stationLat = 0.0;
         double stationLon = 0.0;
         
         if (jsonData.contains("metadata") && jsonData["metadata"].contains("station")) {
             auto& station = jsonData["metadata"]["station"];
             if (station.contains("id")) stationId = station["id"].get<int>();
             if (station.contains("name")) stationName = QString::fromUtf8(station["name"].get<std::string>().c_str());
             if (station.contains("city")) stationCity = QString::fromUtf8(station["city"].get<std::string>().c_str());
             if (station.contains("province")) stationProvince = QString::fromUtf8(station["province"].get<std::string>().c_str());
             
             if (station.contains("location")) {
                 auto& location = station["location"];
                 if (location.contains("lat")) stationLat = location["lat"].get<double>();
                 if (location.contains("lon")) stationLon = location["lon"].get<double>();
             }
         }
         
         QString paramName;
         QString paramFormula;
         QString paramCode;
         int sensorId = -1;
         
         if (jsonData.contains("metadata") && jsonData["metadata"].contains("sensor")) {
             auto& sensor = jsonData["metadata"]["sensor"];
             if (sensor.contains("id")) sensorId = sensor["id"].get<int>();
             if (sensor.contains("paramName")) paramName = QString::fromUtf8(sensor["paramName"].get<std::string>().c_str());
             if (sensor.contains("paramFormula")) paramFormula = QString::fromUtf8(sensor["paramFormula"].get<std::string>().c_str());
             if (sensor.contains("paramCode")) paramCode = QString::fromUtf8(sensor["paramCode"].get<std::string>().c_str());
             
             // Wczytaj kolor dla pojedynczego czujnika
             if (sensor.contains("color") && sensorId != -1) {
                 QString colorName = QString::fromUtf8(sensor["color"].get<std::string>().c_str());
                 sensorColors[sensorId] = QColor(colorName);
             }
         }
         
         // Wczytaj kolory dla wszystkich czujników
         if (jsonData["metadata"].contains("sensorColors") && jsonData["metadata"]["sensorColors"].is_array()) {
             for (const auto& sensorColor : jsonData["metadata"]["sensorColors"]) {
                 int id = sensorColor["id"].get<int>();
                 QString colorName = QString::fromUtf8(sensorColor["color"].get<std::string>().c_str());
                 sensorColors[id] = QColor(colorName);
             }
         }
         
         measurements.clear();
         
         if (jsonData.contains("measurements") && jsonData["measurements"].is_array()) {
             for (const auto& item : jsonData["measurements"]) {
                 Measurement measurement;
                 measurement.date = item["date"].get<std::string>();
                 measurement.value = item["value"].get<double>();
                 measurements.push_back(measurement);
             }
         }
         
         if (measurements.empty()) {
             QMessageBox::warning(this, "Ostrzeżenie", "Plik nie zawiera żadnych pomiarów");
             return false;
         }
         
         Station tempStation;
         tempStation.id = stationId;
         tempStation.name = stationName.toUtf8().toStdString();
         tempStation.city = stationCity.toUtf8().toStdString();
         tempStation.province = stationProvince.toUtf8().toStdString();
         tempStation.lat = stationLat;
         tempStation.lon = stationLon;
         
         Sensor tempSensor;
         tempSensor.id = sensorId;
         tempSensor.paramName = paramName.toUtf8().toStdString();
         tempSensor.paramFormula = paramFormula.toUtf8().toStdString();
         tempSensor.paramCode = paramCode.toUtf8().toStdString();
         
         bool stationExists = false;
         int existingStationIndex = -1;
         for (size_t i = 0; i < stations.size(); i++) {
             if (stations[i].id == stationId) {
                 stationExists = true;
                 existingStationIndex = i;
                 break;
             }
         }
         
         if (!stationExists) {
             stations.push_back(tempStation);
             existingStationIndex = stations.size() - 1;
         }
         
         bool sensorExists = false;
         int existingSensorIndex = -1;
         for (size_t i = 0; i < sensors.size(); i++) {
             if (sensors[i].id == sensorId) {
                 sensorExists = true;
                 existingSensorIndex = i;
                 break;
             }
         }
         
         if (!sensorExists && sensorId != -1) {
             sensors.push_back(tempSensor);
             existingSensorIndex = sensors.size() - 1;
         }
         
         bool stationBlocked = stationComboBox->blockSignals(true);
         bool sensorBlocked = sensorComboBox->blockSignals(true);
         
         int stationIndex = -1;
         for (int i = 0; i < stationComboBox->count(); i++) {
             if (stationComboBox->itemText(i).contains(stationName)) {
                 stationIndex = i;
                 break;
             }
         }
         
         if (stationIndex == -1) {
             QString displayText = QString("%1 (%2, %3) [WCZYTANE Z PLIKU]").arg(
                 stationName, stationCity, stationProvince);
             stationComboBox->addItem(displayText);
             stationIndex = stationComboBox->count() - 1;
         }
         
         stationComboBox->setCurrentIndex(stationIndex);
         
         sensorComboBox->clear();
         sensorComboBox->addItem("Wszystkie");
         QString selectedSensorText;
         for (const auto& sensor : sensors) {
             QString displayText = QString("%1 (%2)").arg(
                 QString::fromUtf8(sensor.paramName.c_str()),
                 QString::fromUtf8(sensor.paramFormula.c_str())
             );
             sensorComboBox->addItem(displayText);
             
             if (sensor.id == sensorId) {
                 selectedSensorText = displayText;
             }
         }
         
         int sensorIndex = -1;
         if (paramName == "Wszystkie") {
             sensorIndex = 0;
         } else {
             for (int i = 0; i < sensorComboBox->count(); i++) {
                 if (sensorComboBox->itemText(i).contains(paramName)) {
                     sensorIndex = i;
                     break;
                 }
             }
         }
         
         if (sensorIndex != -1) {
             sensorComboBox->setCurrentIndex(sensorIndex);
         } else if (existingSensorIndex != -1) {
             sensorComboBox->setCurrentIndex(existingSensorIndex + 1);
         }
         
         stationComboBox->blockSignals(stationBlocked);
         sensorComboBox->blockSignals(sensorBlocked);
         
         return true;
     }
     catch (const std::exception& e) {
         QMessageBox::critical(this, "Błąd", 
             QString("Wystąpił błąd podczas wczytywania pliku JSON: %1").arg(e.what()));
         return false;
     }
 }