#include "mqtt.h"
#include "esp_log.h"

namespace mqtt {

constexpr char TAG[] = "mqtt";

void Mqtt::start() {
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = CONFIG_IOT25_MQTT_BROKER_URI;
    cfg.credentials.client_id = "esp32-c3-01";
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
        esp_mqtt_client_publish(self->client, kTopicStatus, kOnlineMsg, 0, kQoS, kRetain);
    } else if (id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Mqtt disconnected");
        self->connected = false;
    } else if (id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "Mqtt error");
    }
}

void Mqtt::publish(const sensor::SensorReading &reading) {
    if (!connected) {
        return;
    }

    std::string payload = format_json(reading);

    char topic[kTopicSize];
    snprintf(topic, sizeof(topic), "%s%s%s", kTopicBase, "sensor/", reading.sensorId);

    ESP_LOGI(TAG,
             "\n\n"
             "┌─ MQTT Publish · asd ──────────────────────\n"
             "| sensorId: %s\n"
             "│ topic: %s\n"
             "│ value: %f\n"
             "│ unit: %s\n"
             "└───────────────────────────────────────────\n",
             reading.sensorId,
             topic,
             reading.value,
             reading.unit
            );

    esp_mqtt_client_publish(client, topic, payload.c_str(), 0, kQoS, kDontRetain);
}

std::string Mqtt::format_json(const sensor::SensorReading &reading) {
    char buffer[kPayloadBufferSize];

    struct tm tm;
    gmtime_r(&reading.timestamp, &tm);
    char ts[25];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm);

    snprintf(buffer,
             sizeof(buffer),
            "{\"sensorId\":\"%s\", \"timestamp\":%s, \"value\":%.2f, \"unit\":\"%s\"}",
            reading.sensorId,
            ts,
            reading.value,
            reading.unit);
    return buffer;
}

} //namespace mqtt