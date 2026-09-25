#include "wifi.h"

#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace wifi {

constexpr uint16_t kWifiTaskStackSize = 4096;
constexpr uint8_t kWifiTaskPriority = 5;
constexpr uint16_t kWifiLoopPeriodMs = 1000;

static const char *TAG = "wifi";

void Wifi::start()
{
    xTaskCreate(task,
                "wifi",
                kWifiTaskStackSize,
                this,
                kWifiTaskPriority,
                nullptr
            );
}

void Wifi::task(void *pvParameters)
{
    auto *self = static_cast<Wifi *>(pvParameters);

    ESP_LOGI(TAG, "task started");

    self->init();
    self->connect();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kWifiLoopPeriodMs));
    }
}

void Wifi::init()
{
}

void Wifi::connect()
{
}

void Wifi::handle_disconnect()
{
}

void Wifi::sync_time()
{
}

} // namespace wifi
