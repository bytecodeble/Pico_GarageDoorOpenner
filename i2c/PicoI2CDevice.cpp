//
// Created by Keijo Länsikunnas on 4.3.2025.
//

#include "PicoI2CDevice.h"

PicoI2CDevice::PicoI2CDevice(std::shared_ptr<PicoI2CBus> i2c_bus, uint8_t address) : i2c_bus(std::move(i2c_bus)), dev_address(address) {
}

unsigned int PicoI2CDevice::write(const uint8_t *buffer, unsigned int length) {
    return i2c_bus->write(dev_address, buffer, length);
}

unsigned int PicoI2CDevice::read(uint8_t *buffer, unsigned int length) {
    return i2c_bus->read(dev_address, buffer, length);
}

unsigned int PicoI2CDevice::transaction(const uint8_t *wbuffer, unsigned int wlength, uint8_t *rbuffer,
    unsigned int rlength) {
    return i2c_bus->transaction(dev_address, wbuffer, wlength, rbuffer, rlength);
}
