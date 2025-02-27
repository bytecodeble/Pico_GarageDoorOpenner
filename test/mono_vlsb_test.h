//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#ifndef MONO_VLSB_TEST_H
#define MONO_VLSB_TEST_H

#include "mono_vlsb.h"

class mono_vlsb_test : public mono_vlsb {
public:
    explicit mono_vlsb_test(uint16_t width = 128, uint16_t height = 64);
    void show();
};



#endif //MONO_VLSB_TEST_H
