#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "pico/stdlib.h"

void ultrasonic_init(uint trig_pin, uint echo_pin);
float measure_distance();

#endif // ULTRASONIC_H
