#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

#include "input_filters.h"

void initializeTouchSensor();
TouchEvents updateTouchSensor(uint32_t now);

#endif
