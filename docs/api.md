# API

## MQTT-payload

Ett mätvärde per meddelande, publicerat på `iot25/max/sensor/<sensorId>` med QoS 1.

```json
{
  "sensorId": "room-a-temp-01",
  "timestamp": "2026-09-17T10:30:00+02:00",
  "value": 21.7,
  "unit": "C"
}
```

| Fält | Typ | Regel |
|---|---|---|
| `sensorId` | string | Obligatorisk, icke-tom, måste matcha topicen |
| `timestamp` | string | Obligatorisk, ISO 8601 med tidszon |
| `value` | number | Obligatorisk. Temperatur 0–50, luftfuktighet 20–90 |
| `unit` | string | Obligatorisk. `"C"` eller `"%"` |

## REST-API

### `GET /health`

### `GET /api/readings/latest`

### `GET /api/readings/{sensorId}`

### `GET /api/status`

## Felkontrakt

```json
{ "error": "<kod>", "detail": "<beskrivning>" }
```
