#pragma once

#include "esp_event.h"
#include "mqtt_client.h"

namespace mqtt {

class Mqtt {
public:
    void start();
    static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);
    void publish(const char* topic, const char* payload);

private:
    esp_mqtt_client_handle_t client = nullptr;
    bool connected = false;
};

} // namespace mqtt