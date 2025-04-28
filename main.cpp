#include <QApplication>
#include <QIcon>
#include <QSplashScreen>
#include <QTimer>
#include "main_window.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Ustawienie informacji o aplikacji
    QApplication::setApplicationName("Monitor Jakości Powietrza");
    QApplication::setApplicationVersion("1.0");
    
    // Ustawienie ikony aplikacji
    app.setWindowIcon(QIcon("../icons/app_icon.png"));
    
    // Ekran powitalny
    QSplashScreen splash(QPixmap("../welcome.png"));
    splash.show();
    
    // Inicjalizacja głównego okna
    MainWindow mainWindow;
    
    // Pokazanie głównego okna po krótkim opóźnieniu
    QTimer::singleShot(1500, &splash, &QSplashScreen::close);
    QTimer::singleShot(1500, &mainWindow, &MainWindow::show);
    
    return app.exec();
}