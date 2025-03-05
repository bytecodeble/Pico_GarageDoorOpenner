//
// Created by Keijo Länsikunnas on 4.3.2025.
//

#ifndef PICOI2CDEVICE_H
#define PICOI2CDEVICE_H

#include <memory>

#include "i2c_device.h"
#include "PicoI2CBus.h"

class PicoI2CDevice : public i2c_device {
public:
    explicit PicoI2CDevice(std::shared_ptr<PicoI2CBus> i2c_bus, uint8_t address);
    PicoI2CDevice(const PicoI2CDevice &) = delete;

    unsigned int write(const uint8_t *buffer, unsigned int length) override;
    unsigned int read(uint8_t *buffer, unsigned int length) override;
    unsigned int transaction(const uint8_t *wbuffer, unsigned int wlength, uint8_t *rbuffer, unsigned int rlength) override;
private:
    std::shared_ptr<PicoI2CBus> i2c_bus;
    uint8_t dev_address;
};



#endif //PICOI2CDEVICE_H
