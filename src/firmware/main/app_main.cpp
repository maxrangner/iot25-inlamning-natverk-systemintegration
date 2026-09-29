#include "esp_log.h"
#include "sensor.h"
#include "wifi.h"
#include "mqtt.h"

constexpr char TAG[] = "main app";

extern "C" void app_main(void)
{
    static wifi::Wifi wifi;
    static sensor::Dht11Sensor dht11_sensor;
    static mqtt::Mqtt mqtt;

    wifi.start();

    while (!wifi.is_connected()) {
        ESP_LOGI(TAG, "Waiting for wifi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    mqtt.start();

    dht11_sensor.start(&mqtt);
}
