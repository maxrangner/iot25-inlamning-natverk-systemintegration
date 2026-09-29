#include "sensor.h"

#include <stdint.h>
#include <time.h>
#include "dht.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace sensor {

static const char *TAG = "sensor";

constexpr gpio_num_t kSensorGpio = GPIO_NUM_4;

constexpr char kTemperatureId[] = "room-a-temp-01";
constexpr char kHumididtyId[] = "room-a-hum-01";

constexpr uint16_t kSensorTaskStackSize = 4096;
constexpr uint8_t kSensorTaskPriority = 5;

constexpr uint32_t kSensorReadInvervalMs = 10000;
constexpr time_t kMinValidTime = 1735689600;

void Dht11Sensor::start(mqtt::Mqtt* mqtt_)
{
    mqtt = mqtt_;
    xTaskCreate(task,
                "sensor",
                kSensorTaskStackSize,
                this,
                kSensorTaskPriority,
                nullptr
            );
}

void Dht11Sensor::task(void *pvParameters)
{
    auto *self = static_cast<Dht11Sensor *>(pvParameters);

    ESP_LOGI(TAG, "task started");

    while (true) {
        SensorReading temp_reading;
        SensorReading humid_reading;
        esp_err_t err = self->read(&temp_reading, &humid_reading);
        if (err == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(50)); // The DHT driver disables interrupts for 20 ms, which makes the console drop the next line

            if (time(nullptr) < kMinValidTime) {
                ESP_LOGW(TAG, "Time not synced. Not calling publish.");
                return;
            }

            self->mqtt->publish(temp_reading);
            self->mqtt->publish(humid_reading);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "unix time: %lld", (long long)time(nullptr));
                ESP_LOGI(TAG, "temp = %.1f C, humidity = %.1f %%", temp_reading.value, humid_reading.value);
            } else {
                ESP_LOGW(TAG, "Error reading sensor");
            }
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error reading sensor.");
        }
        vTaskDelay(pdMS_TO_TICKS(kSensorReadInvervalMs));
    }
}

esp_err_t Dht11Sensor::read(SensorReading* temp_reading, SensorReading* humid_reading)
{
    float temperature;
    float humidity;
    time_t now = time(nullptr);
    esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, kSensorGpio, &humidity, &temperature);
    if (err != ESP_OK) {
        return err;
    }

    temp_reading->sensorId = kTemperatureId;
    temp_reading->timestamp = now;
    temp_reading->value = temperature;
    temp_reading->unit = "C";

    humid_reading->sensorId = kHumididtyId;
    humid_reading->timestamp = now;
    humid_reading->value = humidity;
    humid_reading->unit = "%";

    return err;
}

} // namespace sensor
