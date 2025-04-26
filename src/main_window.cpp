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
     sensorLayout->addWidget(sensorLabel);
     sensorLayout->addWidget(sensorComboBox);
     
     // Przyciski
     QHBoxLayout *buttonLayout = new QHBoxLayout();
     refreshButton = new QPushButton("Odśwież dane", this);
     //showChartButton = new QPushButton("Pokaż wykres", this);
     saveButton = new QPushButton("Zapisz dane", this);
     openSavedButton = new QPushButton("Przeglądaj zapisane dane", this);
     //showChartButton->setEnabled(false);
     saveButton->setEnabled(false);
     
     buttonLayout->addWidget(refreshButton);
     //buttonLayout->addWidget(showChartButton);
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
     //connect(showChartButton, &QPushButton::clicked, this, &MainWindow::showChart);
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
     if  (index < 0 || index >= static_cast<int>(stations.size())) {
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
         //showChartButton->setEnabled(false);
         saveButton->setEnabled(false);
         return;
     }
     
     // Wypełnienie tabeli danymi
     fillDataTable();
     
     // Aktywacja przycisków
     //showChartButton->setEnabled(true);
     saveButton->setEnabled(true);
     
     statusLabel->setText("Gotowy");
 }
 
 void MainWindow::refreshData() {
     // Odświeżenie danych dla aktualnie wybranego czujnika
     int sensorIndex = sensorComboBox->currentIndex();
     if (sensorIndex >= 0 && sensorIndex < static_cast<int>(sensors.size())) {
         onSensorSelected(sensorIndex);
     }
     createChart();
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
     createChart();
 }
 
 void MainWindow::createChart() {
    if (measurements.empty()) {
        QMessageBox::information(this, "Informacja", "Brak danych do wyświetlenia na wykresie");
        return;
    }
    
    // Pobranie nazwy parametru
    int sensorIndex = sensorComboBox->currentIndex();
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensors.size())) {
        // Zamiast wyświetlać błąd, spróbujmy użyć danych z pliku
        // który właśnie otworzyliśmy
        if (sensors.empty()) {
            QMessageBox::warning(this, "Błąd", "Brak danych o parametrach");
            return;
        }
        
        // Użyj ostatniego dodanego sensora, który prawdopodobnie pochodzi z wczytanego pliku
        sensorIndex = sensors.size() - 1;
        
        // Debug info
        qDebug() << "Brak wybranego parametru, używam ostatniego dostępnego:" 
                 << QString::fromStdString(sensors[sensorIndex].paramName);
    }
    
    // Sprawdź czy chartView jest poprawnie zainicjalizowany
    if (!chartView) {
        QMessageBox::critical(this, "Błąd", "Błąd komponentu wykresu");
        return;
    }
    
    QString paramName = QString::fromUtf8(sensors[sensorIndex].paramName.c_str());
    QString paramFormula = QString::fromUtf8(sensors[sensorIndex].paramFormula.c_str());
    
    // Utwórz wykres
    QChart *chart = new QChart();
    
    try {
        // Debug info
        qDebug() << "Tworzenie wykresu dla parametru:" << paramName 
                << "(" << paramFormula << ")";
        qDebug() << "Liczba pomiarów:" << measurements.size();
        
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
                qDebug() << "Punkt" << series->count() << ":" 
                        << dateTime.toString("yyyy-MM-dd hh:mm")
                        << ", wartość:" << measurement.value;
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
        
        // Określenie zakresu dat dla tytułu
        QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(minTime);
        QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(maxTime);
        QString dateRangeStr = QString("Okres: %1 - %2").arg(
            minDateTime.toString("dd.MM.yyyy"),
            maxDateTime.toString("dd.MM.yyyy")
        );
        
        // Dodanie serii do wykresu
        chart->addSeries(series);
        // Połączenie tytułu z zakresem dat
        chart->setTitle(QString("Pomiary: %1 (%2)\n%3").arg(
            paramName,
            paramFormula,
            dateRangeStr
        ));
        
        // Osie wykresu
        QDateTimeAxis *axisX = new QDateTimeAxis;
        axisX->setTickCount(8); // Mniej punktów na osi
        axisX->setFormat("hh:mm"); // Tylko godzina i minuty
        axisX->setTitleText("Czas pomiaru");

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
        
        qDebug() << "Wykres utworzony pomyślnie";
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

void MainWindow::saveMeasurements() {
    // Sprawdzenie czy są dane do zapisania
    if (measurements.empty()) {
        QMessageBox::warning(this, "Ostrzeżenie", "Brak danych do zapisania");
        return;
    }
    
    // Pobranie informacji o stacji i czujniku
    int stationIndex = stationComboBox->currentIndex();
    int sensorIndex = sensorComboBox->currentIndex();
    
    if (stationIndex < 0 || stationIndex >= static_cast<int>(stations.size()) ||
        sensorIndex < 0 || sensorIndex >= static_cast<int>(sensors.size())) {
        QMessageBox::warning(this, "Błąd", "Nie wybrano stacji lub czujnika");
        return;
    }
    
    // Utworzenie sugerowanej nazwy pliku na podstawie stacji i czujnika
    QString defaultFileName = QString("pomiary_%1_%2_%3")
        .arg(QString::fromStdString(stations[stationIndex].name).simplified().replace(" ", "_"))
        .arg(QString::fromStdString(sensors[sensorIndex].paramFormula))
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));
    
    // Pełna ścieżka do pliku
    QString filePath = exportPath + "/" + defaultFileName + ".json";
    
    bool success = saveMeasurementsToJSON(filePath);
    
    // Informacja o rezultacie
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
        // Pobranie informacji o stacji i czujniku
        int stationIndex = stationComboBox->currentIndex();
        int sensorIndex = sensorComboBox->currentIndex();
        
        // Przygotowanie obiektu JSON
        json jsonData;
        
        // Funkcja pomocnicza do konwersji QString na UTF-8 std::string
        auto qstringToUtf8 = [](const QString& str) -> std::string {
            return str.toUtf8().toStdString();
        };
        
        // Metadane z poprawnym kodowaniem UTF-8
        jsonData["metadata"]["station"]["id"] = stations[stationIndex].id;
        jsonData["metadata"]["station"]["name"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].name));
        jsonData["metadata"]["station"]["city"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].city));
        jsonData["metadata"]["station"]["province"] = qstringToUtf8(QString::fromStdString(stations[stationIndex].province));
        jsonData["metadata"]["station"]["location"]["lat"] = stations[stationIndex].lat;
        jsonData["metadata"]["station"]["location"]["lon"] = stations[stationIndex].lon;
        
        jsonData["metadata"]["sensor"]["id"] = sensors[sensorIndex].id;
        jsonData["metadata"]["sensor"]["paramName"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex].paramName));
        jsonData["metadata"]["sensor"]["paramFormula"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex].paramFormula));
        jsonData["metadata"]["sensor"]["paramCode"] = qstringToUtf8(QString::fromStdString(sensors[sensorIndex].paramCode));
        
        jsonData["metadata"]["exportDate"] = qstringToUtf8(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        
        // Przygotowanie tablicy pomiarów
        json measurementsArray = json::array();
        
        for (const auto& measurement : measurements) {
            json item;
            item["date"] = measurement.date;
            item["value"] = measurement.value;
            measurementsArray.push_back(item);
        }
        
        // Dodanie pomiarów do głównego obiektu
        jsonData["measurements"] = measurementsArray;
        
        // Upewnij się, że katalog eksportu istnieje
        QFileInfo fileInfo(filename);
        QDir().mkpath(fileInfo.path());
        
        // Zapis do pliku z określeniem kodowania UTF-8
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Błąd", 
                QString("Nie można otworzyć pliku do zapisu: %1").arg(filename));
            return false;
        }
        
        // Zapisanie danych JSON z formatowaniem
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << QString::fromStdString(jsonData.dump(4));
        file.close();
        
        // Poprawny zapis do konsoli z polskimi znakami
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
    
    // Etykieta informacyjna
    QLabel* infoLabel = new QLabel("Wybierz plik z zapisanymi pomiarami:", dialog);
    layout->addWidget(infoLabel);
    
    // Lista plików
    QListWidget* fileList = new QListWidget(dialog);
    layout->addWidget(fileList);
    
    // Przyciski
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* refreshButton = new QPushButton("Odśwież listę", dialog);
    QPushButton* closeButton = new QPushButton("Zamknij", dialog);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    // Funkcja wypełniająca listę plików
    auto fillFileList = [this, fileList]() {
        fileList->clear();
        
        QDir dir(exportPath);
        QStringList filters;
        filters << "*.json";
        dir.setNameFilters(filters);
        
        // Sortowanie plików według daty modyfikacji (najnowsze na górze)
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
    
    // Wypełnienie listy plików przy otwarciu
    fillFileList();
    
    // Połączenie sygnałów i slotów
    connect(refreshButton, &QPushButton::clicked, fillFileList);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::loadSavedMeasurement);
    
    return dialog;
}

