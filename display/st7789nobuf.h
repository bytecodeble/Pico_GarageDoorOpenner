//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#ifndef ST7789NOBUF_H
#define ST7789NOBUF_H

#include <hardware/spi.h>

#include "framebuf.h"

class st7789nobuf : public framebuf {
public:
    struct Config {
        spi_inst_t* spi;
        uint gpio_din;
        uint gpio_clk;
        int gpio_cs;
        uint gpio_dc;
        uint gpio_rst;
        uint gpio_bl;
    };

    explicit st7789nobuf(uint16_t width = 240, uint16_t height = 240, uint8_t rotation = 0);
    void show();
private:
    void setpixel(uint16_t x, uint16_t y, uint32_t color) override;
    uint32_t getpixel(uint16_t x, uint16_t y) const override;
    void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) override;

    // private functions for handling the device
    void displayinit(const uint8_t *addr);
    void setrotation(uint8_t rotation);
    void command(uint8_t cmd, const uint8_t* data, size_t len);
    void write(const uint8_t *data, size_t len);
    void write(uint8_t value);
    void write(uint16_t value);
    void write(uint32_t value);
    void setaddrwindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void writecommand(uint8_t cmd);
    void set_cs(bool value) const;
    void set_dc(bool value) const;
    void set_bl(bool value) const;

    uint8_t _colstart = 0; ///< Some displays need this changed to offset
    uint8_t _rowstart = 0; ///< Some displays need this changed to offset
    uint8_t _colstart2 = 0; ///< Offset from the right
    uint8_t _rowstart2 = 0; ///< Offset from the bottom
    uint8_t _xstart = 0;
    uint8_t _ystart = 0;

    uint16_t windowWidth;
    uint16_t windowHeight;
    bool data_mode;
    bool use_cs;
    Config config_;
};



#endif //ST7789NOBUF_H
