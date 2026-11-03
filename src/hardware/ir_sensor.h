#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include <stdint.h>

enum class HandDirection : uint8_t {
    None,
    Left,
    Right,
    Both
};

struct InfraredReading {
    HandDirection direction;
    bool sampleWindowComplete;

    InfraredReading(HandDirection value = HandDirection::None,
                    bool complete = false)
        : direction(value), sampleWindowComplete(complete)
    {
    }
};

void initializeInfraredSensors();
void resetInfraredSampling(uint32_t now);
InfraredReading updateInfraredSensors(uint32_t now);

#endif