void MainWindow::loadSavedMeasurement(QListWidgetItem* item) {
    // Sprawdź czy element jest ważny
    if (!item || item->flags() == Qt::NoItemFlags) {
        return;
    }
    
    // Pobierz ścieżkę do pliku
    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) {
        return;
    }
    
    // Wczytaj dane z pliku
    if (loadMeasurementsFromJSON(filePath)) {
        // Ustaw informacje o stacji i czujniku
        statusLabel->setText(QString("Wczytano zapisane dane z pliku: %1").arg(filePath));
        
        // Ważne: upewnij się, że sensory i stacje są dostępne
        if (measurements.empty()) {
            QMessageBox::warning(this, "Ostrzeżenie", "Wczytany plik nie zawiera pomiarów");
            return;
        }
        
        // Upewnij się, że ComboBox-y pokazują wczytane dane
        int stationIndex = stationComboBox->currentIndex();
        int sensorIndex = sensorComboBox->currentIndex();
        
        // Debug info
        qDebug() << "Indeks stacji:" << stationIndex << "z" << stations.size() << "stacji";
        qDebug() << "Indeks sensora:" << sensorIndex << "z" << sensors.size() << "sensorów";
        
        // Wypełnij tabelę danymi
        fillDataTable();
        
        // Utwórz i wyświetl wykres - nawet jeśli nie mamy wybranego sensora,
        // poprawiona funkcja createChart() powinna sobie poradzić
        createChart();
        
        // Przełącz na zakładkę z wykresem
        QList<QTabWidget*> tabWidgets = findChildren<QTabWidget*>();
        if (!tabWidgets.isEmpty()) {
            tabWidgets.first()->setCurrentIndex(1); // Zakładka z wykresem
        }
        
        // Aktywuj przyciski
        //showChartButton->setEnabled(true);
        saveButton->setEnabled(true);
    }
}

