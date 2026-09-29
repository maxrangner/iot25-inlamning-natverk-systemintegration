#pragma once

#include <driver/gpio.h>
#include <esp_err.h>
#include "mqtt.h"
#include "reading.h"

namespace dht11 {

class Dht11Sensor {
public:
    void start(mqtt::Mqtt* mqtt);

private:
    static void task(void *pvParameters);
    esp_err_t read(SensorReading* temp_reading, SensorReading* humid_reading);

    mqtt::Mqtt* mqtt = nullptr;
};

} // namespace dht11
