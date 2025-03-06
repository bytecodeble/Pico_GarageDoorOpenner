//
// Created by Keijo Länsikunnas on 18.2.2024.
//

#ifndef PICO_MODBUS_SSD1306_H
#define PICO_MODBUS_SSD1306_H

#include <memory>
#include "mono_vlsb.h"
#include "i2c_device.h"

class ssd1306 : public mono_vlsb {
public:
    explicit ssd1306(std::shared_ptr<i2c_device> dev, uint16_t width = 128, uint16_t height = 64);
    void show();
private:
    void init();
    void send_cmd(uint8_t value);
    std::shared_ptr<i2c_device> i2c;
    uint8_t address;
};


#endif //PICO_MODBUS_SSD1306_H
