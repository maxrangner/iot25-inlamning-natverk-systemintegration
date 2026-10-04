# IoT25 - Inlämning Nätverk och Systemintegration

> Arbete pågår. Avsnitten fylls på under projektets gång.

## Syfte

En ESP32-C3 som mäter temperatur och luftfuktighet med en DHT11-sensormodul. Den skickar mätvärdena som JSON över MQTT med TLS till en Mosquitto-broker. En backend validerar mätningarna och gör dem tillgängliga via ett REST-API.

## Hårdvara

- ESP32-C3 supermini
- DHT11-sensormodul med inbyggd pull-up

| DHT11 | | ESP32-C3 |
|---|---| --- |
| VCC |-> | 3,3 V |
| GND |-> | GND |
| DATA |-> | GPIO4 |

## Beroenden

Utvecklingsdator:
- ESP-IDF v6.0.2. Sensordrivrutinen `esp-idf-lib/dht` hämtas automatiskt vid build.

Server (Ubuntu/Debian):
- Mosquitto, mosquitto-clients, Python 3 med venv, ufw

## Installation

Kommandona körs från repots rot. `<server-ip>` är serverns IP-adress på LAN.

### Server

```bash
sudo apt install mosquitto mosquitto-clients python3-venv ufw
git clone <repo-url>
cd iot25-inlamning-natverk-systemintegration
```

Cert:

```bash
scripts/gen-certs.sh <server-ip>
sudo mkdir -p /etc/mosquitto/certs
sudo cp certs/ca.crt certs/server.crt certs/server.key /etc/mosquitto/certs/
sudo chown mosquitto: /etc/mosquitto/certs/server.key
```

Mosquitto:

```bash
sudo cp deploy/mosquitto/iot25.conf /etc/mosquitto/conf.d/
sudo cp deploy/mosquitto/acl.example /etc/mosquitto/acl
sudo mosquitto_passwd -c /etc/mosquitto/passwd esp32-c3-01
sudo mosquitto_passwd /etc/mosquitto/passwd backend
sudo chown mosquitto: /etc/mosquitto/passwd
sudo chmod 600 /etc/mosquitto/passwd
```

Backend. `User` och sökvägarna i `iot25-backend.service` är för min server och behöver ändras:

```bash
python3 -m venv src/backend/.venv
src/backend/.venv/bin/pip install -r src/backend/requirements.txt
sudo mkdir -p /etc/iot25
sudo cp src/backend/.env.example /etc/iot25/backend.env
sudo chmod 600 /etc/iot25/backend.env
sudo cp deploy/systemd/iot25-backend.service /etc/systemd/system/
sudo systemctl daemon-reload
```

Brandvägg. Byt `192.168.50.0/24` mot ditt eget nät:

```bash
sudo ufw default deny incoming
sudo ufw allow from 192.168.50.0/24 to any port 8883 proto tcp
sudo ufw allow from 192.168.50.0/24 to any port 8000 proto tcp
sudo ufw allow from 192.168.50.0/24 to any port 22 proto tcp
sudo ufw enable
```

### ESP32

CA-certet byggs in i firmwaren och måste kopieras från servern:

```bash
scp <server-ip>:<repo>/certs/ca.crt src/firmware/certs/ca.crt
```

## Konfiguration

ESP32: `idf.py menuconfig` i `src/firmware`, under `IoT25 configuration`. Värdena sparas i `sdkconfig`, som inte följer med git.

| Inställning | Värde |
|---|---|
| Wi-Fi SSID / Wi-Fi password | Ditt Wi-Fi |
| MQTT Broker | `mqtts://<server-ip>:8883` |
| MQTT username | `esp32-c3-01` |
| MQTT password | Lösenordet från `mosquitto_passwd` |

Backend: `/etc/iot25/backend.env`.

| Variabel | Värde |
|---|---|
| `MQTT_HOST` | `127.0.0.1` |
| `MQTT_PORT` | `1883` |
| `MQTT_USERNAME` | `backend` |
| `MQTT_PASSWORD` | Lösenordet från `mosquitto_passwd` |

## Starta lösningen

```bash
# Server
sudo systemctl restart mosquitto
sudo systemctl enable --now iot25-backend

# ESP32, i src/firmware
idf.py build flash monitor
```

## Verifiera dataflödet

1. ESP32-monitorn visar `Mqtt connected` och en mätning var 10:e sekund.
2. `mosquitto_sub --cafile certs/ca.crt -h <server-ip> -p 8883 -u backend -P <lösenord> -t 'iot25/max/#' -v` visar meddelandena.
3. `journalctl -u iot25-backend -f` visar `event=reading_accepted`.
4. `curl http://<server-ip>:8000/api/status` visar `deviceOnline: true`, `stale: false` och ökande `messagesAccepted`.

## Kända begränsningar

- Backend sparar bara senaste mätningen, i minnet. Uppgiften krävde ingen historik. En databas som SQLite skulle ge historik som klarar omstarter.
- Mätningar som tas medan ESP32an är offline slängs.
- DHT11 mäter bara hela grader och procent, ±2 °C och ±5 %. En SHT31 vore noggrannare.
- Servercertet innehåller serverns IP. Byts IP:n måste certet skapas om.
- Säkerhetsbegränsningar finns i [sakerhet.md](docs/sakerhet.md#kvarvarande-begränsningar).

## Dokumentation

- [Arkitektur](docs/arkitektur.md)
- [API](docs/api.md)
- [Säkerhet](docs/sakerhet.md)
- [Felsökning](docs/felsokning.md)
- [Testprotokoll](docs/testprotokoll.md)
