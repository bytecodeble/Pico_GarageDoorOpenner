//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#include "st7789nobuf.h"

#include <hardware/gpio.h>
#include <pico/time.h>

#define ST7735_TFTWIDTH_128 128  // for 1.44 and mini
#define ST7735_TFTWIDTH_80 80    // for mini
#define ST7735_TFTHEIGHT_128 128 // for 1.44" display
#define ST7735_TFTHEIGHT_160 160 // for 1.8" and mini display

#define ST_CMD_DELAY 0x80 // special signifier for command lists

#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1 0xDA
#define ST77XX_RDID2 0xDB
#define ST77XX_RDID3 0xDC
#define ST77XX_RDID4 0xDD

// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00

// clang-format off

static const uint8_t generic_st7789[] =
{
    // Init commands for 7789 screens
    9, //  9 commands in list:
    ST77XX_SWRESET, ST_CMD_DELAY, //  1: Software reset, no args, w/delay
    150, //     ~150 ms delay
    ST77XX_SLPOUT, ST_CMD_DELAY, //  2: Out of sleep mode, no args, w/delay
    10, //      10 ms delay
    ST77XX_COLMOD, 1 + ST_CMD_DELAY, //  3: Set color mode, 1 arg + delay:
    0x55, //     16-bit color
    10, //     10 ms delay
    ST77XX_MADCTL, 1, //  4: Mem access ctrl (directions), 1 arg:
    0x08, //     Row/col addr, bottom-top refresh
    ST77XX_CASET, 4, //  5: Column addr set, 4 args, no delay:
    0x00,
    0, //     XSTART = 0
    0,
    240, //     XEND = 240
    ST77XX_RASET, 4, //  6: Row addr set, 4 args, no delay:
    0x00,
    0, //     YSTART = 0
    320 >> 8,
    320 & 0xFF, //     YEND = 320
    ST77XX_INVON, ST_CMD_DELAY, //  7: hack
    10,
    ST77XX_NORON, ST_CMD_DELAY, //  8: Normal display on, no args, w/delay
    10, //     10 ms delay
    ST77XX_DISPON, ST_CMD_DELAY, //  9: Main screen turn on, no args, delay
    10
}; //    10 ms delay

// clang-format on


st7789nobuf::st7789nobuf(std::shared_ptr<spi_device> dev, uint dc_pin, uint rst_pin, uint bl_pin, uint16_t width,
                         uint16_t height, uint8_t rotation) : framebuf(width, height), spi(dev), gpio_dc(dc_pin),
                                                              gpio_rst(rst_pin), gpio_bl(bl_pin) {
    gpio_init(gpio_dc);
    gpio_set_dir(gpio_dc, GPIO_OUT);

    if(gpio_bl != UINT_MAX) {
        gpio_init(gpio_bl);
        gpio_set_dir(gpio_bl, GPIO_OUT);
    }
    if(gpio_rst != UINT_MAX) {
        gpio_init(gpio_rst);
        gpio_set_dir(gpio_rst, GPIO_OUT);
    }

    set_dc(1);
    set_rst(1);
    spi->set_cs(1);
    sleep_ms(100);

    if (width == 240 && height == 240) {
        // 1.3", 1.54" displays (right justified)
        _rowstart = (320 - height);
        _rowstart2 = 0;
        _colstart = _colstart2 = (240 - width);
    } else if (width == 135 && height == 240) {
        // 1.14" display (centered, with odd size)
        _rowstart = _rowstart2 = (int) ((320 - height) / 2);
        // This is the only device currently supported device that has different
        // values for _colstart & _colstart2. You must ensure that the extra
        // pixel lands in _colstart and not in _colstart2
        _colstart = (int) ((240 - width + 1) / 2);
        _colstart2 = (int) ((240 - width) / 2);
    } else {
        // 1.47", 1.69, 1.9", 2.0" displays (centered)
        _rowstart = _rowstart2 = (int) ((320 - height) / 2);
        _colstart = _colstart2 = (int) ((240 - width) / 2);
    }

    windowWidth = width;
    windowHeight = height;

    displayinit(generic_st7789);
    setrotation(rotation);

    set_bl(1);
}

void st7789nobuf::show() {
    // dummy function - data is written directly to display
}

void st7789nobuf::setpixel(uint16_t x, uint16_t y, uint32_t color) {
    if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
        //SPI_BEGIN_TRANSACTION();
        spi->set_cs(0);
        setaddrwindow(x, y, 1, 1);
        write(static_cast<uint16_t>(color));
        spi->set_cs(1);
        //SPI_END_TRANSACTION();
    }
}

