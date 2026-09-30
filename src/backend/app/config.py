import os
import sys


def require(name):
    value = os.environ.get(name)
    if not value:
        sys.exit(f"Missing environment variable {name}")
    return value


MQTT_HOST = require("MQTT_HOST")
MQTT_PORT = int(require("MQTT_PORT"))
