#include "GarageDoor.h"
#include <stdio.h>

GarageDoor::GarageDoor(StepperMotor& motor, RotaryEncoder& encoder, uint limit_top_pin, uint limit_bottom_pin, const uint led_pins[3])
    : _motor(motor),
      _encoder(encoder),
      _limit_top_pin(limit_top_pin),
      _limit_bottom_pin(limit_bottom_pin),
      _state(DoorState::CLOSED), //default state
      _last_moving_state(DoorState::STOPPED),
      _calibrated(false),
      _total_encoder_ticks(0),
      _current_encoder_ticks(0) {

    gpio_init(_limit_top_pin);
    gpio_set_dir(_limit_top_pin, GPIO_IN);
    gpio_pull_up(_limit_top_pin);

    gpio_init(_limit_bottom_pin);
    gpio_set_dir(_limit_bottom_pin, GPIO_IN);
    gpio_pull_up(_limit_bottom_pin);

    _led_pins[0] = led_pins[0];
    _led_pins[1] = led_pins[1];
    _led_pins[2] = led_pins[2];
    for (int i = 0; i < 3; i++) {
        gpio_init(_led_pins[i]);
        gpio_set_dir(_led_pins[i], GPIO_OUT);
    }
    _led_blink_timer = get_absolute_time();

    // try to load status from EEPROM
    GarageDoorStateData loaded_data;
    if (_persistent_state.load_state(loaded_data)) {
        _calibrated = loaded_data.calibrated;
        _total_encoder_ticks = loaded_data.total_ticks;
        _current_encoder_ticks = loaded_data.current_tick;
        printf("Loaded state from EEPROM: cal=%d, total=%d, current=%d\n", _calibrated, _total_encoder_ticks, _current_encoder_ticks);

        // Determine the initial state based on the loaded status
        if (_calibrated) {
            if (_current_encoder_ticks <= 0) _state = DoorState::CLOSED;
            else if (_current_encoder_ticks >= _total_encoder_ticks) _state = DoorState::OPEN;
            else _state = DoorState::STOPPED; // assume it at the middle
        } else {
            printf("No valid state in EEPROM. Please calibrate.\n");
            _state = DoorState::CLOSED; // default state
        }
    }
}

DoorState GarageDoor::get_state() const {
    return _state;
}

bool GarageDoor::is_calibrated() const {
    return _calibrated;
}

const char* GarageDoor::get_state_string() const {
    switch (_state) {
        case DoorState::CLOSED: return "CLOSED";
        case DoorState::OPEN: return "OPEN";
        case DoorState::OPENING:
        case DoorState::CLOSING:
        case DoorState::STOPPED: return "IN_BETWEEN";
        case DoorState::CALIBRATING: return "CALIBRATING";
        case DoorState::ERROR_STUCK: return "ERROR_STUCK";
        default: return "UNKNOWN";
    }
}

void GarageDoor::save_state() {
    GarageDoorStateData data_to_save{
        .calibrated = _calibrated,
        .total_ticks = _total_encoder_ticks,
        .current_tick = _current_encoder_ticks
    };

    _persistent_state.save_state(data_to_save);
    printf("State saved.\n");
}

void GarageDoor::start_calibration() {
    printf("Starting calibration...\n");
    _state = DoorState::CALIBRATING;
    _calibrated = false;
    _total_encoder_ticks = 0;

    gpio_put(_led_pins[0], 1);
    gpio_put(_led_pins[1], 1);
    gpio_put(_led_pins[2], 1);

    // Move down to bottom limit for original point
    _last_encoder_tick = get_absolute_time();
    while (gpio_get(_limit_bottom_pin)) {
        _motor.step_backward();
        sleep_ms(MOTOR_DELAY_MS);

        // check for stuck
        Rotation rot = _encoder.get_rotation();
        if (rot != Rotation::NONE) _last_encoder_tick = get_absolute_time();
        if (absolute_time_diff_us(_last_encoder_tick, get_absolute_time()) / 1000 > STUCK_TIMEOUT_MS) {
            _motor.stop();
            _state = DoorState::ERROR_STUCK;
            printf("ERROR: Door stuck during calibration (going down)!\n");
            return;
        }
    }
    _motor.stop();
    _current_encoder_ticks = 0;
    printf("Bottom limit found.\n");
    sleep_ms(500);

    // Move up until top limit is hit and count motor steps
    _last_encoder_tick = get_absolute_time();
    while (gpio_get(_limit_top_pin)) {
        _motor.step_forward();
        // count the motor steps
        Rotation rot = _encoder.get_rotation();
        if (rot == Rotation::CLOCKWISE) {
            _total_encoder_ticks++;
        }else if (rot == Rotation::COUNTER_CLOCKWISE) {
            _total_encoder_ticks--;
        }
        sleep_ms(MOTOR_DELAY_MS);

        if (rot != Rotation::NONE) _last_encoder_tick = get_absolute_time();
        if (absolute_time_diff_us(_last_encoder_tick, get_absolute_time()) / 1000 > STUCK_TIMEOUT_MS) {
            _motor.stop();
            _state = DoorState::ERROR_STUCK;
            _total_encoder_ticks = 0;
            printf("ERROR: Door stuck during calibration (going up)!\n");
            return;
        }
    }
    _motor.stop();
    printf("Top limit found. Total encoder ticks: %d\n", _total_encoder_ticks);
    printf("Ratio: %.1f motor steps per encoder tick\n", 6800.0f / _total_encoder_ticks);

    _calibrated = true;
    _state = DoorState::OPEN;
    _current_encoder_ticks = _total_encoder_ticks;
    _last_encoder_tick = get_absolute_time();
    save_state();
    printf("Calibration complete.\n");
}

