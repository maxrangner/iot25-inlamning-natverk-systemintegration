# Säkerhet

## Risker

| Risk | Vad kan hända | Åtgärd |
|---|---|---|
| Någon avlyssnar trafiken mellan ESP32 och brokern | Mätvärden och MQTT-lösenordet läses i klartext | TLS |
| Någon ansluter till brokern och skickar falska mätningar | API:t visar fel värden | Lösenord och ACL |
| ESP32 luras att ansluta till en falsk broker | Data och lösenord hamnar hos någon annan | ESP32 verifierar brokerns cert |
| Trasig eller manipulerad data når backend | Orimliga värden i API:t, eller en krasch | Validering av all inkommande JSON |
| Tjänster går att nå från fler datorer än nödvändigt | Större attackyta | ufw |
| Lösenord hamnar i Git | Den som kan läsa repot kan logga in | Hemligheter bara i filer som inte versionshanteras |

## Åtgärder

### TLS

`scripts/gen-certs.sh` skapar en egen CA och ett servercert med serverns IP-adress. En egen CA räcker på ett lokalt nät och kräver inget domännamn. CA-certet är inbyggt i firmwaren, och ESP32 kontrollerar signatur, IP-adress och giltighetstid. Är broker-URI:n inte `mqtts://` startar MQTT inte alls.

Backend ansluter utan TLS på `127.0.0.1:1883`, eftersom den trafiken aldrig lämnar servern.

### Inloggning och behörighet

Mosquitto tillåter inga anonyma anslutningar. Enligt ACL:en får `esp32-c3-01` bara skriva och `backend` bara läsa under `iot25/max/#`. Läcker backendens lösenord kan man inte använda det för att skicka falska mätningar.

### Validering

Varje meddelande kontrolleras mot [datakontraktet](api.md#mqtt-payload). Felaktig data når aldrig API:t. API:t har bara `GET` och kan inte ändra något.

### Brandvägg

ufw nekar allt inkommande utom:

| Port | Tjänst | Från |
|---|---|---|
| 8883 | Mosquitto (TLS) | LAN `192.168.50.0/24` |
| 8000 | REST-API | LAN |
| 22 | SSH | LAN |

Port 1883 lyssnar bara på `127.0.0.1`.

## Känslig konfiguration

| Vad | Var | I Git? |
|---|---|---|
| Wi-Fi SSID och lösenord, MQTT-lösenord för ESP32 | `src/firmware/sdkconfig` via `idf.py menuconfig` | Nej |
| CA-cert för ESP32 | `src/firmware/certs/ca.crt` | Nej |
| MQTT-lösenord för backend | `/etc/iot25/backend.env` (600), läses av systemd | Nej |
| Lösenord i Mosquitto | `/etc/mosquitto/passwd` som hash (600) | Nej |
| CA-nyckel och servernyckel | `certs/` på servern (600) | Nej |

I Git finns bara exempelfiler med `changeme` som lösenord. Lösenord skrivs aldrig ut i loggarna.

## Kvarvarande begränsningar

- API:t har varken HTTPS eller inloggning. Nästa steg vore HTTPS och en API-nyckel.
- Ett läckt MQTT-lösenord räcker för att ansluta. Klientcert per enhet (mTLS) vore säkrare.
- Lösenorden ligger i klartext i ESP32ans flash. Flash-kryptering finns men är inte påslagen.
- CA-nyckeln ligger på servern. Den som tar sig in där kan skapa cert som ESP32 litar på.
- Backend körs som min egen användare, inte root. En egen systemanvändare vore bättre.
- Ingen begränsning av antal anrop. En klient på LAN kan överbelasta API:t eller brokern.
