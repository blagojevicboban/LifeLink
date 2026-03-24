# LifeLink Pametni Sat - ESP32-S3

[🇬🇧 English documentation is available in README.md](README.md)

LifeLink je napredni prototip pametnog sata izgrađen na **ESP32-S3** platformi. Koristi **ESP-IDF** u kombinaciji sa grafičkom bibliotekom **LVGL** za iscrtavanje prelepog korisničkog interfejsa na okruglom AMOLED ekranu rezolucije 466x466 piksela. Prevenstveno je fokusiran na brigu o starijim i ugroženim licima, praćenje zdravstvenih parametara i brzo reagovanje u hitnim situacijama.

## Glavne Funkcionalnosti

- **Napredna Detekcija Pada**: Koristi QMI8658 IMU (Akcelerometar + Žiroskop) za otkrivanje naglih padova i jakih udaraca o tlo. Zahteva period zadržavanja u nepomičnom stanju i specifičnu promenu ugla nagiba nakon udara kako bi potvrdio pravi pad a izbegao lažne uzbune.
- **Optimizovan MAX30102 Algoritam**: Precizno očitavanje pulsa i kiseonika (SpO2) koristeći FFT (Brzu Furijeovu Transformaciju) na prozoru od 100Hz.
  - **Odbacivanje šuma/artefakata**: Filtrira niske frekvencije (ispod 45 BPM) i fiziološki nemoguće vrednosti (preko 220 BPM).
  - **Reflektivna kalibracija**: Prilagođena formula za SpO2 (`104 - 17*R`) optimizovana za merenje na zglobu ruke.
- **Autonomni Hitni Odgovor**: Sat može raditi potpuno nezavisno od mobilne aplikacije u hitnim situacijama:
  - **Sekvencijalni SMS**: Šalje SOS poruke na više brojeva hitnih kontakata.
  - **Sekvencijalni Pozivi**: Automatski poziva kontakte po prioritetu ako se detektuje pad.
  - **SOS Poziv**: Brzo biranje primarnog hitnog broja.
- **I2C Fast Mode (400kHz)**: Optimizovana brzina komunikacije između procesora i svih senzora (MAX30102, QMI8658, AXP2101) za maksimalnu stabilnost podataka.
- **Automatski GSM SMS Alarmi**: Komunicira sa SIM800L/A6 GSM Modulom kako bi asinhrono poslao SMS upozorenja koja sadrže:
  - Precizne GPS koordinate formatirane kao direktan Google Maps link.
  - Otkucaje srca u realnom vremenu u trenutku pada.
  - Informaciju o tipu pada (stvaran ili simuliran).
- **WiFi Sinhronizacija sa Cloud-om**: Direktno povezivanje na WiFi i slanje zdravstvenih snapshot-ova i GPS koordinata u Firestore bazu svakih 30 sekundi radi daljinskog praćenja.
- **Automatska Sinhronizacija Vremena (GSM/NITZ)**: Sat automatski preuzima tačno lokalno vreme od operatera mobilne mreže čim se registruje, eliminišući potrebu za ručnim podešavanjem sata ili internet konekcijom.
- **Always-On Display (AOD)**: Štedljivi režim rada koji prikazuje vreme na zatamnjenoj crnoj pozadini kada je sat u stanju mirovanja, produžavajući trajanje baterije.
- **NMEA GPS Podrška**: Integrisano čitanje NMEA protokola sa eksternih GPS modula za precizno praćenje lokacije.

## Prateća Mobilna Aplikacija (Flutter)

Cross-platform **Flutter** prateća aplikacija proširuje mogućnosti LifeLink sistema putem Bluetooth Low Energy (BLE) konekcije:

- **Dashboard Uživo**: Prikaz vitalnih parametara u realnom vremenu — puls (BPM), SpO2, G-sila i GPS lokacija preslikani sa sata.
- **BLE Povezivanje**: Automatsko ili manuelno uparivanje sa LifeLink ESP32 satom putem BLE SPP protokola.
- **Hitni Odgovor**: Konfigurisane akcije pri padu — direktan telefonski **poziv**, **SMS** sa GPS koordinatama ili sistemski **SOS** signal.
- **Ogledalo Detekcije Pada**: Aplikacija preslikava 3-faznu detekciju pada sa sata (Bezbedno → Upozorenje → Alarm) sa 5-sekundnim odbrojavanjem i haptičkim/zvučnim alarmom.
- **Podešavanja**: Konfiguracija hitnog kontakta, tipa akcije pada, trajanja odbrojavanja i MAC adrese uređaja.
- **Interaktivna Mapa**: Prikaz lokacije korisnika na OpenStreetMap mapi za pomoć spasiocima.

