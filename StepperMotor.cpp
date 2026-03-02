#include "StepperMotor.h"

// full-step sequence
const int StepperMotor::_step_sequence[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

StepperMotor::StepperMotor(const uint pins[4]) : _current_step(0) {
    for (int i = 0; i < 4; i++) {
        _pins[i] = pins[i];
        gpio_init(_pins[i]);
        gpio_set_dir(_pins[i], GPIO_OUT);
    }
    stop(); // ensure motor is off at start
}

void StepperMotor::step_forward() {
    _current_step = (_current_step + 1) % 4;
    for (int i = 0; i < 4; i++) {
        gpio_put(_pins[i], _step_sequence[_current_step][i]);
    }
}

void StepperMotor::step_backward() {
    _current_step = (_current_step - 1 + 4) % 4;
    for (int i = 0; i < 4; i++) {
        gpio_put(_pins[i], _step_sequence[_current_step][i]);
    }
}

void StepperMotor::stop() {
    for (int i = 0; i < 4; i++) {
        gpio_put(_pins[i], 0);
    }
}