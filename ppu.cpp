//
// Created by joshua on 3/26/22.
//

#include "ppu.h"

ppu::ppu(uint8_t *mem, uint32_t *video) {
    memory = mem;
    display = video;
    VRAM_OAM_lock = false;
}

void ppu::check_lyc() {
    if (line_count == LYC)
    {
        //generate interrupt
        memory[0xFF0F] |= 0x02; //bit 1 LCD STAT set
    }
}
