#pragma once

#include <time.h>

namespace sensor {

struct SensorReading {
    const char* sensorId;
    time_t timestamp;
    float value;
    const char* unit;
};

} //namespace sensor
