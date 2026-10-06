# API

## MQTT-payload

Ett mätvärde per meddelande på `iot25/max/sensor/<sensorId>`:

```
iot25/max/sensor/room-a-temp-01 {"sensorId":"room-a-temp-01", "timestamp":"2026-09-29T10:30:00Z", "value":22.00, "unit":"C"}
iot25/max/sensor/room-a-hum-01 {"sensorId":"room-a-hum-01", "timestamp":"2026-09-29T10:30:00Z", "value":47.00, "unit":"%"}
```

| Fält | Typ | Regel |
|---|---|---|
| `sensorId` | string | Får inte vara tom och måste stämma med topicen |
| `timestamp` | string | ISO 8601 med tidszon. ESP32 skickar UTC (`Z`) |
| `value` | number | Temperatur 0-50, luftfuktighet 20-90 |
| `unit` | string | `C` eller `%` |

Alla fält är obligatoriska. Meddelanden som inte följer kontraktet loggas och räknas i `/api/status`, i en av fyra kategorier:

| Kategori | Exempel |
|---|---|
| `syntax` | Inte giltig JSON, t.ex. `{bad` |
| `structure` | Ett fält saknas, eller `sensorId` stämmer inte med topicen |
| `type` | `value` är en sträng eller `true`, eller `timestamp` är inte ISO 8601 |
| `range` | `value` är orimligt (t.ex. 99 °C), eller `unit` är okänd |

## REST-API

Bas-URL i min utvecklingsmiljö: `http://192.168.50.150:8000`. Alla svar är JSON.

| Metod | Sökväg | Svar |
|---|---|---|
| GET | `/health` | `{"status":"ok"}` om backend körs |
| GET | `/api/readings/latest` | Senaste mätningen per sensor |
| GET | `/api/readings/{sensorId}` | Senaste mätningen för en sensor |
| GET | `/api/status` | Övervakningsmått |

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

`receivedAt` sätts av backend. Finns ingen mätning än blir svaret `404`.

### GET /api/readings/{sensorId}

Parameter: `sensorId` i sökvägen, `room-a-temp-01` eller `room-a-hum-01`.

```bash
curl http://192.168.50.150:8000/api/readings/room-a-temp-01
```

```json
{"sensorId":"room-a-temp-01","timestamp":"2026-09-29T15:55:54Z","value":22.0,"unit":"C","receivedAt":"2026-09-29T15:55:54+00:00"}
```

Okänd sensor ger `404`.

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
| `deviceOnline` | `true` när ESP32 har rapporterat `online`. `false` om enheten rapporterat `offline` eller backend inte kan bekräfta statusen, till exempel när brokern är nere |
| `stale` | `true` om ingen mätning kommit på över 30 s |

Räknarna nollställs när backend startas om.

## Felkontrakt

```json
{"error":"not_found","detail":"Unknown sensor nope"}
```

| Status | `error` | När |
|---|---|---|
| 404 | `not_found` | Inget data, okänd sensor eller okänd sökväg |
| 405 | `method_not_allowed` | Annan metod än GET |
