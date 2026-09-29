#pragma once

#include <driver/gpio.h>
#include <esp_err.h>
#include "mqtt.h"
#include "reading.h"

namespace sensor {



class Dht11Sensor {
public:
    void start(mqtt::Mqtt* mqtt);

private:
    static void task(void *pvParameters);
    esp_err_t read(SensorReading* temp_reading, SensorReading* humid_reading);

    mqtt::Mqtt* mqtt;
    uint8_t sensorId = 0;
};

} // namespace sensor
