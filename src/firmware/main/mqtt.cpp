#include "mqtt.h"
#include "esp_log.h"

namespace mqtt {

constexpr char TAG[] = "mqtt";

constexpr char kTopic[] = "iot25/max/status/esp32-c3-01";
constexpr uint8_t kQoS = 1;
constexpr uint8_t kRetain = 1;
constexpr char kOnlineMsg[] = "online";
constexpr char kOfflineMsg[] = "offline";

void Mqtt::start() {
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = CONFIG_IOT25_MQTT_BROKER_URI;
    cfg.credentials.client_id = "esp32-c3-01";
    cfg.session.last_will.topic = kTopic;
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
        self->publish(kTopic, kOnlineMsg);
    } else if (id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Mqtt disconnected");
    } else if (id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "Mqtt error");
    }
}

void Mqtt::publish(const char* topic, const char* payload) {
    ESP_LOGI(TAG,
             "\n\n"
             "┌─ MQTT Publish ────────────────────────────\n"
             "│ topic: %s\n"
             "│ payload: %s\n"
             "└───────────────────────────────────────────\n",
             topic,
             payload);
    esp_mqtt_client_publish(client, topic, payload, 0, kQoS, kRetain);
}

} //namespace mqtt