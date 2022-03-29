//
// Created by joshua on 3/26/22.
//

#include "ppu.h"

ppu::ppu(uint8_t *mem, uint32_t *video) {
    memory = mem;
    display = video;
}

void ppu::check_lyc() {
    if (line_count == LYC)
    {
        //generate interrupt
        memory[0xFF0F] |= 0x02; //bit 1 LCD STAT set
    }
}

void ppu::update_graphics(uint32_t cycles) {
    setLCDstatus();

    if(display_enabled())
        scanline_counter -= cycles;
    else
        return;

    if(scanline_counter <= 0)
    {
        //time to move to next scanline
        LY++;
        uint8_t currentline = memory[0xFF44];

        scanline_counter = 456; //456 CPU cycles per scanline

        //vertical blank?
        if (currentline == 144)
        {
            memory[0xFF0F] |= 0x01; //bit 0 VBLANK set
        }
        else if (currentline > 153)
        {
            memory[0xFF44] = 0;
        }
        else if (currentline < 144)
            draw_scanline();
    }
}

void ppu::draw_scanline() {

}

void ppu::setLCDstatus() {

}
