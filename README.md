# IoT25 - Inlämning Nätverk och Systemintegration

> Arbete pågår. Avsnitten fylls på under projektets gång.

## Syfte

En ESP32-C3 mäter temperatur och luftfuktighet med en DHT11. Skickar mätvärdena som JSON över MQTT med TLS till en Mosquitto-broker. En backend validerar mätningarna och gör dem tillgängliga via ett REST-API med övervakningsmått.

## Hårdvara

- ESP32-C3
- DHT11: temperatur- och luftfuktighetssensor (0-50 °C ±2, 20-90 % ±5)

### Koppling

| DHT11 | ESP32-C3 |
|---|---|
| VCC | 3,3 V |
| GND | GND |
| DATA | GPIO4 |

DHT11 använder ett en-tråds-protokoll och behöver en pull-up på 10 kΩ mellan DATA och 3,3 V. De flesta färdiga moduler har den redan pålödd; en lös sensor behöver en egen.

GPIO4 är vald för att den är fri och inte är en strapping-pinne. GPIO2, GPIO8 och GPIO9 läses av vid uppstart och undviks därför.

Sensorn läses var 10:e sekund, vilket är väl över DHT11:s minsta samplingsintervall på 1 s.

## Arkitektur

Se [docs/arkitektur.md](docs/arkitektur.md).

## Beroenden

### Utvecklingsmiljö (Windows)

- ESP-IDF v6.0.2 (installerad i `C:\esp\v6.0.2`) med VS Code-tillägget `espressif.esp-idf-extension`
- Python 3 för backend och tester
- Sensordrivrutinen `esp-idf-lib/dht` hämtas automatiskt vid build, ingen manuell installation behövs

### Server (Ubuntu/Debian)

- Mosquitto, mosquitto-clients
- Python 3 med venv

## Installation

## Konfiguration

## Starta lösningen

## Verifiera dataflödet

## Kända begränsningar

## Dokumentation

- [Arkitektur](docs/arkitektur.md)
- [API](docs/api.md)
- [Säkerhet](docs/sakerhet.md)
- [Felsökning](docs/felsokning.md)
- [Testprotokoll](docs/testprotokoll.md)
