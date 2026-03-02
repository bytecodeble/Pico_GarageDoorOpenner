#ifndef PICO_MODBUS_ROTARYENCODER_H
#define PICO_MODBUS_ROTARYENCODER_H
#pragma once
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/util/queue.h"

enum class Rotation {
    NONE,
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

class RotaryEncoder {
public:
    RotaryEncoder(uint pin_a, uint pin_b);
    Rotation get_rotation();

private:
    static void gpio_callback(uint gpio, uint32_t events);

    static uint _pin_a;
    static uint _pin_b;
    static queue_t _event_queue;
};


#endif //PICO_MODBUS_ROTARYENCODER_H