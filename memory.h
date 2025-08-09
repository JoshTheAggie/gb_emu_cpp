//
// Created by joshua on 3/29/22.
//

#ifndef GBEMUJM_MEMORY_H
#define GBEMUJM_MEMORY_H

#include <fstream>
#include <iostream>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

enum MBC {NONE, MBC1, MBC2};

class memory {
    uint8_t system_mem [0x10000]{};    //64KiB of memory
    uint8_t bootrom [256]{}; //256 byte bootstrap
    uint8_t cartridge_rom [0x200000]{}; //cartridge rom
    //rom banking
    MBC mbc_type = NONE;
    uint8_t currentROMbank_lo5 = 1;
    uint16_t number_of_rom_banks = 2;
    //ram banks
    uint8_t rambanks [0x8000]{};
    uint8_t currentRAMbank_rombank_hi2 = 0;
    uint16_t number_of_ram_banks = 0;
    void handlebanking(uint16_t address, uint8_t value);
    bool enableRAM = false;
    bool MBC1bankingmode1 = false;
    void ramenable(uint16_t address, uint8_t data);
    void changeLorombank(uint8_t data);
    void changeHirombank(uint8_t data);
    void rambankchange(uint8_t data);
    void changeromrammode(uint8_t data);
public:
    uint8_t buttons = 0xFF;
    uint8_t directions = 0xFF;
    memory();

    void write_mem(uint16_t address, uint8_t value);
    uint8_t read_mem(uint16_t address) const;
    void request_interrupt(uint8_t irq_num);
    void performDMAtransfer(uint8_t data);
    void LoadROM(char const *filename);
    void incrementLY();
};


#endif //GBEMUJM_MEMORY_H
