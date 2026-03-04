#ifndef PICO_MODBUS_PERSISTENTSTATE_H
#define PICO_MODBUS_PERSISTENTSTATE_H

#include "pico/stdlib.h"
#include "hardware/flash.h"

// Define the data structure that stores in Flash
struct GarageDoorStateData {
    uint32_t magic; // validate data
    bool calibrated;
    int total_steps;
    int current_step;
    uint32_t checksum; // prevent data lost during transition or storing
};

class PersistentState {
public:
    PersistentState();
    // load and save to Flash
    bool load_state(GarageDoorStateData& data);
    void save_state(const GarageDoorStateData& data);

private:
    uint32_t calculate_checksum(const GarageDoorStateData& data) const;
    // Flash target offset must be a multiple of the sector size
    // save at the very end of Flash
    static constexpr uint32_t FLASH_TARGET_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
};

#endif //PICO_MODBUS_PERSISTENTSTATE_H