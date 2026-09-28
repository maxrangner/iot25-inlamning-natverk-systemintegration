#include "wifi.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"


namespace wifi {

constexpr uint16_t kWifiTaskStackSize = 4096;
constexpr uint8_t kWifiTaskPriority = 5;
constexpr uint16_t kWifiLoopPeriodMs = 1000;

static const char *TAG = "wifi";

void Wifi::start()
{
    xTaskCreate(task,
                "wifi",
                kWifiTaskStackSize,
                this,
                kWifiTaskPriority,
                nullptr
            );
}

void Wifi::task(void *pvParameters)
{
    auto *self = static_cast<Wifi *>(pvParameters);

    ESP_LOGI(TAG, "task started");

    self->init();
    self->connect();
    self->sync_time();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kWifiLoopPeriodMs));
    }
}

void Wifi::event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    auto *self = static_cast<Wifi *>(arg);

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *event = static_cast<wifi_event_sta_disconnected_t *>(data);
        ESP_LOGW(TAG, "disconnected, reason=%d, retry in %d s", event->reason, self->backoff_seconds);

        self->connected = false;
        esp_timer_start_once(self->reconnect_timer, self->backoff_seconds * 1000000ULL);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto *event = static_cast<ip_event_got_ip_t *>(data);

        self->connected = true;
        self->backoff_seconds = 1;
        ESP_LOGI(TAG, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void Wifi::init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, this));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    const esp_timer_create_args_t args = {
        .callback = handle_disconnect,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_retry",
        .skip_unhandled_events = false,
        };
    ESP_ERROR_CHECK(esp_timer_create(&args, &reconnect_timer));
}

void Wifi::connect()
{
    wifi_config_t config = {};
    snprintf(reinterpret_cast<char *>(config.sta.ssid), sizeof(config.sta.ssid), "%s", CONFIG_IOT25_WIFI_SSID);
    snprintf(reinterpret_cast<char *>(config.sta.password), sizeof(config.sta.password), "%s", CONFIG_IOT25_WIFI_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(34)); // Full TX power made connection buggy for some reason

    ESP_LOGI(TAG, "connecting to \"%s\"", CONFIG_IOT25_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void Wifi::handle_disconnect(void *arg)
{
    auto *self = static_cast<Wifi *>(arg);

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "reconnect failed: %s", esp_err_to_name(err));
    }

    self->backoff_seconds = self->backoff_seconds * 2;
    if (self->backoff_seconds > 60) {
        self->backoff_seconds = 60;
    }
}

void Wifi::sync_time()
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
}

bool Wifi::is_connected() const {
    return connected;
}

} // namespace wifi
