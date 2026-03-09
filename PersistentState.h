#ifndef PICO_MODBUS_PERSISTENTSTATE_H
#define PICO_MODBUS_PERSISTENTSTATE_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Define the data structure that stores in eerpom
struct GarageDoorStateData {
    uint32_t magic; // validate data
    bool calibrated;
    int total_ticks;
    int current_tick;
    uint32_t checksum; // prevent data lost during transition or storing
};

class PersistentState {
public:
    PersistentState();
    // load and save to eeprom
    bool load_state(GarageDoorStateData& data);
    void save_state(const GarageDoorStateData& data);

private:
    uint32_t calculate_checksum(const GarageDoorStateData& data) const;
    // i2c eeprom
    static constexpr uint I2C_SDA_PIN = 14;
    static constexpr uint I2C_SCL_PIN = 15;
    static constexpr uint8_t EEPROM_ADDR = 0x50; // eeprom i2c address
    static constexpr uint16_t EEPROM_MEM_ADDR = 0x0000; // Starting address of data storage
};

#endif //PICO_MODBUS_PERSISTENTSTATE_H