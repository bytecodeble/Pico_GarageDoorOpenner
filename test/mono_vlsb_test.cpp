//
// Created by Keijo Länsikunnas on 27.2.2025.
//
#include <iostream>
#include <fstream>

#include "mono_vlsb_test.h"

mono_vlsb_test::mono_vlsb_test(uint16_t width, uint16_t height) : mono_vlsb(width, height, width) {
}

void mono_vlsb_test::show() {
    std::ofstream header_file("output.h");
    uint8_t *data = buffer.get();

    header_file << "const unsigned char binary_data[] = {\n";
    header_file << "// font edit begin : monovlsb : " << width << " : " << height << " : " << stride << "\n";


    for (int i = 0; i < size; ++i) {
        if(i && (i % 16 == 0)) header_file << "\n";
        header_file << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
        if(i < size -1) header_file << ", ";
    }
    header_file << "\n";
    header_file << "// font edit end\n";
    header_file << "};";
    header_file << std::endl;
}