void GarageDoor::operate() {
    if (!_calibrated) {
        printf("ERROR: Not calibrated. Cannot operate.\n");
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
        case DoorState::ERROR_STUCK:
            break;
        default:
            // Do nothing in other states
            break;
    }
}

void GarageDoor::update() {
    Rotation rot = _encoder.get_rotation();
    if (rot != Rotation::NONE) {
        _last_encoder_tick = get_absolute_time();
        if (rot == Rotation::CLOCKWISE) {
            _current_encoder_ticks++;
        } else if (rot == Rotation::COUNTER_CLOCKWISE) {
            _current_encoder_ticks--;
        }
    }

    // Main state machine
    if (_state == DoorState::OPENING) {
        if (is_limit_triggered(_limit_top_pin) || _current_encoder_ticks >= _total_encoder_ticks) {
            _motor.stop();
            _state = DoorState::OPEN;
            _current_encoder_ticks = _total_encoder_ticks;
            save_state();
            printf("Door is OPEN.\n");
        } else {
            _motor.step_forward();
            sleep_ms(MOTOR_DELAY_MS);
            check_stuck("opening");
        }
    } else if (_state == DoorState::CLOSING) {
        if (is_limit_triggered(_limit_bottom_pin) || _current_encoder_ticks <= 0) {
            _motor.stop();
            _state = DoorState::CLOSED;
            _current_encoder_ticks = 0;
            save_state();
            printf("Door is CLOSED.\n");
        } else {
            _motor.step_backward();
            sleep_ms(MOTOR_DELAY_MS);
            check_stuck("closing");
        }
    }
}

bool GarageDoor::is_limit_triggered(uint pin) {
    if (!gpio_get(pin)) {
        sleep_ms(5);
        return !gpio_get(pin);
    }
    return false;
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
    save_state();
    printf("Door STOPPED.\n");
}

void GarageDoor::check_stuck(const char *direction) {
    // Check for stuck condition
    if (absolute_time_diff_us(_last_encoder_tick, get_absolute_time()) / 1000 > STUCK_TIMEOUT_MS) {
        _state = DoorState::ERROR_STUCK;
        _motor.stop();
        _calibrated = false; // Require recalibration
        save_state();
        printf("ERROR: Door stuck while %s!\n", direction);
    }
}

void GarageDoor::update_leds() {
    switch (_state) {
        case DoorState::OPENING:
            // led 2 on when opening
            gpio_put(_led_pins[0], 0);
            gpio_put(_led_pins[1], 1);
            gpio_put(_led_pins[2], 0);
            break;

        case DoorState::CLOSING:
            // led 3 on when closing
            gpio_put(_led_pins[0], 0);
            gpio_put(_led_pins[1], 0);
            gpio_put(_led_pins[2], 1);
            break;

        case DoorState::CALIBRATING:
            // all turned on
            gpio_put(_led_pins[0], 1);
            gpio_put(_led_pins[1], 1);
            gpio_put(_led_pins[2], 1);
            break;

        case DoorState::ERROR_STUCK:
            // all blink if stuck
            if (absolute_time_diff_us(_led_blink_timer, get_absolute_time()) > 500000) {
                // reverse status of all leds
                gpio_put(_led_pins[0], !gpio_get(_led_pins[0]));
                gpio_put(_led_pins[1], !gpio_get(_led_pins[1]));
                gpio_put(_led_pins[2], !gpio_get(_led_pins[2]));
                _led_blink_timer = get_absolute_time();
            }
            break;

        case DoorState::OPEN:
        case DoorState::CLOSED:
        case DoorState::STOPPED:
        default:
            // LED 1 breathing
            if (absolute_time_diff_us(_led_blink_timer, get_absolute_time()) > 1000000) {
                gpio_put(_led_pins[0], !gpio_get(_led_pins[0]));
                _led_blink_timer = get_absolute_time();
            }
            // turn off the other two lights
            gpio_put(_led_pins[1], 0);
            gpio_put(_led_pins[2], 0);
            break;
    }
}

void GarageDoor::reset_stuck_timer() {
    _last_encoder_tick = get_absolute_time(); // Exposed to external manual refresh time
}