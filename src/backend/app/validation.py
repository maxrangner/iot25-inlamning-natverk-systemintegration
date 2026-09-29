import json
from datetime import datetime

REQUIRED_FIELDS = ("sensorId", "timestamp", "value", "unit")
LIMITS = {"C": (0, 50), "%": (20, 90)}


class ValidationError(Exception):
    def __init__(self, category, detail):
        super().__init__(detail)
        self.category = category


def validate(topic, payload):
    try:
        data = json.loads(payload)
    except ValueError:
        raise ValidationError("syntax", "payload is not valid JSON")

    if not isinstance(data, dict):
        raise ValidationError("structure", "payload is not a JSON object")
    for field in REQUIRED_FIELDS:
        if field not in data:
            raise ValidationError("structure", f"missing field {field}")

    sensor_id = data["sensorId"]
    timestamp = data["timestamp"]
    value = data["value"]
    unit = data["unit"]

    for field in ("sensorId", "timestamp", "unit"):
        if not isinstance(data[field], str):
            raise ValidationError("type", f"{field} must be a string")
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValidationError("type", "value must be a number")
    try:
        parsed = datetime.fromisoformat(timestamp)
    except ValueError:
        raise ValidationError("type", "timestamp is not ISO 8601")
    if parsed.tzinfo is None:
        raise ValidationError("type", "timestamp has no time zone")

    if not sensor_id:
        raise ValidationError("structure", "sensorId is empty")
    if sensor_id != topic.rsplit("/", 1)[-1]:
        raise ValidationError("structure", "sensorId does not match topic")

    if unit not in LIMITS:
        raise ValidationError("range", f"unknown unit {unit}")
    low, high = LIMITS[unit]
    if not low <= value <= high:
        raise ValidationError("range", f"value {value} outside {low}-{high} {unit}")

    return {"sensorId": sensor_id, "timestamp": timestamp, "value": value, "unit": unit}
