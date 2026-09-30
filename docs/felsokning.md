# Felsökning

## Fel 1: Brokern stoppas eller nätverket bryts

1. **Observerat symptom:**
2. **Hur felet identifierades:**
3. **Verktyg och loggar:**
4. **Orsak:**
5. **Åtgärd:**
6. **Verifiering:**

## Fel 2: Ogiltig JSON eller fel datatyp

1. **Observerat symptom:**
2. **Hur felet identifierades:**
3. **Verktyg och loggar:**
4. **Orsak:**
5. **Åtgärd:**
6. **Verifiering:**

## Fel som uppstod under arbetet

De här två felen var inte avsiktliga. De dök upp när jag lade till TLS.

### Mosquitto startar inte

1. **Observerat symptom:** `systemctl restart mosquitto` misslyckades och tjänsten stod som `failed`. Varken ESP32 eller backend kom åt brokern.
2. **Hur felet identifierades:** `systemctl status` visade bara att processen hade avslutats med felkod 1. Mosquitto skriver sina fel till en egen loggfil, så jag läste den i stället.
3. **Verktyg och loggar:** `sudo tail /var/log/mosquitto/mosquitto.log`
   ```
   Error: Unable to load server certificate "/etc/mosquitto/certs/server.crt". Check certfile.
   OpenSSL Error[0]: error:80000002:system library::No such file or directory
   ```
   `ls /etc/mosquitto/certs/` visade att `server.crt` saknades.
4. **Orsak:** jag hade bara kopierat `ca.crt` och `server.key`, inte `server.crt`. Mosquitto startar inte om TLS-lyssnaren inte kan läsa sitt cert.
5. **Åtgärd:** kopierade `server.crt` till `/etc/mosquitto/certs/` och startade om Mosquitto.
6. **Verifiering:** `systemctl status mosquitto` visade `active (running)`, och en anslutning med `mosquitto_sub` över TLS fungerade.

### ESP32 får certfel direkt efter start

1. **Observerat symptom:** efter flashning med TLS visade monitorn `mbedtls_ssl_handshake returned -0x2700` och `Error transport connect`. Ingen data kom fram.
2. **Hur felet identifierades:** `-0x2700` betyder att certet inte kunde verifieras. Precis före felet fanns raden `Time not synced` från sensormodulen, alltså var klockan inte satt.
3. **Verktyg och loggar:** ESP32:ns monitor (`idf.py monitor`)
   ```
   W (2462) dht11: Time not synced. Not calling publish.
   E (3482) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x2700
   E (3492) mqtt_client: Error transport connect
   ```
4. **Orsak:** firmwaren kontrollerar certets giltighetstid. MQTT startar direkt när Wi-Fi är uppe, men då har SNTP inte hunnit synka klockan, som står på 1970. Certet är giltigt från 2026, så det ser ut att inte ha börjat gälla än.
5. **Åtgärd:** ingen ändring behövdes. esp-mqtt försöker igen efter 10 s, och då är klockan rätt. Jag dokumenterade beteendet i README. Firmwaren skulle också kunna vänta på SNTP innan den ansluter.
6. **Verifiering:** nästa försök gav `Mqtt connected`, och `/api/status` visade `deviceOnline: true` med ökande `messagesAccepted`.
