#include "GarageDoor.h"
#include <stdio.h>

GarageDoor::GarageDoor(StepperMotor& motor, RotaryEncoder& encoder, uint limit_top_pin, uint limit_bottom_pin)
    : _motor(motor),
      _encoder(encoder),
      _limit_top_pin(limit_top_pin),
      _limit_bottom_pin(limit_bottom_pin),
      _state(DoorState::CLOSED),
      _last_moving_state(DoorState::STOPPED),
      _calibrated(false),
      _total_steps(0),
      _current_step(0) {

    gpio_init(_limit_top_pin);
    gpio_set_dir(_limit_top_pin, GPIO_IN);
    gpio_pull_up(_limit_top_pin);

    gpio_init(_limit_bottom_pin);
    gpio_set_dir(_limit_bottom_pin, GPIO_IN);
    gpio_pull_up(_limit_bottom_pin);
}

DoorState GarageDoor::get_state() const {
    return _state;
}

bool GarageDoor::is_calibrated() const {
    return _calibrated;
}

void GarageDoor::start_calibration() {
    printf("Starting calibration...\n");
    _state = DoorState::CALIBRATING;
    _calibrated = false;
    _total_steps = 0;

    // Move down to bottom limit for original point
    while (gpio_get(_limit_bottom_pin)) {
        _motor.step_backward();
        sleep_ms(20); // a slightly slower speed
    }
    _motor.stop();
    _current_step = 0;
    printf("Bottom limit found.\n");
    sleep_ms(500);

    // Move up until top limit is hit and count motor steps
    while (gpio_get(_limit_top_pin)) {
        _motor.step_forward();

        // count the motor steps
        _total_steps++;

        sleep_ms(20);
    }
    _motor.stop();
    printf("Top limit found. Total steps: %d\n", _total_steps);

    _calibrated = true;
    _state = DoorState::OPEN;
    _current_step = _total_steps;
    printf("Calibration complete.\n");
}

void GarageDoor::operate() {
    if (!_calibrated) {
        printf("Error: Not calibrated. Cannot operate.\n");
        return;
    }

    switch (_state) {
        case DoorState::CLOSED:
            move_up();
            break;
        case DoorState::OPEN:
            move_down();
            break;
        case DoorState::OPENING:
        case DoorState::CLOSING:
            stop_moving();
            break;
        case DoorState::STOPPED:
            // Resume in opposite direction
            if (_last_moving_state == DoorState::OPENING) {
                move_down();
            } else if (_last_moving_state == DoorState::CLOSING) {
                move_up();
            }
            break;
        default:
            // Do nothing in other states
            break;
    }
}

void GarageDoor::update() {
    // The encoder is now only used for stuck detection, not position tracking
    Rotation rot = _encoder.get_rotation();
    if (rot != Rotation::NONE) {
        _last_encoder_tick = get_absolute_time();
    }

    // Main state machine
    if (_state == DoorState::OPENING) {
        if (!gpio_get(_limit_top_pin) || _current_step >= _total_steps) {
            stop_moving();
            _state = DoorState::OPEN;
            _current_step = _total_steps;
            printf("Door is OPEN.\n");
        } else {
            _motor.step_forward();
            _current_step++;
            // Check for stuck condition
            if (absolute_time_diff_us(get_absolute_time(), _last_encoder_tick) / 1000 > STUCK_TIMEOUT_MS) {
                _state = DoorState::ERROR_STUCK;
                _motor.stop();
                _calibrated = false; // Require recalibration
                printf("ERROR: Door stuck while opening!\n");
            }
        }
    } else if (_state == DoorState::CLOSING) {
        if (!gpio_get(_limit_bottom_pin) || _current_step <= 0) {
            stop_moving();
            _state = DoorState::CLOSED;
            _current_step = 0;
            printf("Door is CLOSED.\n");
        } else {
            _motor.step_backward();
            _current_step--;
            // Check for stuck condition
            if (absolute_time_diff_us(get_absolute_time(), _last_encoder_tick) / 1000 > STUCK_TIMEOUT_MS) {
                _state = DoorState::ERROR_STUCK;
                _motor.stop();
                _calibrated = false; // Require recalibration
                printf("ERROR: Door stuck while closing!\n");
            }
        }
    }
}

void GarageDoor::move_up() {
    _state = DoorState::OPENING;
    _last_moving_state = DoorState::OPENING;
    _last_encoder_tick = get_absolute_time();
    printf("Door OPENING...\n");
}

void GarageDoor::move_down() {
    _state = DoorState::CLOSING;
    _last_moving_state = DoorState::CLOSING;
    _last_encoder_tick = get_absolute_time();
    printf("Door CLOSING...\n");
}

void GarageDoor::stop_moving() {
    _motor.stop();
    _state = DoorState::STOPPED;
    printf("Door STOPPED.\n");
}