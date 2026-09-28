#pragma once

namespace wifi {

class Wifi {
public:
    void start();

private:
    static void task(void *pvParameters);

    void init();
    void connect();
    void handle_disconnect();
    void sync_time();

    int backoff_seconds = 1;
    bool connected = false;
};

} // namespace wifi