uint32_t st7789nobuf::getpixel(uint16_t x, uint16_t y) const {
    return 0;
}

void st7789nobuf::fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    if ((w >= 0) && (x < width) && (h >= 0) && (y < height)) {
        // clip rectangle
        if (x + w >= width) w = width - x;
        if (y + h >= height) h = height - y;
        //SPI_BEGIN_TRANSACTION();
        spi->set_cs(0);
        setaddrwindow(x, y, w, h);
        for (uint32_t i = w * h; i > 0; --i) {
            write(static_cast<uint16_t>(color));
        }
        spi->set_cs(1);
        //SPI_END_TRANSACTION();
    }
}


void st7789nobuf::displayinit(const uint8_t *addr) {
    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = *addr++; // Number of commands to follow
    while (numCommands--) {
        // For each command...
        cmd = *addr++; // Read command
        numArgs = *addr++; // Number of args to follow
        ms = numArgs & ST_CMD_DELAY; // If hibit set, delay follows args
        numArgs &= ~ST_CMD_DELAY; // Mask out delay bit
        command(cmd, addr, numArgs);
        addr += numArgs;

        if (ms) {
            ms = *addr++; // Read post-command delay time (ms)
            if (ms == 255)
                ms = 500; // If 255, delay for 500 ms
            sleep_ms(ms);
        }
    }
}

void st7789nobuf::setrotation(uint8_t rotation) {
    uint8_t madctl = 0;

    rotation = rotation & 3; // can't be higher than 3

    switch (rotation) {
        case 0:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            width = windowWidth;
            height = windowHeight;
            break;
        case 1:
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart;
            _ystart = _colstart2;
            height = windowWidth;
            width = windowHeight;
            break;
        case 2:
            madctl = ST77XX_MADCTL_RGB;
            _xstart = _colstart2;
            _ystart = _rowstart2;
            width = windowWidth;
            height = windowHeight;
            break;
        case 3:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart2;
            _ystart = _colstart;
            height = windowWidth;
            width = windowHeight;
            break;
    }

    command(ST77XX_MADCTL, &madctl, 1);
}

void st7789nobuf::command(uint8_t cmd, const uint8_t *data, size_t len) {
    //SPI_BEGIN_TRANSACTION();
    spi->set_cs(0);

    set_dc(0); // Command mode
    write(cmd); // Send the command byte
    set_dc(1); // data mode

    if (len > 0) write(data, len);
    spi->set_cs(1);
    //SPI_END_TRANSACTION();
}

void st7789nobuf::write(const uint8_t *data, size_t len) {
    // spi write
    spi->write(data, len);
}

void st7789nobuf::write(uint8_t value) {
    // spi write
    spi->write(&value, 1);
}

void st7789nobuf::write(uint16_t value) {
#if 0
    uint8_t data[2];
    data[0] = value >> 8;
    data[1] = value;
#else
    uint8_t data[] = {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
#endif
    // spi write
    spi->write(data, 2);
}

void st7789nobuf::write(uint32_t value) {
    // convert to big endian before write
    // RP2040 is little endian
#if 0
    uint8_t data[4];
    data[0] = value >> 24;
    data[1] = value >> 16;
    data[2] = value >> 8;
    data[3] = value;
#else
    uint8_t data[] = {
        static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)
    };
#endif
    // spi write
    spi->write(data, 4);
}


void st7789nobuf::setaddrwindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    x += _xstart;
    y += _ystart;
    uint32_t xa = ((uint32_t) x << 16) | (x + w - 1);
    uint32_t ya = ((uint32_t) y << 16) | (y + h - 1);

    writecommand(ST77XX_CASET); // Column addr set
    write(xa);

    writecommand(ST77XX_RASET); // Row addr set
    write(ya);

    writecommand(ST77XX_RAMWR); // write to RAM
}

void st7789nobuf::writecommand(uint8_t cmd) {
    set_dc(0);
    write(cmd);
    set_dc(1);
}

void st7789nobuf::set_dc(bool value) const {
    gpio_put(gpio_dc, value);
}

void st7789nobuf::set_rst(bool value) const {
    if(gpio_rst != UINT_MAX) gpio_put(gpio_rst, value);
}

void st7789nobuf::set_bl(bool value) const {
    if(gpio_bl != UINT_MAX) gpio_put(gpio_bl, value);
}
