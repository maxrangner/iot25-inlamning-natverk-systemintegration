# Felsökning

## Fel 1: Brokern stoppas

1. Observerat symptom: backend tappade anslutningen till brokern.
2. Hur felet identifierades: loggen visade `mqtt_disconnected` och sedan tre `mqtt_connect_failed`.
3. Verktyg och loggar: backendens logg och `/api/status`.
4. Orsak: Mosquitto stoppades avsiktligt.
5. Åtgärd: Mosquitto startades igen.
6. Verifiering: loggen visade `mqtt_connected` och `/api/status` visade `mqttReconnects: 1`.

## Fel 2: Ogiltig JSON

1. Observerat symptom: jag skickade `{bad` som MQTT-meddelande, men det dök inte upp i API:t.
2. Hur felet identifierades: backend loggade `reading_rejected` med `category=syntax`.
3. Verktyg och loggar: backendloggen och `/api/status`.
4. Orsak: `{bad` är inte giltig JSON, så backend kunde inte läsa någon mätning ur meddelandet.
5. Åtgärd: jag skickade giltiga mätningar igen. Backend behövde inte ändras.
6. Verifiering: API:t visade båda sensorerna. `/api/status` visade fyra godkända meddelanden och fem valideringsfel totalt.

## Fel som uppstod under arbetet

De här två felen var inte avsiktliga. De dök upp när jag lade till TLS.

### Mosquitto startar inte

1. Observerat symptom: `systemctl restart mosquitto` misslyckades och tjänsten stod som `failed`.
2. Hur felet identifierades: `systemctl status` visade bara felkod 1, så jag läste Mosquittos egen logg.
3. Verktyg och loggar: `sudo tail /var/log/mosquitto/mosquitto.log`
   ```
   Error: Unable to load server certificate "/etc/mosquitto/certs/server.crt". Check certfile.
   OpenSSL Error[0]: error:80000002:system library::No such file or directory
   ```
   `ls /etc/mosquitto/certs/` visade att `server.crt` saknades.
4. Orsak: jag hade glömt att kopiera `server.crt`.
5. Åtgärd: kopierade `server.crt` och startade om Mosquitto.
6. Verifiering: `systemctl status mosquitto` visade `active (running)`.

### ESP32 får certfel direkt efter start

1. Observerat symptom: första MQTT-anslutningen efter start misslyckades.
2. Hur felet identifierades: `-0x2700` betyder att certet inte kunde verifieras. Precis före stod `Time not synced`.
3. Verktyg och loggar: ESP32ans monitor (`idf.py monitor`)
   ```
   W (2462) dht11: Time not synced. Not calling publish.
   E (3482) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x2700
   E (3492) mqtt_client: Error transport connect
   ```
4. Orsak: firmwaren kontrollerar certets giltighetstid, men klockan står på 1970 tills SNTP har synkat. Certet ser då ut att inte ha börjat gälla.
5. Åtgärd: ingen. esp-mqtt försöker igen efter 10 s, och då är klockan rätt. Firmwaren skulle kunna vänta på SNTP innan den ansluter.
6. Verifiering: nästa försök gav `Mqtt connected`.
