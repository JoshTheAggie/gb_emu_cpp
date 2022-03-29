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
    //does nothing yet
}

void ppu::setLCDstatus() {
    uint8_t status = memory[0xFF41];
    if(!display_enabled())
    {
        //set the mode to 1 during lcd disabled, set scanline to 0
        scanline_counter = 456;
        LY = 0x00;
        status &= 0xFC; //clear bits 1,0
        status |= 0x01; //set bit 0
        memory[0xFF41] = status;
        return;
    }

    uint8_t currentline = LY;
    uint8_t currentmode = status & 0x03;

    uint8_t mode = 0;
    bool requestInt = false;

    // in vblank set mode to 1
    if (currentline >= 144)
    {
        mode = 1;
        status |= 0x01; //set bit 0
        status &= 0xFD; //reset bit 1
        requestInt = (status & 0x10) != 0; //is bit 4 on?
    }
    else
    {
        int mode2bounds = 456 - 80;
        int mode3bounds = mode2bounds - 172;

        //mode 2
        if (scanline_counter >= mode2bounds)
        {
            mode = 2;
            status |= 0x02; //set bit 1
            status &= 0xFE; //reset bit 0
            requestInt = (status & 0x20) != 0; //is bit 5 on?
        }
        //mode 3
        else if (scanline_counter >= mode3bounds)
        {
            mode = 3;
            status |= 0x03; //set bits 1,0
        }
        //mode 0
        else
        {
            mode = 0;
            status &= 0xFC; //reset bits 1,0
            requestInt = (status & 0x08) != 0; //is bit 3 on?
        }
    }

    //if changing modes request interrupt
    if(requestInt && (mode != currentmode))
    {
        memory[0xFF0F] |= 0x02; //bit 1 LCDSTAT set - interrupt
    }

    //check the coincidence flag
    if(LY == LYC)
    {
        status |= 0x04; //set bit 2
        if ((status & 0x40) != 0) //if bit 6 is on
            memory[0xFF0F] |= 0x02; //bit 1 LCDSTAT set - interrupt
    }
    else
    {
        status &= 0xFB; //reset bit 2
    }
    memory[0xFF41] = status;
}
