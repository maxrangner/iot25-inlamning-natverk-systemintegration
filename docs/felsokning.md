# Felsökning

## Fel 1: Brokern stoppas eller nätverket bryts

1. Observerat symptom:
2. Hur felet identifierades:
3. Verktyg och loggar:
4. Orsak:
5. Åtgärd:
6. Verifiering:

## Fel 2: Ogiltig JSON eller fel datatyp

1. Observerat symptom:
2. Hur felet identifierades:
3. Verktyg och loggar:
4. Orsak:
5. Åtgärd:
6. Verifiering:

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
