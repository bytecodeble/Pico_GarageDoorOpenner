//
// Created by Keijo Länsikunnas on 4.3.2025.
//

#ifndef PICOI2CBUS_H
#define PICOI2CBUS_H

#include <cstdint>
#include <hardware/i2c.h>

class PicoI2CBus {
public:
    explicit PicoI2CBus(unsigned int bus_nr, unsigned int sda_pin, unsigned int scl_pin, unsigned int speed = 100000);
    PicoI2CBus(const PicoI2CBus &) = delete;

    unsigned int write(uint8_t address, const uint8_t *buffer, unsigned int length);
    unsigned int read(uint8_t address, uint8_t *buffer, unsigned int length);
    unsigned int transaction(uint8_t address, const uint8_t *wbuffer, unsigned int wlength, uint8_t *rbuffer, unsigned int rlength);
private:
    i2c_inst *i2c;

};



#endif //PICOI2CBUS_H
