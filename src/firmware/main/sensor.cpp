#include "sensor.h"

#include <stdint.h>
#include <time.h>
#include "dht.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace sensor {

constexpr gpio_num_t kSensorGpio = GPIO_NUM_4;
constexpr uint16_t kSensorTaskStackSize = 4096;
constexpr uint8_t kSensorTaskPriority = 5;
constexpr uint32_t kSensorPeriodMs = 10000;

static const char *TAG = "sensor";

void Sensor::start()
{
    xTaskCreate(task,
                "sensor",
                kSensorTaskStackSize,
                this,
                kSensorTaskPriority,
                nullptr
            );
}

void Sensor::task(void *pvParameters)
{
    auto *self = static_cast<Sensor *>(pvParameters);

    ESP_LOGI(TAG, "task started");

    while (true) {
        float temperature = 0.0f;
        float humidity = 0.0f;

        esp_err_t err = self->read(&temperature, &humidity);

        esp_rom_delay_us(30000); // The DHT driver disables interrupts for 20 ms, which makes the console drop the next line

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "unix time: %lld", (long long)time(nullptr));
            ESP_LOGI(TAG, "temp = %.1f C, humidity = %.1f %%", temperature, humidity);
        } else {
            ESP_LOGW(TAG, "Error reading sensor");
        }

        vTaskDelay(pdMS_TO_TICKS(kSensorPeriodMs));
    }
}

esp_err_t Sensor::read(float *temperature, float *humidity)
{
    return dht_read_float_data(DHT_TYPE_DHT11, kSensorGpio, humidity, temperature);
}

} // namespace sensor
