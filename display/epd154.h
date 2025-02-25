//
// Created by Keijo Länsikunnas on 8.3.2024.
//

#ifndef PICO_MODBUS_EPD154_H
#define PICO_MODBUS_EPD154_H

#include "mono_horiz.h"

typedef unsigned char UBYTE;
typedef unsigned short UWORD;

class epd154 : public mono_horiz {
public:
    epd154();
    void show();
private:
    void EPD_1IN54_V2_Init();
    void EPD_1IN54_V2_Reset();
    void EPD_1IN54_V2_SendCommand(UBYTE Reg);
    void EPD_1IN54_V2_SendData(UBYTE Data);
    void EPD_1IN54_V2_ReadBusy();
    void EPD_1IN54_V2_TurnOnDisplay();
    void EPD_1IN54_V2_TurnOnDisplayPart();
    void EPD_1IN54_V2_Lut(UBYTE *lut);
    void EPD_1IN54_V2_SetLut(UBYTE *lut);
    void EPD_1IN54_V2_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend);
    void EPD_1IN54_V2_SetCursor(UWORD Xstart, UWORD Ystart);

    void DEV_Digital_Write(uint8_t pin, bool val);
    bool DEV_Digital_Read(uint8_t pin);
    void DEV_SPI_WriteByte(uint8_t val);
    void DEV_Delay_ms(uint32_t ms);
    int DEV_Module_Init();
    void DEV_Module_Exit();

};


#endif //PICO_MODBUS_EPD154_H
