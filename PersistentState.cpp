#include "PersistentState.h"
#include "hardware/sync.h"
#include <cstring>

PersistentState::PersistentState() {
    i2c_init(i2c1,400 *1000); // i2c1 400khz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

uint32_t PersistentState::calculate_checksum(const GarageDoorStateData& data) const {
    // a simple calibrate calculation
    return data.magic + (data.calibrated ? 1 : 0) + data.total_steps + data.current_step;
}

bool PersistentState::load_state(GarageDoorStateData& data) {
    uint8_t buffer[sizeof(GarageDoorStateData)];

    uint8_t mem_addr_buf[2] = {
        (EEPROM_MEM_ADDR >> 8) & 0xFF,
        EEPROM_MEM_ADDR & 0xFF
    };

    // set EEPROM internal address
    if (i2c_write_blocking(i2c1, EEPROM_ADDR, mem_addr_buf, 2, true) != 2) {
        return false;
    }

    // read data
    if (i2c_read_blocking(i2c1, EEPROM_ADDR, buffer, sizeof(buffer), false) != sizeof(buffer)) {
        return false;
    }

    memcpy(&data, buffer, sizeof(GarageDoorStateData));

    if (data.magic != 0xDEADBEEF) {
        return false;
    }

    if (data.checksum != calculate_checksum(data)) {
        return false;
    }

    return true;
}

void PersistentState::save_state(const GarageDoorStateData& data) {
    GarageDoorStateData data_to_save = data;
    data_to_save.magic = 0xDEADBEEF; // set magic number
    data_to_save.checksum = calculate_checksum(data_to_save);

    uint8_t buffer[sizeof(GarageDoorStateData) + 2];
    buffer[0] = (EEPROM_MEM_ADDR >> 8) & 0xFF;
    buffer[1] = EEPROM_MEM_ADDR & 0xFF;
    memcpy(&buffer[2], &data_to_save, sizeof(GarageDoorStateData));

    i2c_write_blocking(i2c1, EEPROM_ADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
}