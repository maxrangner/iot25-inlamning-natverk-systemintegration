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

### TLS mellan ESP32 och brokern

ESP32 ansluter till Mosquitto på port 8883 med TLS. `scripts/gen-certs.sh` skapar med openssl en egen CA och ett servercert med serverns IP-adress. En egen CA räcker på ett lokalt nät, och det behövs varken domännamn eller extern tjänst.

CA-certet är inbyggt i firmwaren. ESP32 kontrollerar att brokerns cert är signerat av CA:n, att IP-adressen stämmer och att certet är giltigt. Det finns ingen okrypterad reserv: är broker-URI:n inte `mqtts://` startar firmwaren inte MQTT alls.

Backend ansluter utan TLS på `127.0.0.1:1883`. Den trafiken lämnar aldrig servern, och porten går inte att nå utifrån.

### Inloggning och behörighet

Mosquitto tillåter inga anonyma anslutningar. ESP32 och backend har var sin användare, och lösenorden sparas som hash i `/etc/mosquitto/passwd`.

Med ACL:en får varje användare bara det den behöver:

| Användare | Får |
|---|---|
| `esp32-c3-01` | bara skriva under `iot25/max/#` |
| `backend` | bara läsa under `iot25/max/#` |

Läcker backendens lösenord kan man alltså inte använda det för att skicka falska mätningar.

### Validering

Backend litar inte på det som kommer in via MQTT. Varje meddelande kontrolleras mot datakontraktet: giltig JSON, alla fält, rätt typer, rimliga värden och att `sensorId` stämmer med topicen. Felaktig data når aldrig API:t. Den loggas och räknas i stället, så att det syns i `/api/status`. API:t har bara `GET` och kan inte ändra något.

### Brandvägg

ufw nekar allt inkommande utom:

| Port | Tjänst | Från |
|---|---|---|
| 8883 | Mosquitto (TLS) | LAN `192.168.50.0/24` |
| 8000 | REST-API | LAN |
| 22 | SSH | LAN |

Port 1883 har ingen regel och lyssnar bara på `127.0.0.1`. Testat från en annan dator: 1883 går inte att nå, men 8883 och 8000 går att nå.

(Servern kör även Syncthing med egna regler. Det hör inte till projektet.)

### Tjänsten körs inte som root

Backend körs som en vanlig användare. Ett fel i backend ger därför inte full åtkomst till servern.

## Känslig konfiguration

| Vad | Var | I Git? |
|---|---|---|
| Wi-Fi SSID och lösenord, MQTT-lösenord för ESP32 | `idf.py menuconfig` → `src/firmware/sdkconfig` | Nej |
| CA-cert för ESP32 | `src/firmware/certs/ca.crt` | Nej |
| MQTT-lösenord för backend | `/etc/iot25/backend.env` (600), läses av systemd | Nej |
| Lösenord i Mosquitto | `/etc/mosquitto/passwd` som hash (600) | Nej |
| CA-nyckel och servernyckel | `certs/` på servern (600) | Nej |

`.gitignore` utesluter `sdkconfig`, `.env`, `certs/`, nycklar och `passwd`. I Git finns bara exempelfiler med påhittade värden (`.env.example`, standardvärdet `changeme` i `Kconfig.projbuild`). Lösenord skrivs aldrig ut i loggarna.

## Kvarvarande begränsningar

- **API:t har varken HTTPS eller inloggning.** Alla på LAN kan läsa mätvärdena. Nästa steg vore HTTPS och en API-nyckel.
- **Lösenord i stället för klientcert.** Ett läckt lösenord räcker för att ansluta. Med ett eget klientcert per enhet (mTLS) vore det svårare.
- **Lösenorden ligger i klartext i ESP32:ns flash.** Den som har enheten i handen kan läsa ut dem. ESP32 har stöd för flash-kryptering, men den är inte påslagen.
- **CA-nyckeln ligger på servern.** Den som tar sig in på servern kan skapa cert som ESP32 litar på. Nyckeln borde förvaras någon annanstans.
- **Backend körs som min egen användare.** En egen systemanvändare för tjänsten vore bättre.
- **Ingen begränsning av antal anrop.** En klient på LAN kan överbelasta API:t eller brokern.
