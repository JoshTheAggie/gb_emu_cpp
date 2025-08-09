//
// Created by joshua on 3/29/22.
//

#include "memory.h"

memory::memory() {
    //load boot rom
    //open file as binary stream and move file pointer to end
    std::ifstream file("mgb_boot.bin", std::ios::binary | std::ios::ate);
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
            if (mbc_type == MBC1 && MBC1bankingmode1)
                rambanks[newaddress + ((currentRAMbank_rombank_hi2 & (number_of_ram_banks-1)) * 0x2000)] = value;
            else if (mbc_type == MBC2) {
                //only the bottom 9 bits are used here
                rambanks[newaddress & 0x1FF] = value;
            }
            else
                rambanks[newaddress] = value;
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
            //we operate under the assumption that exactly one or the other is 0
            //direction buttons selected
            return ((system_mem[0xFF00] & 0xF0) | (directions & 0x0F));
        }
    }
    else if (address < 0x100 && system_mem[0xFF50] == 0x00)
    {
        return bootrom[address];
    }
    else if (address < 0x4000) {
        //ROM banking only in MBC1 mode 1
        if (mbc_type == MBC1 && MBC1bankingmode1) {
            uint8_t bankNumber = (currentRAMbank_rombank_hi2 << 5) & (number_of_rom_banks-1);
            return cartridge_rom[address + (bankNumber * 0x4000)];
        }
        else //MBC2 never banks in this region
            return cartridge_rom[address];
    }
    else if (address >= 0x4000 && address < 0x8000) {
        //reading from ROM bank
        uint16_t newaddress = address - 0x4000;
        if (mbc_type == MBC1) {
            uint8_t bankNumber = ((currentRAMbank_rombank_hi2 << 5) | currentROMbank_lo5)
                                 & (number_of_rom_banks-1);
            return cartridge_rom[newaddress + bankNumber * 0x4000];
        }
        else if (mbc_type == MBC2)
            return cartridge_rom[newaddress + currentROMbank_lo5 * 0x4000];
        else
            return cartridge_rom[newaddress];
    }
    else if (address >= 0xA000 && address < 0xC000)
    {
        //reading from RAM bank
        // currently passing the 32KB ram test, failing the 8KB (one bank) test.
        if(enableRAM) {
            uint16_t newaddress = address - 0xA000;
            if (mbc_type == MBC1 && MBC1bankingmode1)
                return rambanks[newaddress + ((currentRAMbank_rombank_hi2 & (number_of_ram_banks-1)) * 0x2000)];
            else if (mbc_type == MBC2)
                return rambanks[newaddress & 0x1FF];
            else
                return rambanks[newaddress];
        }
        else
            return 0xFF; //rambanks[address-0xA000];
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
        std::exit(EXIT_FAILURE);
    }

    //detect ROM banking
    switch (system_mem[0x147])
    {
        case 0x00: // ROM only/No MBC
            std::cout << "No MBC detected\n";
            break;
        case 0x01: // MBC1
        case 0x02: // MBC1 + RAM
        case 0x03: // MBC1 + RAM + Battery
            mbc_type = MBC1;
            std::cout << "MBC1 detected\n";
            break;
        case 0x05: // MBC2
        case 0x06: // MBC2 + Battery
            mbc_type = MBC2;
            std::cout << "MBC2 detected\n";
            break;
        case 0x08: // ROM + RAM              No licensed cart uses 8 or 9
        case 0x09: // ROM + RAM + Battery    No licensed cart uses 8 or 9
        case 0x0B: // MMM01
        case 0x0C: // MMM01 + RAM
        case 0x0D: // MMM01 + RAM + Battery
        case 0x0F: // MBC3 + TIMER + Battery
        case 0x10: // MBC3 + TIMER + RAM + Battery
        case 0x11: // MBC3
        case 0x12: // MBC3 + RAM
        case 0x13: // MBC3 + RAM + Battery
        case 0x19: // MBC5
        case 0x1A: // MBC5 + RAM
        case 0x1B: // MBC5 + RAM + Battery
        case 0x1C: // MBC5 + RUMBLE
        case 0x1D: // MBC5 + RUMBLE + RAM
        case 0x1E: // MBC5 + RUMBLE + RAM + Battery
        case 0x20: // MBC6
        case 0x22: // MBC7 + SENSOR + RUMBLE + RAM + Battery
        case 0xFC: // POCKET CAMERA
        case 0xFD: // BANDAI TAMA5
        case 0xFE: // HuC3
        case 0xFF: // HuC1 + RAM + Battery
        default:
            std::cout << "Unimplemented MBC detected. [0x0147] = " << system_mem[0x147] << std::endl;
            break;

    }

    // detect rom size
    switch (system_mem[0x148]) {
        case 0x00: number_of_rom_banks = 2; break;
        case 0x01: number_of_rom_banks = 4; break;
        case 0x02: number_of_rom_banks = 8; break;
        case 0x03: number_of_rom_banks = 16; break;
        case 0x04: number_of_rom_banks = 32; break;
        case 0x05: number_of_rom_banks = 64; break;
        case 0x06: number_of_rom_banks = 128; break;
        case 0x07: number_of_rom_banks = 256; break;
        case 0x08: number_of_rom_banks = 512; break;
        //case 0x52: number_of_rom_banks = 72; break; //note: unofficial numbers for these last three
        //case 0x53: number_of_rom_banks = 80; break; //      no cartridges or ROMS using these sizes are known
        //case 0x54: number_of_rom_banks = 96; break;
        default: number_of_rom_banks = 2; break;
    }

    // detect ram size
    switch (system_mem[0x149]) {
        case 0x00: number_of_ram_banks = 0; break;
        case 0x02: number_of_ram_banks = 1; break;
        case 0x03: number_of_ram_banks = 4; break;
        case 0x04: number_of_ram_banks = 16; break;
        case 0x05: number_of_ram_banks = 8; break;
        default: number_of_ram_banks = 0; break;
    }
}

