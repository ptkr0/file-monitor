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
./filemonitor [OPT: -R] [ŚCIEŻKA_KATALOGU_ŹRÓDŁOWEGO] [ŚCIEŻKA_KATALOGU_DOCELOWEGO] [PRÓG_WIELKOŚCI]:[CZAS_POMIĘDZY_SKANOWANIEM]
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

## Różnica pomiędzy kopiowaniem read/write a sendfile

Nasz program wykorzystuje 2 metody kopiowania plików. Dla mniejszych jest to read/write, zaś dla większych sendfile.
Sendfile jest rozwiązaniem szybszym dla dużych plików, ponieważ omija buforowanie danych w przestrzeni użytkownika.
Przeprowadziliśmy prosty test kopiując plik o wielkości 100MB przy użyciu 2 metod:

read/write:
```sh
real	0m0,220s
user	0m0,013s
sys	0m0,183s
```

sendfile:
```sh
real	0m0,067s
user	0m0,000s
sys	0m0,062s
```

Jak widać różnica jest znaczna co przy kopiowaniu wielu różnych plików w niewielkich odstępach czasu ma ogromne znaczenie.

## Autorzy

Piotr Radziszewski\
Dawid Szymański


