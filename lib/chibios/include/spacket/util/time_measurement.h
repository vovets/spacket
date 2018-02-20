#pragma once

#include "chtm.h"

class TimeMeasurement {
public:
    TimeMeasurement() {
        chTMObjectInit(&tm);
    }

    void start() {
        chTMStartMeasurementX(&tm);
    }

    void stop() {
        chTMStopMeasurementX(&tm);
    }
    
    time_measurement_t tm;
};
