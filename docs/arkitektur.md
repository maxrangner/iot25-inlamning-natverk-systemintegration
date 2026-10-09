# Arkitektur

## Översikt

```
DHT11 ─> GPIO4 ─> ESP32-C3 ─> MQTTS :8883 ─> Mosquitto ─> MQTT 127.0.0.1:1883 ─> Backend (FastAPI)
                                                                              │   HTTP :8000
                                                                              v
                                                                    curl / webbläsare (LAN)
```

Mosquitto och backend körs på samma server som systemd-tjänster.

## Komponenter

### ESP32-C3 (ESP-IDF v6.0.2, C++)

| Modul | Ansvar |
|---|---|
| `wifi` | Ansluter till Wi-Fi, återansluter vid avbrott. Synkar klockan med SNTP |
| `dht11` | Läser sensorn var 10:e sekund och gör om avläsningen till två mätningar |
| `mqtt` | Ansluter till brokern över TLS. Publicerar status och mätningar som JSON |

### Mosquitto

Tar emot meddelanden från ESP32 och skickar dem vidare till backend. Kräver användare och lösenord.

### Backend (Python, FastAPI, paho-mqtt)

| Fil | Ansvar |
|---|---|
| `config.py` | Läser inställningarna från miljövariabler |
| `validation.py` | Kontrollerar att en mätning följer datakontraktet |
| `store.py` | Håller senaste mätningen per sensor och räknarna för övervakning |
| `mqtt_client.py` | Prenumererar, återansluter och skickar mätningarna vidare till validering och lagring |
| `main.py` | REST-API:t |

## IP-adresser och portar

Dessa IP-adresser och namn är för min utvecklingsmiljö.

| Enhet | IP-adress |
|---|---|
| Server `orange-box` | `192.168.50.150` (DHCP-reservation) |
| ESP32-C3 | DHCP |
| LAN | `192.168.50.0/24` |

| Port | Protokoll | Används av | Åtkomst |
|---|---|---|---|
| 8883 | MQTT över TLS | ESP32 -> Mosquitto | Bara LAN |
| 1883 | MQTT | Backend -> Mosquitto | Bara lokalt på servern |
| 8000 | HTTP | Klienter -> backend | Bara LAN |

## Kommunikationsmodell och topics

MQTT med publish/subscribe. ESP32 publicerar och backend prenumererar.

| Topic | Innehåll | QoS | Retained |
|---|---|---|---|
| `iot25/max/sensor/<sensorId>` | En mätning som JSON | 1 | Nej |
| `iot25/max/status/esp32-c3-01` | `online` eller `offline` | 1 | Ja |

ESP32 publicerar `online` när den ansluter och registrerar `offline` som Last Will, som brokern publicerar om enheten försvinner.

Mätningar är inte retained. En gammal mätning skulle annars se ny ut för backend efter en omstart.

## Dataformat

JSON, en mätning per meddelande. Datakontraktet finns i [api.md](api.md#mqtt-payload).

```json
{"sensorId":"room-a-temp-01", "timestamp":"2026-09-29T10:30:00Z", "value":22.00, "unit":"C"}
```

## Återanslutning och felhantering

### Wi-Fi
Exponentiell backoff: 1 s, 2 s, 4 s osv, upp till 60 s.

### MQTT på ESP32
esp-mqtt försöker igen var 10:e sekund. Mätningar som tas medan enheten är offline slängs i stället för att buffras, eftersom bara senaste värdet är intressant.

### TLS
Certets giltighetstid kontrolleras, så första försöket efter start misslyckas innan klockan är synkad. Nästa försök lyckas.

### Sensorn
Misslyckade avläsningar loggas och hoppas över. Innan klockan är synkad skickas ingenting.

### Backend
- Startar även om brokern är nere.
- paho återansluter med växande väntetid (1-60 s) och prenumererar på nytt.
- En felaktig mätning loggas och räknas, men påverkar inte tjänsten.
- systemd startar om processen efter 5 s om den kraschar.

## Loggning och övervakning

Backend loggar till journald (`journalctl -u iot25-backend`) med formatet `event=<namn>`, så att loggen går att filtrera med `grep`:

```
INFO event=reading_accepted sensorId=room-a-temp-01 value=22.0 unit=C
WARNING event=reading_rejected category=syntax topic=iot25/max/sensor/room-a-temp-01 detail=payload is not valid JSON payload=b'{bad'
INFO event=api_request method=GET path=/api/readings/room-a-temp-01 status=200 duration_ms=0
```

Anslutningar, frånkopplingar, mätningar, valideringsfel, enhetsstatus och API-anrop loggas. ESP32 loggar med `ESP_LOGI`/`W`/`E` och en tagg per modul.

Övervakningsmåtten finns i [`GET /api/status`](api.md#get-apistatus). Är `deviceOnline` true men `stale` också true ligger felet troligen i sensorn.

## Varför MQTT

- ESP32 behöver inte veta vem som läser datan. Fler mottagare kan läggas till utan att firmwaren ändras.
- Litet overhead och en anslutning som hålls öppen. Det passar en liten enhet.
- QoS 1 gör att brokern kvitterar mottagna meddelanden. Mätningar som tas medan ESP32 är frånkopplad sparas inte. Last Will ger enhetsstatus utan polling.
- TLS och inloggning finns inbyggt i Mosquitto.

Alternativet hade varit att ESP32 skickar varje mätning med HTTP POST direkt till backend. Då hade den behövt veta backendens adress, och det hade inte funnits någon Last Will som visar om den försvunnit.

## Varför REST

Andra system kan hämta data när de behöver den, utan att hålla en MQTT-anslutning öppen. REST över HTTP fungerar med `curl`, en webbläsare eller vilket språk som helst.
