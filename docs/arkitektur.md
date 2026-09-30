# Arkitektur

## Översikt

```
DHT11 ─GPIO4─ ESP32-C3 ──MQTTS :8883──▶ Mosquitto ──MQTT 127.0.0.1:1883──▶ Backend (FastAPI)
                                                                             │ HTTP :8000
                                                                             ▼
                                                                    curl / webbläsare (LAN)
```

1. ESP32 läser sensorn var 10:e sekund och skickar två JSON-meddelanden, ett för temperatur och ett för luftfuktighet, till Mosquitto över MQTT med TLS.
2. Mosquitto kontrollerar inloggning och behörighet och skickar meddelandena vidare till backend.
3. Backend kontrollerar varje mätning och sparar senaste värdet per sensor i minnet.
4. Andra system hämtar datan från backendens REST-API.

Mosquitto och backend körs på samma server, `orange-box`, som systemd-tjänster.

## Komponenter

### ESP32-C3 (ESP-IDF v6.0.2, C++)

| Modul | Ansvar |
|---|---|
| `wifi` | Ansluter till Wi-Fi, återansluter vid avbrott och synkar klockan med SNTP |
| `dht11` | Läser sensorn var 10:e sekund och gör om avläsningen till två mätningar |
| `mqtt` | Ansluter till brokern över TLS, publicerar status och mätningar som JSON |

`dht11` vet vad som mäts, och `mqtt` vet hur det skickas. Sensordelen känner inte till några topics.

### Mosquitto

Tar emot meddelanden från ESP32 och skickar dem vidare till backend. Mosquitto har två lyssnare:

- **8883** med TLS, för ESP32. Nås bara från LAN.
- **1883** utan TLS, bara på `127.0.0.1`, för backend. Nås inte utifrån.

Båda kräver användare och lösenord. Konfigurationen ligger i `deploy/mosquitto/`.

### Backend (Python, FastAPI, paho-mqtt)

| Fil | Ansvar |
|---|---|
| `config.py` | Läser inställningarna från miljövariabler |
| `validation.py` | Kontrollerar att en mätning följer datakontraktet |
| `store.py` | Håller senaste mätningen per sensor och räknarna för övervakning |
| `mqtt_client.py` | Prenumererar, återansluter och skickar mätningarna vidare till validering och lagring |
| `main.py` | REST-API:t |

MQTT-klienten körs i en egen tråd och API:t i en annan. De delar bara lagringen, som skyddas med ett lock.

Backend sparar bara senaste värdet och inget på disk. Uppgiften kräver att datan ska vara tillgänglig, inte historik, så jag valde det enklaste.

## Loggning och övervakning

**Backend** loggar till journald (`journalctl -u iot25-backend`). Varje rad har formatet `event=<namn>` följt av värden, så att loggen är lätt att filtrera med `grep`:

```
INFO event=reading_accepted sensorId=room-a-temp-01 value=22.0 unit=C
WARNING event=reading_rejected category=syntax topic=iot25/max/sensor/room-a-temp-01 detail=payload is not valid JSON payload=b'{bad'
INFO event=api_request method=GET path=/api/readings/room-a-temp-01 status=200 duration_ms=0
```

Följande loggas:
- start och stopp
- MQTT-anslutning, frånkoppling och misslyckade försök
- godkända och nekade mätningar
- ändrad enhetsstatus
- alla API-anrop

Normalt flöde loggas som INFO, nekade mätningar och frånkoppling som WARNING, och kommunikationsfel som ERROR.

**ESP32** loggar med `ESP_LOGI`/`W`/`E` och en tagg per modul (`wifi`, `mqtt`, `dht11`).

**Övervakning:** `GET /api/status` visar bland annat:
- om backend är ansluten till brokern
- om ESP32 är online
- antal mottagna och godkända meddelanden
- valideringsfel per kategori
- antal återanslutningar
- tid sedan senaste mätningen

