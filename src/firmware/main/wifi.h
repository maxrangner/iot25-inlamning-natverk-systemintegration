#pragma once

#include "esp_event.h"
#include "esp_timer.h"

namespace wifi {

class Wifi {
public:
    void start();
    bool is_connected() const;

private:
    static void task(void *pvParameters);
    static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);

    void init();
    void connect();
    static void handle_disconnect(void *arg);
    void sync_time();

    int backoff_seconds = 1;
    bool connected = false;
    esp_timer_handle_t reconnect_timer = nullptr;
};

} // namespace wifi
