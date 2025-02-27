//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#ifndef MONO_HLSB_TEST_H
#define MONO_HLSB_TEST_H

#include "mono_horiz.h"

class mono_hlsb_test  : public mono_horiz {
public:
    explicit mono_hlsb_test(uint16_t width = 200, uint16_t height = 200);
    void show();
};



#endif //MONO_HLSB_TEST_H
