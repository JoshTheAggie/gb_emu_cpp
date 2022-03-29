//
// Created by joshua on 3/29/22.
//

#ifndef GBEMUJM_MEMORY_H
#define GBEMUJM_MEMORY_H


#include <cstdint>
#include <cstdio>
#include <fstream>

class memory {
    uint8_t system_mem [0x10000]{};    //64KiB of memory
    uint8_t bootrom [256]{}; //256 byte bootstrap
    uint8_t cartridge_rom [0x200000]{}; //cartridge rom
public:
    uint8_t buttons = 0xFF;
    uint8_t directions = 0xFF;
    memory();

    void write_mem(uint16_t address, uint8_t value);
    uint8_t read_mem(uint16_t address);
    void performDMAtransfer(uint8_t data);
    void LoadROM(char const *filename);
    void incrementLY();
};


#endif //GBEMUJM_MEMORY_H
