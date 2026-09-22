# IoT25 - Inlämning Nätverk och Systemintegration

> Arbete pågår. Avsnitten fylls på under projektets gång.

## Syfte

En ESP32-C3 mäter temperatur och luftfuktighet med en DHT11. Skickar mätvärdena som JSON över MQTT med TLS till en Mosquitto-broker. En backend validerar mätningarna och gör dem tillgängliga via ett REST-API med övervakningsmått.

## Hårdvara

- ESP32-C3
- DHT11: temperatur- och luftfuktighetssensor

## Arkitektur

Se [docs/arkitektur.md](docs/arkitektur.md).

## Beroenden

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
