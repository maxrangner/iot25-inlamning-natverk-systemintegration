#pragma once

#include <string>
#include "esp_event.h"
#include "mqtt_client.h"
#include "reading.h"

namespace mqtt {

constexpr uint8_t kPayloadBufferSize = 150;
constexpr uint8_t kTopicSize = 100;
constexpr char kTopicBase[] = "iot25/max/";
constexpr char kTopicStatus[] = "iot25/max/status/esp32-c3-01";
constexpr uint8_t kQoS = 1;
constexpr uint8_t kDontRetain = 0;
constexpr uint8_t kRetain = 1;
constexpr char kOnlineMsg[] = "online";
constexpr char kOfflineMsg[] = "offline";

class Mqtt {
public:
    void start();
    void publish(const sensor::SensorReading &reading);
    
private:
    static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);
    std::string format_json(const sensor::SensorReading &reading);

    esp_mqtt_client_handle_t client = nullptr;
    bool connected = false;
};

} // namespace mqtt