#include "sensor.h"
#include "wifi.h"

extern "C" void app_main(void)
{
    static wifi::Wifi wifi;
    static sensor::Sensor sensor;

    wifi.start();
    sensor.start();
}
