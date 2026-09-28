#pragma once

#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t read_sensor(gpio_num_t pin, float *temperature, float *humidity);
