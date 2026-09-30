# API

Lösningen har två gränssnitt: MQTT mellan ESP32 och backend, och REST mellan backend och andra system.

## MQTT

### Anslutning

| | ESP32 | Backend |
|---|---|---|
| Adress | `mqtts://192.168.50.150:8883` | `127.0.0.1:1883` |
| Användare | `esp32-c3-01` | `backend` |
| Får | skriva under `iot25/max/#` | läsa under `iot25/max/#` |

### MQTT-payload

Ett mätvärde per meddelande på `iot25/max/sensor/<sensorId>`, QoS 1:

```
iot25/max/sensor/room-a-temp-01 {"sensorId":"room-a-temp-01", "timestamp":"2026-09-29T10:30:00Z", "value":22.00, "unit":"C"}
iot25/max/sensor/room-a-hum-01 {"sensorId":"room-a-hum-01", "timestamp":"2026-09-29T10:30:00Z", "value":47.00, "unit":"%"}
```

| Fält | Typ | Regel |
|---|---|---|
| `sensorId` | string | Får inte vara tom och måste stämma med topicen |
| `timestamp` | string | ISO 8601 med tidszon. ESP32 skickar UTC (`Z`) |
| `value` | number | Temperatur 0–50, luftfuktighet 20–90 |
| `unit` | string | `C` eller `%` |

Alla fält är obligatoriska. Okända fält ignoreras, så att kontraktet kan byggas ut utan att något går sönder.

### Validering

Backend nekar meddelanden som inte följer kontraktet. Varje nekat meddelande loggas och räknas i `/api/status`, i en av fyra kategorier:

| Kategori | Exempel |
|---|---|
| `syntax` | Inte giltig JSON, t.ex. `{bad` |
| `structure` | Ett fält saknas, eller `sensorId` stämmer inte med topicen |
| `type` | `value` är en sträng eller `true`, eller `timestamp` är inte ISO 8601 |
| `range` | `value` är orimligt (t.ex. 99 °C), eller `unit` är okänd |

### Status

`iot25/max/status/esp32-c3-01` innehåller `online` eller `offline` som ren text, retained. ESP32 publicerar `online` när den ansluter, och brokern publicerar `offline` (Last Will) om enheten försvinner.

## REST-API

Bas-URL: `http://192.168.50.150:8000`. Alla svar är JSON.

| Metod | Sökväg | Svar |
|---|---|---|
| GET | `/health` | `200` om backend körs |
| GET | `/api/readings/latest` | Senaste mätningen per sensor, `404` om inget finns |
| GET | `/api/readings/{sensorId}` | Senaste mätningen för en sensor, `404` om sensorn är okänd |
| GET | `/api/status` | Övervakningsmått |

### GET /health

```bash
curl http://192.168.50.150:8000/health
```

```json
{"status":"ok"}
```

### GET /api/readings/latest

```bash
curl http://192.168.50.150:8000/api/readings/latest
```

```json
[
  {"sensorId":"room-a-temp-01","timestamp":"2026-09-29T15:55:54Z","value":22.0,"unit":"C","receivedAt":"2026-09-29T15:55:54+00:00"},
  {"sensorId":"room-a-hum-01","timestamp":"2026-09-29T15:55:54Z","value":40.0,"unit":"%","receivedAt":"2026-09-29T15:55:54+00:00"}
]
```

`receivedAt` sätts av backend när mätningen tas emot. Finns ingen mätning än, till exempel direkt efter en omstart, blir svaret `404`:

```json
{"error":"not_found","detail":"No readings received yet"}
```

### GET /api/readings/{sensorId}

| Parameter | Exempel |
|---|---|
| `sensorId` (i sökvägen) | `room-a-temp-01` eller `room-a-hum-01` |

```bash
curl http://192.168.50.150:8000/api/readings/room-a-temp-01
```

```json
{"sensorId":"room-a-temp-01","timestamp":"2026-09-29T15:55:54Z","value":22.0,"unit":"C","receivedAt":"2026-09-29T15:55:54+00:00"}
```

Okänd sensor ger `404`:

```json
{"error":"not_found","detail":"Unknown sensor nope"}
```

### GET /api/status

```bash
curl http://192.168.50.150:8000/api/status
```

```json
{
  "mqttConnected": true,
  "deviceOnline": true,
  "messagesReceived": 45,
  "messagesAccepted": 40,
  "validationErrors": {"syntax": 1, "structure": 2, "type": 1, "range": 1},
  "mqttReconnects": 1,
  "lastReadingAt": "2026-09-29T15:59:27+00:00",
  "secondsSinceLastReading": 8.6,
  "stale": false,
  "startedAt": "2026-09-29T15:55:36+00:00",
  "uptimeSeconds": 239
}
```

| Fält | Betydelse |
|---|---|
| `mqttConnected` | Backend är ansluten till brokern |
| `deviceOnline` | ESP32 är ansluten till brokern |
| `messagesReceived` / `messagesAccepted` | Mottagna mätningar och hur många av dem som godkändes |
| `validationErrors` | Nekade mätningar per kategori |
| `mqttReconnects` | Antal återanslutningar sedan start |
| `secondsSinceLastReading` | Sekunder sedan senaste godkända mätningen |
| `stale` | `true` om ingen mätning kommit på över 30 s |

Räknarna nollställs när backend startas om.

## Felkontrakt

Alla fel har samma form:

```json
{"error": "<kod>", "detail": "<beskrivning>"}
```

| Status | `error` | När |
|---|---|---|
| 404 | `not_found` | Inget data, okänd sensor eller okänd sökväg |
| 405 | `method_not_allowed` | Annan metod än GET |

Klienter bör jämföra mot `error`. `detail` är till för människor.
