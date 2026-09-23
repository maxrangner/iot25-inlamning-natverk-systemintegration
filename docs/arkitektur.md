# Arkitektur

## Översikt

```
DHT11 ─GPIOX─ ESP32-C3 ──MQTTS :8883──▶ Mosquitto ──MQTT 127.0.0.1:1883──▶ Backend (FastAPI)
                                                                             │ HTTP :8000
                                                                             ▼
                                                                    curl / webbläsare (LAN)
```

## Komponenter och ansvar

## IP-adresser, värdnamn och portar

## Kommunikationsmodell och topics

## Dataformat

## Återanslutning och felhantering

## Varför MQTT

## Varför REST