## Hardver

- **Mikrokontroler**: ESP32-S3
- **Displej**: Okrugli AMOLED ekran (466x466)
- **Mreža / Komunikacija**: SIM800L GSM Modul (Komunikacija bazirana na AT Komandama, napajan direktno sa 3.7V Li-Ion baterije)
- **IMU Senzori**: QMI8658 (Praćenje pokreta i nagiba)
- **Senzori Zdravlja**: MAX30102 (Otkucaji srca i SpO2)
- **Power Management (Baterija i Struja)**: AXP2101

## Podešavanje i Pokretanje

Ovaj projekat je izgrađen i napisan u jezicima C i C++, preko [Espressif ESP-IDF frejmvorka](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) (v5.x i vise je preporuka).

### 1. Konfiguracija
Setujte vaš procesor na ESP32-S3 i uđite u meni opcije kako bi uverili konfiguracije:
```bash
idf.py set-target esp32s3
idf.py menuconfig
```
### 2. Građenje arhitekture i Flešovanje
Kompilirajte kod i prebacite softver na mikrokontroler:
```bash
idf.py build
idf.py -p COMX flash monitor
```

 *(COMX podesite na port vašeg esp programatora)*

## Pregled i mapiranje Ekrana

1. **Glavni Skrin (Ekran 1)**: Brojčanik (Sat), glavni vitali i najosnovnije konektivne ikonice.
2. **Prikaz Senzora (Ekran 2)**: Test dugme za simulaciju pada bez prave povrede, uz Debug panel parametara žiroskopa za stručno lice.
3. **Podešavanja (Ekran 3)**: Prikaza ogromne numeričke tastature gde prstima svako može uneti pretplatnički broj mobilnog telefona i sačuvati podešavanje u obezbeđenu RAM particiju sata bez ometanja. 
4. **Ekran u hitnim situacijama (Ekran 4)**: Alarmantan crveni ekran, koji glasnim i krupnim tekstom nudi korisniku obaranje upozorenja ako on stoji i zapravo je dobro. 

## Odstranjivanje grešaka (Troubleshooting)

### Problem sa GSM SIM800L modulom: `+CREG: 1,3` i nasumični `+CPIN: NO SIM` logovi
Ako uređaj ne uspeva da se registruje na mrežu, a serijski monitor u petlji izbacuje `Network not registered yet. CREG status: +CREG: 1,3` (Registration Denied) prećeno sa `+CPIN: NO SIM` ili `+CME ERROR: 256`, problem leži u **nedovoljno snažnom napajanju** modula.
- **Šta se dešava:** Prilikom pokušaja registracije na baznu stanicu GSM (2G) mreže, RF pojačivač unutar SIM800L modula naglo povuče i do **2 ampera** (2A peak current) u kratkom piku (burst). Ukoliko napajanje ne može da isporuči tu količinu čiste struje momentalno, napon pada i dešava se tkz. *"Brownout reset"*.
- **Kako popraviti (Rešenje):**
  1. SIM800L radi na **3.7–4.2V** i napaja se direktno sa Li-Ion baterije — nije potreban boost konvertor.
  2. Zalemiti **1000µF 10V elektrolitski kondenzator** i **100nF keramički kondenzator** paralelno, direktno na VCC i GND pinove SIM800L modula. Elektrolitski apsorbuje 2A strujne pikove, keramički filtrira visokofrekventne smetnje.
  3. Koristite deblje (manjeg otpora) napojne kablove između baterije i SIM800L modula.
  4. Uverite se da SIM kartica nije 4G-only u mreži vašeg operatera i da nema aktivan PIN kod.
  5. Softver sadrži automatski recovery mehanizam — nakon 3 uzastopna neuspeha, GSM modul se automatski restartuje.