Har ingen mätning kommit på 30 s sätts `stale` till `true`. Se [api.md](api.md#get-apistatus).

## IP-adresser och portar

| Enhet | IP-adress |
|---|---|
| Server `orange-box` | `192.168.50.150` (DHCP-reservation) |
| ESP32-C3 | `192.168.50.115` (DHCP) |
| LAN | `192.168.50.0/24` |

| Port | Protokoll | Används av | Åtkomst |
|---|---|---|---|
| 8883 | MQTT över TLS | ESP32 → Mosquitto | Bara LAN |
| 1883 | MQTT | Backend → Mosquitto | Bara lokalt på servern |
| 8000 | HTTP | Klienter → backend | Bara LAN |

## Kommunikationsmodell och topics

MQTT med publish/subscribe. ESP32 publicerar och backend prenumererar.

| Topic | Innehåll | QoS | Retained |
|---|---|---|---|
| `iot25/max/sensor/<sensorId>` | En mätning som JSON | 1 | Nej |
| `iot25/max/status/esp32-c3-01` | `online` eller `offline` | 1 | Ja |

`sensorId` är `room-a-temp-01` för temperatur och `room-a-hum-01` för luftfuktighet. Backend prenumererar på `iot25/max/sensor/+`.

**Status:** ESP32 publicerar `online` varje gång den ansluter. Vid anslutningen registrerar den också `offline` som Last Will, som brokern själv publicerar om enheten försvinner. Status är retained, så en ny prenumerant ser direkt om enheten är uppe.

**Mätningar är inte retained.** En gammal mätning skulle annars se ny ut för backend efter en omstart.

## Dataformat

En mätning per meddelande, som JSON:

```json
{"sensorId":"room-a-temp-01", "timestamp":"2026-09-29T10:30:00Z", "value":22.00, "unit":"C"}
```

Tiden kommer från SNTP och skickas i UTC. Hela datakontraktet och valideringsreglerna finns i [api.md](api.md#mqtt-payload).

JSON byggs med `snprintf`. Payloaden har bara fyra fält, så ett JSON-bibliotek hade varit överflödigt.

## Återanslutning och felhantering

**Wi-Fi.** Tappar ESP32 anslutningen görs ett nytt försök med exponentiell backoff: 1 s, 2 s, 4 s och så vidare, upp till 60 s.

**MQTT på ESP32.** esp-mqtt försöker igen var 10:e sekund, med ny TLS-handskakning och ny inloggning varje gång. Mätningar som tas medan enheten är offline slängs i stället för att buffras. De skulle annars skickas i klump efter återanslutningen, och bara senaste värdet är intressant.

**TLS.** Det finns ingen okrypterad reserv. Är broker-URI:n inte `mqtts://` startar MQTT inte alls. Certets giltighetstid kontrolleras, och därför misslyckas första försöket efter start innan klockan är synkad. Nästa försök lyckas.

**Sensorn.** Misslyckas en avläsning loggas en varning och avläsningen hoppas över. Innan klockan är synkad skickas ingenting, så att inga mätningar med tidsstämplar från 1970 når backend.

**Backend.**
- Backend startar även om brokern är nere och ansluter när den kommer upp.
- Tappas anslutningen försöker paho igen med växande väntetid (1–60 s) och prenumererar på nytt vid varje anslutning.
- En felaktig mätning loggas och räknas, men påverkar inte tjänsten.
- Kraschar processen startar systemd om den efter 5 s.

`deviceOnline` och `stale` i status kompletterar varandra. `deviceOnline` visar om ESP32 är ansluten, och `stale` visar om det faktiskt kommer data. Är enheten online men datan inaktuell ligger felet troligen i sensorn.

## Varför MQTT

- ESP32 behöver inte veta vem som läser datan. Fler mottagare kan läggas till utan att firmwaren ändras.
- Litet overhead och en anslutning som hålls öppen, vilket passar en liten enhet.
- QoS 1 ger leveransgaranti.
- Last Will och retained ger enhetsstatus utan polling.
- TLS och inloggning finns inbyggt i Mosquitto.

## Varför REST

Andra system vill kunna fråga efter data när de behöver den, utan att hålla en MQTT-anslutning öppen. Ett REST-API över HTTP fungerar med `curl`, en webbläsare eller vilket språk som helst.

API:t har bara `GET`, eftersom det bara visar data. Backend blir bryggan mellan MQTT, som passar enheten, och HTTP, som passar klienterna.
