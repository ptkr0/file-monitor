# FileMonitor

FileMonitor to linuxowy demon, który pozwala na kontrolę plików w katalogu.

## Cechy

- Opcjonalne ustawienienie czasu pomiędzy kontrolą plików
- Możliwość ustawienia limitu, pomiędzy którym demon kopiuje pliki przy użyciu read/write, a sendfile
- Opcja -r pozwala na rekurencyjną synchronizację katalogów
- Możliwość manualnego wykonania synchronizacji katalogów poprzez wysłanie sygnału do demona
- Kontrola działania demona w pliku syslog
- Kontrola plików poprzez sprawdzanie sum kontrolnych biblioteki OpenSSL

## Kompilacja Kodu

Do poprawnej kompilacji demona potrzebny jest pakiet OpenSSL w wersji 1.0.2:
```sh
sudo apt-get install libssl-dev
```

Następnie wystarczy skorzystać z pliku makefile:

```sh
make
```

## Uruchamianie

Aby poprawnie uruchomić demona należy skorzystać z następującej składni:

```sh
./filemonitor [ŚCIEŻKA_KATALOGU_ŹRÓDŁOWEGO] [ŚCIEŻKA_KATALOGU_DOCELOWEGO] [PRÓG_WIELKOŚCI]:[CZAS_POMIĘDZY_SKANOWANIEM] [OPT: -R]
```

- Opcja -R uruchamiania rekurencyjną synchronizację plików
- Opcje [PRÓG_WIELKOŚCI] i [CZAS_POMIĘDZY_SKANOWANIEM] są opcjonalne -- zostawienie tych pól pustych będzie skutkowało użyciem domyślnych wartości (10MB i 5 minut)

## Działanie programu

Pracę demona można kontrolować poprzez przeglądanie pliku syslog

```sh
sudo cat /var/log/syslog
```

Aby wysłać żądanie synchronizacji plików należy użyć następującego polecenia:

```sh
kill -SIGUSR1 [PID_DEMONA]
```

## Autorzy

Piotr Radziszewski
Dawid Szymański


