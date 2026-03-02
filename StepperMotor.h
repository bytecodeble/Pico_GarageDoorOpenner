#ifndef PICO_MODBUS_STEPPERMOTOR_H
#define PICO_MODBUS_STEPPERMOTOR_H

#pragma once
#include "pico/stdlib.h"


class StepperMotor {
public:
    StepperMotor(const uint pins[4]);
    void step_forward();
    void step_backward();
    void stop();

private:
    uint _pins[4];
    int _current_step;
    static const int _step_sequence[4][4];
};


#endif //PICO_MODBUS_STEPPERMOTOR_H