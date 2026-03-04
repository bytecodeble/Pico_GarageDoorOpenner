#include "PersistentState.h"
#include "hardware/sync.h"
#include <cstring>

PersistentState::PersistentState() {
    // leave it empty
}

uint32_t PersistentState::calculate_checksum(const GarageDoorStateData& data) const {
    // a simple calibrate calculation
    return data.magic + (data.calibrated ? 1 : 0) + data.total_steps + data.current_step;
}

static GarageDoorStateData temp_data;

bool PersistentState::load_state(GarageDoorStateData& data) {
    // read data from Flash
    // const uint8_t* flash_data_ptr = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    // memcpy(&data, flash_data_ptr, sizeof(GarageDoorStateData));
    //
    // // validate magic and checksum
    // if (data.magic != 0xDEADBEEF || data.checksum != calculate_checksum(data)) {
    //     return false;
    // }
    data = temp_data;
    return true;
}

void PersistentState::save_state(const GarageDoorStateData& data) {
    GarageDoorStateData data_to_save = data;
    data_to_save.magic = 0xDEADBEEF; // set magic number
    data_to_save.checksum = calculate_checksum(data_to_save);
    temp_data=data_to_save;
    // disable interrupts and write it in Flash
    // uint32_t ints = save_and_disable_interrupts();
    // flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    // flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&data_to_save, sizeof(GarageDoorStateData));
    // restore_interrupts(ints);
}