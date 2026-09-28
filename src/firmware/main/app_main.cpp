#include "sensor.h"
#include "wifi.h"
#include "mqtt.h"

extern "C" void app_main(void)
{
    static wifi::Wifi wifi;
    static sensor::Sensor sensor;
    static mqtt::Mqtt mqtt;

    wifi.start();

    while (!wifi.is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    mqtt.start();

    sensor.start(&mqtt);
}
