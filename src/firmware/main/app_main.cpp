#include "dht11.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "wifi.h"

#define SENSOR_GPIO GPIO_NUM_4
#define SENSOR_PERIOD_US (10 * 1000 * 1000) // 10 seconds

static const char *TAG = "app";

static void timer_callback(void *arg)
{
    float temperature = 0.0f;
    float humidity = 0.0f;

    esp_err_t err = read_sensor(SENSOR_GPIO, &temperature, &humidity);

    esp_rom_delay_us(30000); // Wait for sensor to cool down after reading

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "temp = %.1f C, humidity = %.1f %%", temperature, humidity);
    } else {
        ESP_LOGW(TAG, "Error reading sensor");
    }
}

extern "C" void app_main(void)
{
    static wifi::Wifi wifi;
    wifi.start();

    const esp_timer_create_args_t args = {
        .callback = timer_callback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sensor",
        .skip_unhandled_events = false,
    };

    esp_timer_handle_t sensor_read_timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &sensor_read_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sensor_read_timer, SENSOR_PERIOD_US));
}