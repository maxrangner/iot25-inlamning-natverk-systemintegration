import threading
from datetime import datetime, timezone

STALE_AFTER_SECONDS = 30
CATEGORIES = ("syntax", "structure", "type", "range")


def now_utc():
    return datetime.now(timezone.utc)


class Store:
    def __init__(self):
        self.lock = threading.Lock()
        self.readings = {}
        self.messages_received = 0
        self.messages_accepted = 0
        self.validation_errors = {category: 0 for category in CATEGORIES}
        self.mqtt_connected = False
        self.mqtt_reconnects = 0
        self.has_connected = False
        self.device_online = False
        self.last_reading_at = None
        self.started_at = now_utc()

    def add_reading(self, reading):
        now = now_utc()
        reading["receivedAt"] = now.isoformat(timespec="seconds")
        with self.lock:
            self.readings[reading["sensorId"]] = reading
            self.messages_received += 1
            self.messages_accepted += 1
            self.last_reading_at = now

    def add_rejected(self, category):
        with self.lock:
            self.messages_received += 1
            self.validation_errors[category] += 1

    def set_mqtt_connected(self, connected):
        with self.lock:
            if connected and self.has_connected:
                self.mqtt_reconnects += 1
            if connected:
                self.has_connected = True
            self.mqtt_connected = connected

    def set_device_online(self, online):
        with self.lock:
            self.device_online = online

    def latest(self):
        with self.lock:
            return list(self.readings.values())

    def get(self, sensor_id):
        with self.lock:
            return self.readings.get(sensor_id)

    def status(self):
        now = now_utc()
        with self.lock:
            if self.last_reading_at is None:
                last_reading_at = None
                seconds_since_last = None
            else:
                last_reading_at = self.last_reading_at.isoformat(timespec="seconds")
                seconds_since_last = round((now - self.last_reading_at).total_seconds(), 1)

            return {
                "mqttConnected": self.mqtt_connected,
                "deviceOnline": self.device_online,
                "messagesReceived": self.messages_received,
                "messagesAccepted": self.messages_accepted,
                "validationErrors": dict(self.validation_errors),
                "mqttReconnects": self.mqtt_reconnects,
                "lastReadingAt": last_reading_at,
                "secondsSinceLastReading": seconds_since_last,
                "stale": seconds_since_last is None or seconds_since_last > STALE_AFTER_SECONDS,
                "startedAt": self.started_at.isoformat(timespec="seconds"),
                "uptimeSeconds": round((now - self.started_at).total_seconds()),
            }
