#ifndef PICO_MODBUS_GARAGEDOOR_H
#define PICO_MODBUS_GARAGEDOOR_H

#pragma once
#include "StepperMotor.h"
#include "RotaryEncoder.h"
#include "PersistentState.h"


enum class DoorState {
    CLOSED,
    OPEN,
    OPENING,
    CLOSING,
    STOPPED,
    CALIBRATING,
    ERROR_STUCK
};

class GarageDoor {
public:
    GarageDoor(StepperMotor& motor, RotaryEncoder& encoder, uint limit_top_pin, uint limit_bottom_pin, uint led_error_pin);

    void start_calibration();
    void operate(); // main operation button
    void update();  // repeatedly called in the main loop
    void update_leds();

    DoorState get_state() const;
    const char* get_state_string() const;
    bool is_calibrated() const;

private:
    void move_up();
    void move_down();
    void stop_moving();
    void save_state();
    void check_stuck(const char* direction);

    StepperMotor& _motor;
    RotaryEncoder& _encoder;
    uint _limit_top_pin;
    uint _limit_bottom_pin;

    uint _led_error_pin;
    absolute_time_t _led_blink_timer;

    static constexpr uint32_t MOTOR_DELAY_MS = 2;

    DoorState _state;
    DoorState _last_moving_state; // To remember direction after stopping

    bool _calibrated;
    int _total_steps;
    int _current_step;

    absolute_time_t _last_encoder_tick;
    static const uint STUCK_TIMEOUT_MS = 500;

    PersistentState _persistent_state;
};

#endif //PICO_MODBUS_GARAGEDOOR_H