//
// Created by Keijo Länsikunnas on 4.3.2025.
//

#include "PicoI2CBus.h"

#include <hardware/gpio.h>

PicoI2CBus::PicoI2CBus(unsigned int bus_nr, unsigned int sda_pin, unsigned int scl_pin, unsigned int speed) {
    if(bus_nr == 0) {
        i2c = i2c0;
    }
    else {
        i2c = i2c1;
    }
    // I2C is "open drain",
    // pull ups to keep signal high when no data is being sent
    // external pullups should be used to guarantee proper operation
    i2c_init(i2c, speed);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C); // the display has external pull-ups
    gpio_set_function(sda_pin, GPIO_FUNC_I2C); // the display has external pull-ups
    gpio_pull_up(scl_pin);
    gpio_pull_up(sda_pin);

}

unsigned int PicoI2CBus::write(uint8_t address, const uint8_t *buffer, unsigned int length) {
    return i2c_write_blocking(i2c, address, buffer, length, false);
}

unsigned int PicoI2CBus::read(uint8_t address, uint8_t *buffer, unsigned int length) {
    return i2c_read_blocking(i2c, address, buffer, length, false);
}

unsigned int PicoI2CBus::transaction(uint8_t address, const uint8_t *wbuffer, unsigned int wlength, uint8_t *rbuffer,
    unsigned int rlength) {
    if(i2c_write_blocking(i2c, address, wbuffer, wlength, true) == PICO_ERROR_GENERIC) return 0;
    return i2c_read_blocking(i2c, address, rbuffer, rlength, false);
}
