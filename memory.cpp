//
// Created by joshua on 3/29/22.
//

#include "memory.h"

memory::memory() {
    //load boot rom
    //open file as binary stream and move file pointer to end
    std::ifstream file("gb_bios.bin", std::ios::binary | std::ios::ate);
    if(file.is_open())
    {
        char* buffer = new char[256];

        //go back to beginning of file and fill buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, 256);
        file.close();

        //load ROM contents into the gb's memory, starting at 0x0
        for (int i = 0; i < 256; i++)
        {
            bootrom[i] = buffer[i];
        }

        //free the buffer
        delete[] buffer;
    }
    else
    {
        std::cerr << "Cannot load boot rom!\n";
    }
}

void memory::write_mem(uint16_t address, uint8_t value) {
//todo: implement rom banking, any other indirection, etc.
    //don't allow writes to read-only memory
    if (address < 0x8000)
    {
        handlebanking(address, value);
    }
    else if (address >= 0xA000 & address < 0xC000)
    {
        if (enableRAM)
        {
            uint16_t newaddress = address - 0xA000;
            rambanks[newaddress + (currentRAMbank * 0x2000)] = value;
        }
    }
        // writes to ECHO RAM writes to RAM as well
    else if ((address >= 0xE000) && (address < 0xFE00))
    {
        system_mem[address] = value;
        write_mem(address-0x2000, value);
    }
        //this area is restricted
    else if (( address >= 0xFEA0 ) && (address < 0xFEFF))
    {
        //printf("Attempted write to restricted memory, address %x\n", address);
    }
    else if (address == 0xFF04){
        //attempted write to timer div register, resets it
        system_mem[0xFF04] = 0;
    }
    else if (address == 0xFF44)
    {
        //attempted write to scanline counter, resets it
        system_mem[address] = 0;
    }
    else if (address == 0xFF46)
    {
        //DMA time
        performDMAtransfer(value);
    }
    else if(address >= 0x8000 && address < 0xA000)
    {
        //printf("Write to VRAM. Address: %X\tData: %X\n", address, value);
        system_mem[address] = value;
    }
    else
        system_mem[address] = value;
}

uint8_t memory::read_mem(uint16_t address) const {
    //todo: go back and replace all the '&memory[HL()]' calls
    if (address == 0xFF00){
        //joypad register
        if((system_mem[0xFF00] & 0x20) == 0)
        {
            //action buttons selected
            return ((system_mem[0xFF00] & 0xF0) | (buttons & 0x0F));
        }
        else //if((system_mem[0xFF00] & 0x10) == 0)
        {
            //todo: ensure the above commenting out works
            //we operate under the assumption that exactly one or the other is 0
            //direction buttons selected
            return ((system_mem[0xFF00] & 0xF0) | (directions & 0x0F));
        }
    }
    else if (address < 0x100 && system_mem[0xFF50] == 0x00)
    {
        return bootrom[address];
    }
    else if (address >= 0x4000 && address < 0x8000) {
        //reading from ROM bank
        uint16_t newaddress = address - 0x4000;
        return cartridge_rom[newaddress + (currentROMbank * 0x4000)];
    }
    else if (address >= 0xA000 && address < 0xC000)
    {
        //reading from RAM bank
        uint16_t newaddress = address - 0xA000;
        return rambanks[newaddress + (currentRAMbank * 0x2000)];
    }
    else
        return system_mem[address];
}

void memory::performDMAtransfer(uint8_t data) {
    uint16_t address = data << 8;
    for (int i = 0; i < 0xA0; i++)
    {
        write_mem(0xFE00+i, read_mem(address+i));
    }
}

void memory::LoadROM(const char *filename) {
    //open file as binary stream and move file pointer to end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    /*std::printf("Opening rom file ");
    std::printf(filename);
    std::printf(".\n");*/

    if(file.is_open())
    {
        //get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        //go back to beginning of file and fill buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        //load ROM contents into the gb's memory, starting at 0x00
        for (int i = 0; i < size; i++)
        {
            cartridge_rom[i] = buffer[i];
        }

        //free the buffer
        delete[] buffer;

        //copy the first 0x8000 bytes into RAM
        for (int i = 0; i < 0x8000; i++)
        {
            if(i >= size) system_mem[i] = 0x00;
            else
                system_mem[i] = cartridge_rom[i];
        }
    }
    else
    {
        std::cerr << "Cannot open ROM file!\n";
    }

    //detect ROM banking
    switch (system_mem[0x147])
    {
        case 1:
        case 2:
        case 3:
            mbc_type = MBC1;
            break;
        case 5:
        case 6:
            mbc_type = MBC2;
            break;
        default:
            break;

    }
}

void memory::incrementLY() {
    system_mem[0xFF44]++;
}

void memory::handlebanking(uint16_t address, uint8_t value) {
    //do RAM enable
    if (address < 0x2000)
    {
        if(mbc_type == MBC1 || mbc_type == MBC2)
        {
            rambankenable(address, value);
        }
    }

    //do ROM bank change
    else if (address >= 0x200 && address < 0x4000)
    {
        if (mbc_type == MBC1 || mbc_type == MBC2)
            changeLorombank(value);
    }

    //do ROM or RAM bank change
    else if (address >= 0x4000 && address < 0x6000)
    {
        if (mbc_type == MBC1)
        {
            if(rombanking) changeHirombank(value);

            else rambankchange(value);
        }
    }

    //change banking mode - rom or ram banking
    else if (address >= 0x6000 && address < 0x8000)
    {
        if (mbc_type == MBC1)
            changeromrammode(value);
    }
}

void memory::rambankenable(uint16_t address, uint8_t data) {
    if(mbc_type == MBC2)
    {
        if ((address & 0x10) != 0) return;
    }

    if ((data & 0xF) == 0xA) enableRAM = true;
    else if ((data & 0xF) == 0x0) enableRAM = false;
}

void memory::changeLorombank(uint8_t data) {
    if (mbc_type == MBC2)
    {
        currentROMbank = data & 0xF;
        if (currentROMbank == 0) currentROMbank++;
        return;
    }
    currentROMbank &= 0xE0; //turn off the lower 5 bits
    currentROMbank |= (data & 0x1F); //superimpose lower 5 from data
    if (currentROMbank == 0) currentROMbank++;
}

void memory::changeHirombank(uint8_t data) {
    //change upper ROM bank number bits in MBC1 mode
    currentROMbank &= 0x1F; //turn off upper 3 bits
    data &= 0xE0; //turn off lower 5 bits of data
    currentROMbank |= data;
    if (currentROMbank == 0) currentROMbank++;
}

void memory::rambankchange(uint8_t data) {
    currentRAMbank = data & 0x3;
}

void memory::changeromrammode(uint8_t data) {
    rombanking = ((data & 0x1) == 0);
    if (rombanking) currentRAMbank = 0;
    //GB can only access RAM bank 0 while ROM banking
}
