import threading
from datetime import datetime, timezone

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