bool MainWindow::loadMeasurementsFromJSON(const QString& filePath) {
    try {
        // Odczyt pliku za pomocą Qt z obsługą UTF-8
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Błąd", 
                QString("Nie można otworzyć pliku: %1").arg(filePath));
            return false;
        }
        
        // Odczyt zawartości pliku z poprawnym kodowaniem
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString jsonString = in.readAll();
        file.close();
        
        // Parsowanie JSON
        json jsonData = json::parse(jsonString.toStdString());
        
        // Pobranie metadanych o stacji
        QString stationName;
        QString stationCity;
        QString stationProvince;
        int stationId = -1;
        
        if (jsonData.contains("metadata") && jsonData["metadata"].contains("station")) {
            auto& station = jsonData["metadata"]["station"];
            if (station.contains("id")) stationId = station["id"].get<int>();
            if (station.contains("name")) stationName = QString::fromUtf8(station["name"].get<std::string>().c_str());
            if (station.contains("city")) stationCity = QString::fromUtf8(station["city"].get<std::string>().c_str());
            if (station.contains("province")) stationProvince = QString::fromUtf8(station["province"].get<std::string>().c_str());
        }
        
        // Pobranie metadanych o czujniku
        QString paramName;
        QString paramFormula;
        int sensorId = -1;
        
        if (jsonData.contains("metadata") && jsonData["metadata"].contains("sensor")) {
            auto& sensor = jsonData["metadata"]["sensor"];
            if (sensor.contains("id")) sensorId = sensor["id"].get<int>();
            if (sensor.contains("paramName")) paramName = QString::fromUtf8(sensor["paramName"].get<std::string>().c_str());
            if (sensor.contains("paramFormula")) paramFormula = QString::fromUtf8(sensor["paramFormula"].get<std::string>().c_str());
        }
        
        // Pobranie pomiarów
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
        
        // Debug - wyświetlenie odczytanych nazw z polskimi znakami
        qDebug() << "Odczytano stację:" << stationName;
        qDebug() << "Miasto:" << stationCity;
        qDebug() << "Województwo:" << stationProvince;
        
        // Tymczasowa stacja i czujnik do pokazania w interfejsie
        Station tempStation;
        tempStation.id = stationId;
        tempStation.name = stationName.toUtf8().toStdString();
        tempStation.city = stationCity.toUtf8().toStdString();
        tempStation.province = stationProvince.toUtf8().toStdString();
        
        Sensor tempSensor;
        tempSensor.id = sensorId;
        tempSensor.paramName = paramName.toUtf8().toStdString();
        tempSensor.paramFormula = paramFormula.toUtf8().toStdString();
        
        // Dodaj tymczasową stację i czujnik bez usuwania istniejących danych
        // (zamiast zastępować całe wektory)
        bool stationExists = false;
        for (const auto& station : stations) {
            if (station.id == stationId) {
                stationExists = true;
                break;
            }
        }
        
        if (!stationExists) {
            stations.push_back(tempStation);
        }
        
        bool sensorExists = false;
        for (const auto& sensor : sensors) {
            if (sensor.id == sensorId) {
                sensorExists = true;
                break;
            }
        }
        
        if (!sensorExists) {
            sensors.push_back(tempSensor);
        }
        
        // Zapamiętaj indeksy, żeby wybrać te elementy w interfejsie
        int stationIndex = -1;
        int sensorIndex = -1;
        
        // Znajdź indeksy w ComboBox'ach
        for (int i = 0; i < stationComboBox->count(); i++) {
            if (stationComboBox->itemText(i).contains(stationName)) {
                stationIndex = i;
                break;
            }
        }
        
        for (int i = 0; i < sensorComboBox->count(); i++) {
            if (sensorComboBox->itemText(i).contains(paramName)) {
                sensorIndex = i;
                break;
            }
        }
        
        // Jeśli element nie istnieje, dodaj go do ComboBox'a
        if (stationIndex == -1) {
            stationComboBox->addItem(QString("%1 (%2, %3) [WCZYTANE Z PLIKU]").arg(
                stationName, stationCity, stationProvince));
            stationIndex = stationComboBox->count() - 1;
        }
        
        if (sensorIndex == -1) {
            sensorComboBox->addItem(QString("%1 (%2) [WCZYTANE Z PLIKU]").arg(
                paramName, paramFormula));
            sensorIndex = sensorComboBox->count() - 1;
        }
        
        // Ustaw aktualnie wybrany element
        stationComboBox->setCurrentIndex(stationIndex);
        sensorComboBox->setCurrentIndex(sensorIndex);
        
        // Wypełnienie tabeli danymi
        fillDataTable();
        
        // Aktywacja przycisków
        //showChartButton->setEnabled(true);
        saveButton->setEnabled(true);
        
        return true;
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Błąd", 
            QString("Wystąpił błąd podczas wczytywania pliku JSON: %1").arg(e.what()));
        return false;
    }
}