void memory::incrementLY() {
    system_mem[0xFF44]++;
}

void memory::handlebanking(uint16_t address, uint8_t value) {
    //do RAM enable
    if (address < 0x2000)
    {
        if(mbc_type == MBC1)
        {
            ramenable(value);
        }
        if (mbc_type == MBC2)
            MBC2_ramenable_rombankswitch(address, value);
    }

    //do ROM bank change
    else if (address >= 0x2000 && address < 0x4000)
    {
        if (mbc_type == MBC1)
            changeLorombank(value);
        if (mbc_type == MBC2)
            MBC2_ramenable_rombankswitch(address, value);
    }

    //do ROM or RAM bank change
    else if (address >= 0x4000 && address < 0x6000)
    {
        if (mbc_type == MBC1)
        {
            rambankchange(value);
        }
    }

    //change banking mode - rom or ram banking
    else if (address >= 0x6000 && address < 0x8000)
    {
        if (mbc_type == MBC1)
            changeromrammode(value);
    }
}

void memory::ramenable(uint8_t data) {
    if ((data & 0xF) == 0xA) enableRAM = true;
    else enableRAM = false;
}

void memory::changeLorombank(uint8_t data) {
    currentROMbank_lo5 = (data & 0x1F);
    if (currentROMbank_lo5 == 0x00) currentROMbank_lo5 = 1;

    // truncate bits for currentTROMbank larger than
    // the number of actual rom banks
    //if (currentROMbank_lo5 > number_of_rom_banks)
    //    currentROMbank_lo5 = currentROMbank_lo5 & (number_of_rom_banks - 1);
    // this is checked on the fly
}

void memory::MBC2_ramenable_rombankswitch(uint16_t address, uint8_t data) {
    if ((address & 0x0100) == 0) // ram enable
        ramenable(data);
    else {
        //select ROM bank
        currentROMbank_lo5 = data & 0xF;
        if (currentROMbank_lo5 == 0) currentROMbank_lo5 = 1;
    }
}

void memory::changeHirombank(uint8_t data) {
    //Note: this isn't needed anymore since I'm only using the RAMbank number now
    // and concatenating when needed
    //change upper ROM bank number bits in MBC1 mode
    currentROMbank_lo5 &= 0x1F; //turn off upper 3 bits
    data &= 0xE0; //turn off lower 5 bits of data
    currentROMbank_lo5 |= data;
    if (currentROMbank_lo5 == 0) currentROMbank_lo5 = 1;
    if (mbc_type == MBC1)
        if (currentROMbank_lo5 == 0x0)
            currentROMbank_lo5 = data + 1; //MBC1 bug
    // truncate bits for larger currentTROMbank larger than
    // the number of actual rom banks
    if (mbc_type == MBC1)
        if (currentROMbank_lo5 > number_of_rom_banks)
            currentROMbank_lo5 = currentROMbank_lo5 & (number_of_rom_banks - 1);
}

void memory::rambankchange(uint8_t data) {

    currentRAMbank_rombank_hi2 = data & 0x3;
    if (mbc_type == MBC1)
        if (currentROMbank_lo5 == 0x00)
            currentROMbank_lo5 = 1; //MBC1 bug
}

void memory::changeromrammode(uint8_t data) {
    MBC1bankingmode1 = ((data & 0x1) == 1);
    //if (!MBC1bankingmode1) currentRAMbank_rombank_hi2 = 0;
    //GB can only access RAM bank 0 while ROM banking
    // is this true? pandocs doesn't say anything about it
    // yes, according to gbdev.gg8.se
    //Last line no longer needed since this is done on the fly
}

void memory::request_interrupt(uint8_t irq_num) {
    system_mem[0xFF0F] |= (0x1 << irq_num);
}
