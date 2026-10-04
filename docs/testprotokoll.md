# Testprotokoll

End-to-end-tester som körts under arbetet, i den ordning delarna byggdes.

## ESP32, Wi-Fi och broker

| # | Test | Förväntat | Observerat | OK |
|---|---|---|---|---|
| 1 | Läsa DHT11 i en halv minut | Rimliga värden var 10:e s | 29 avläsningar i rad, 22,0 °C och 48 % | ✓ |
| 2 | Dra ur sensorns datakabel | Varning, ingen krasch | `Error reading sensor` vid varje intervall, ingen omstart | ✓ |
| 3 | Ansluta till Wi-Fi | IP-adress från DHCP | `got ip 192.168.50.115` | ✓ |
| 4 | Stänga av routern en stund | ESP32 återansluter själv | Frånkoppling loggades, återanslöt när routern kom tillbaka | ✓ |
| 5 | ESP32 ansluter till brokern | `online` på status-topicen | `Mqtt connected`, `iot25/max/status/esp32-c3-01 online` | ✓ |
| 6 | Brokern nere när ESP32 startar | Felet loggas, nytt försök | `Error transport connect`, anslöt vid nästa försök 10 s senare | ✓ |
| 7 | Bryta strömmen till ESP32 | Brokern publicerar `offline` (Last Will) | `offline` och sedan `online` när enheten kom tillbaka | ✓ |

## Backend och API

Kördes med backend på Windows-datorn mot brokern, innan TLS lades till.

| # | Test | Förväntat | Observerat | OK |
|---|---|---|---|---|
| 8 | ESP32 skickar mätningar | Backend godkänner båda sensorerna | `event=reading_accepted` för temperatur och luftfuktighet var 10:e s | ✓ |
| 9 | Skicka felaktiga meddelanden | Nekas med rätt kategori, ingen krasch | `{bad` -> `syntax`, saknad `timestamp` -> `structure`, `"value": true` -> `type`, `"value": 99` -> `range`, fel `sensorId` -> `structure` | ✓ |
| 10 | Anropa API:t innan data finns | `404` med felkontraktet | `{"error":"not_found","detail":"No readings received yet"}` | ✓ |
| 11 | Okänd sensor, okänd sökväg, POST | `404`, `404`, `405` | Alla gav rätt status och `error`-kod | ✓ |
| 12 | Hämta mätningar och status | Data och räknare som stämmer | Båda sensorerna, `messagesAccepted: 4`, fem valideringsfel, `stale: false` | ✓ |
| 13 | Stoppa Mosquitto i 20 s | Backend återansluter själv | `mqtt_disconnected`, tre `mqtt_connect_failed`, sedan `mqtt_connected`. `mqttReconnects: 1` | ✓ |
| 14 | Starta utan `MQTT_HOST` | Tydligt fel | `Missing environment variable MQTT_HOST` | ✓ |

## Säkerhet och drift

| # | Test | Förväntat | Observerat | OK |
|---|---|---|---|---|
| 15 | Skapa cert | Serverns IP i certet | `IP Address:192.168.50.150`, giltigt till 2029-01-01 | ✓ |
| 16 | Ansluta över TLS med lösenord | Godkänns | `mosquitto_sub --cafile ca.crt -p 8883 -u backend` tog emot status | ✓ |
| 17 | Ansluta utan lösenord | Nekas | `Connection Refused: not authorised.` | ✓ |
| 18 | Nå portarna från Windows | 1883 stängd, 8883 och 8000 öppna | `Test-NetConnection`: 1883 `False`, 8883 `True`, `curl` mot 8000 svarade | ✓ |
| 19 | ESP32 med TLS | Ansluter, data når API:t | Certfel före SNTP-synk, sedan `Mqtt connected`. API: `deviceOnline: true`, 22,0 °C och 42 % | ✓ |
| 20 | Starta om servern utan att logga in | Allt startar själv | `/api/status` visade ny `startedAt` och godkända mätningar efter 28 s | ✓ |
| 21 | `ufw status verbose` | Allt inkommande nekas som standard | `Default: deny (incoming)`, bara 8883, 8000 och 22 från LAN | ✓ |
