#pragma once

#include <time.h>

struct SensorReading {
    const char* sensorId;
    time_t timestamp;
    float value;
    const char* unit;
};
