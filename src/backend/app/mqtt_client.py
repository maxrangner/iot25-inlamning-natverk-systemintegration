import logging

import paho.mqtt.client as mqtt

from .validation import ValidationError, validate

CLIENT_ID = "iot25-backend"
SENSOR_TOPIC = "iot25/max/sensor/+"
STATUS_TOPIC = "iot25/max/status/esp32-c3-01"
QOS = 1

log = logging.getLogger(__name__)


def create_client(store):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=CLIENT_ID, userdata=store)
    client.on_connect = on_connect
    client.on_connect_fail = on_connect_fail
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.reconnect_delay_set(min_delay=1, max_delay=60)
    return client


def on_connect(client, store, flags, reason_code, properties):
    if reason_code.is_failure:
        log.error("event=mqtt_connect_refused reason=%s", reason_code)
        return
    store.set_mqtt_connected(True)
    client.subscribe([(SENSOR_TOPIC, QOS), (STATUS_TOPIC, QOS)])
    log.info("event=mqtt_connected")


def on_connect_fail(client, store):
    log.error("event=mqtt_connect_failed")


def on_disconnect(client, store, flags, reason_code, properties):
    store.set_mqtt_connected(False)
    log.warning("event=mqtt_disconnected reason=%s", reason_code)


def on_message(client, store, message):
    try:
        if message.topic == STATUS_TOPIC:
            online = message.payload == b"online"
            store.set_device_online(online)
            log.info("event=device_status online=%s", online)
            return

        reading = validate(message.topic, message.payload)
        store.add_reading(reading)
        log.info(
            "event=reading_accepted sensorId=%s value=%s unit=%s",
            reading["sensorId"], reading["value"], reading["unit"],
        )
    except ValidationError as error:
        store.add_rejected(error.category)
        log.warning(
            "event=reading_rejected category=%s topic=%s detail=%s payload=%r",
            error.category, message.topic, error, message.payload[:200],
        )
    except Exception:
        log.exception("event=message_error topic=%s", message.topic)
