#pragma once

#include <driver/gpio.h>
#include <esp_err.h>

namespace sensor {

class Sensor {
public:
    void start();

private:
    static void task(void *pvParameters);

    esp_err_t read(float *temperature, float *humidity);
};

} // namespace sensor
