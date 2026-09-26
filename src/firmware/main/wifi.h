#pragma once

#include "esp_event.h"

namespace wifi {

class Wifi {
public:
    void start();

private:
    static void task(void *pvParameters);
    static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);

    void init();
    void connect();
    void handle_disconnect();
    void sync_time();

    int backoff_seconds = 1;
    bool connected = false;
};

} // namespace wifi
