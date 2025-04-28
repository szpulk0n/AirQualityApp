# Monitor Jakości Powietrza

Aplikacja desktopowa do monitorowania jakości powietrza wykorzystująca API Głównego Inspektoratu Ochrony Środowiska (GIOŚ).

## Opis

Monitor Jakości Powietrza to aplikacja, która umożliwia użytkownikowi przeglądanie danych o jakości powietrza z różnych stacji pomiarowych w Polsce. Aplikacja pobiera dane z oficjalnego API GIOŚ, prezentuje je w formie tabel i wykresów oraz umożliwia zapisywanie danych do plików JSON w celu późniejszej analizy.

![Screenshot aplikacji](welcome.png)

## Funkcje

- Pobieranie listy stacji pomiarowych z całej Polski
- Wyświetlanie czujników dostępnych na wybranej stacji
- Wyświetlanie pomiarów dla wybranego czujnika lub wszystkich czujników
- Wizualizacja danych w formie tabeli i wykresów
- Eksport danych do plików JSON
- Import wcześniej zapisanych danych
- Obsługa trybu offline dzięki mechanizmom cachowania
- Kolorystyczne rozróżnianie parametrów na wykresach
- Elegancki interfejs użytkownika z efektami wizualnymi (cienie, zaokrąglone rogi)

## Wymagania systemowe

- System operacyjny: Windows, Linux lub macOS
- Procesor: 1 GHz lub szybszy
- Pamięć RAM: minimum 512 MB
- Miejsce na dysku: minimum 50 MB
- Połączenie internetowe (opcjonalnie - dla pobierania danych na żywo)

## Wymagania techniczne

- C++17 lub nowszy
- Qt 5.12 lub nowszy
- libcurl
- nlohmann/json
- GoogleTest (do testów jednostkowych)
- CMake 3.14 lub nowszy

## Instalacja

### Kompilacja ze źródeł

1. Sklonuj repozytorium:
```bash
git clone https://github.com/twojlogin/monitor-jakosci-powietrza.git
cd monitor-jakosci-powietrza
```

2. Utwórz katalog build i skompiluj projekt:
```bash
mkdir build
cd build
cmake ..
make
```

3. Uruchom aplikację:
```bash
./AirQualityApp
```

### Opcje CMake

Projekt zawiera następujące opcje CMake:

- `-DBUILD_TESTS=OFF` - wyłączenie kompilacji testów
- `-DBUILD_DOCS=OFF` - wyłączenie generowania dokumentacji

## Struktura projektu

- `main.cpp` - punkt wejścia aplikacji
- `src/api_client.cpp`, `include/api_client.hpp` - klasa do komunikacji z API GIOŚ
- `src/main_window.cpp`, `include/main_window.hpp` - główne okno aplikacji
- `tests/` - testy jednostkowe z użyciem Google Test
- `data/` - katalog do przechowywania lokalnych kopii danych
- `export/` - domyślny katalog na eksportowane pliki JSON
- `docs/` - automatycznie generowana dokumentacja (Doxygen)

## Testowanie

Aplikacja zawiera testy jednostkowe napisane z wykorzystaniem frameworka GoogleTest. Aby uruchomić testy:

```bash
cd build
make test
```

lub bezpośrednio:

```bash
./tests/test_api_client
./tests/test_main_window
```

## Dokumentacja

Dokumentacja kodu jest generowana przy użyciu narzędzia Doxygen. Po zbudowaniu projektu z włączoną opcją dokumentacji, możesz ją przeglądać otwierając plik `build/docs/html/index.html` w przeglądarce.

Aby wygenerować dokumentację:

```bash
cd build
make docs
```

## API GIOŚ

Aplikacja korzysta z publicznego API Głównego Inspektoratu Ochrony Środowiska dostępnego pod adresem:
```
http://api.gios.gov.pl/pjp-api/rest
```

Endpointy używane przez aplikację:
- `/station/findAll` - pobieranie listy stacji pomiarowych
- `/station/sensors/{stationId}` - pobieranie czujników dla danej stacji
- `/data/getData/{sensorId}` - pobieranie danych pomiarowych dla czujnika

## Znane problemy

- Aplikacja obecnie obsługuje tylko język polski
- Niektóre ścieżki plików są zdefiniowane na stałe
- Wyłączona weryfikacja SSL podczas komunikacji z API (w przypadku HTTPS)
- Wyświetlanie dużej ilości danych może wpływać na wydajność aplikacji

## Planowane funkcje

- Obsługa wielu języków
- Eksport danych do formatu CSV
- Filtrowanie danych według zakresu dat
- Porównywanie danych z różnych stacji na jednym wykresie
- Mapa Polski z zaznaczonymi stacjami pomiarowymi
- Powiadomienia o przekroczeniu norm jakości powietrza

## Rozwiązywanie problemów

### Problem z połączeniem do API

Jeśli aplikacja nie może nawiązać połączenia z API GIOŚ:
1. Sprawdź swoje połączenie internetowe
2. Upewnij się, że firewall nie blokuje aplikacji
3. API GIOŚ może być czasowo niedostępne - spróbuj ponownie później
4. Skorzystaj z danych zapisanych lokalnie przez wybranie opcji "Przeglądaj zapisane dane"

### Problemy z kompilacją

1. Upewnij się, że masz zainstalowane wszystkie wymagane zależności
2. Sprawdź, czy używasz kompatybilnej wersji Qt (5.12 lub nowszej)
3. Upewnij się, że masz wystarczające uprawnienia do tworzenia katalogów

## Licencja

Ten projekt jest udostępniany na licencji MIT. Szczegóły w pliku LICENSE.

## Autor

Konrad Szpulecki

## Podziękowania

- Głównemu Inspektoratowi Ochrony Środowiska za udostępnienie API
- Twórcom bibliotek Qt, libcurl i nlohmann/json
- Społeczności C++ za wsparcie i inspirację