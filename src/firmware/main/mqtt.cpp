#include "mqtt.h"
#include <string.h>
#include "esp_log.h"

extern const char ca_crt_start[] asm("_binary_ca_crt_start");

namespace mqtt {

constexpr char TAG[] = "mqtt";
constexpr char kSecureScheme[] = "mqtts://";

void Mqtt::start() {
    if (strncmp(CONFIG_IOT25_MQTT_BROKER_URI, kSecureScheme, strlen(kSecureScheme)) != 0) {
        ESP_LOGE(TAG, "Broker URI must start with %s, not starting MQTT", kSecureScheme);
        return;
    }

    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = CONFIG_IOT25_MQTT_BROKER_URI;
    cfg.broker.verification.certificate = ca_crt_start;
    cfg.credentials.client_id = "esp32-c3-01";
    cfg.credentials.username = CONFIG_IOT25_MQTT_USERNAME;
    cfg.credentials.authentication.password = CONFIG_IOT25_MQTT_PASSWORD;
    cfg.session.last_will.topic = kTopicStatus;
    cfg.session.last_will.msg = kOfflineMsg;
    cfg.session.last_will.qos = kQoS;
    cfg.session.last_will.retain = kRetain;

    client = esp_mqtt_client_init(&cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, event_handler, this));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}

void Mqtt::event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    auto *self = static_cast<Mqtt *>(arg);

    if (id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Mqtt connected");
        self->connected = true;
        if (esp_mqtt_client_publish(self->client, kTopicStatus, kOnlineMsg, 0, kQoS, kRetain) < 0) {
            ESP_LOGW(TAG, "Failed to publish online status");
        }
    } else if (id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Mqtt disconnected");
        self->connected = false;
    } else if (id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "Mqtt error");
    }
}

void Mqtt::publish(const SensorReading &reading) {
    if (!connected) {
        return;
    }

    std::string payload = format_json(reading);

    char topic[kTopicSize];
    snprintf(topic, sizeof(topic), "%s%s%s", kTopicBase, "sensor/", reading.sensorId);

    if (esp_mqtt_client_publish(client, topic, payload.c_str(), 0, kQoS, kDontRetain) < 0) {
        ESP_LOGW(TAG, "Failed to publish reading to %s", topic);
        return;
    }

    ESP_LOGI(TAG,
             "\n\n"
             "┌─ MQTT Publish ────────────────────────────\n"
             "│ sensorId: %s\n"
             "│ topic: %s\n"
             "│ value: %f\n"
             "│ unit: %s\n"
             "└───────────────────────────────────────────\n",
             reading.sensorId,
             topic,
             reading.value,
             reading.unit
            );
}

std::string Mqtt::format_json(const SensorReading &reading) {
    char buffer[kPayloadBufferSize];

    struct tm tm;
    gmtime_r(&reading.timestamp, &tm);
    char ts[25];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm);

    snprintf(buffer,
             sizeof(buffer),
            "{\"sensorId\":\"%s\", \"timestamp\":\"%s\", \"value\":%.2f, \"unit\":\"%s\"}",
            reading.sensorId,
            ts,
            reading.value,
            reading.unit);
    return buffer;
}

} //namespace mqtt
