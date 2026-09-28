#pragma once

#include <driver/gpio.h>
#include <esp_err.h>
#include "mqtt.h"

namespace sensor {

class Sensor {
public:
    void start(mqtt::Mqtt* mqtt);

private:
    static void task(void *pvParameters);
    esp_err_t read(float *temperature, float *humidity);

    mqtt::Mqtt* mqtt;
};

} // namespace sensor
