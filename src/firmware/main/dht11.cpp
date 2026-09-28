#include "dht11.h"

#include "dht.h"

esp_err_t read_sensor(gpio_num_t pin, float *temperature, float *humidity)
{
    return dht_read_float_data(DHT_TYPE_DHT11, pin, humidity, temperature);
}
