//
// Created by joshua on 3/26/22.
//

#include "ppu.h"

ppu::ppu(uint32_t *video) {
    display = video;
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
        sharedMemory.write_mem(0xFF44, LY+1);
        uint8_t currentline = LY;

        scanline_counter = 456; //456 CPU cycles per scanline

        //vertical blank?
        if (currentline == 144)
        {
            //bit 0 VBLANK set
            sharedMemory.write_mem(0xFF0F, (sharedMemory.read_mem(0xFF0F)|0x01));
        }
        else if (currentline > 153)
        {
            sharedMemory.write_mem(0xFF44, 0x00);
        }
        else if (currentline < 144)
            draw_scanline();
    }
}

void ppu::draw_scanline() {
    uint8_t control = sharedMemory.read_mem(0xFF40);
    if(testbit(0, control)) // is bit 0 on?
    {
        render_tiles();
    }
    if(testbit(1, control)) // is bit 1 on?
    {
        render_sprites();
    }
}

void ppu::setLCDstatus() {
    uint8_t status = sharedMemory.read_mem(0xFF41);
    if(!display_enabled())
    {
        //set the mode to 1 during lcd disabled, set scanline to 0
        scanline_counter = 456;
        sharedMemory.write_mem(0xFF44, 0x00);
        status &= 0xFC; //clear bits 1,0
        status |= 0x01; //set bit 0
        sharedMemory.write_mem(0xFF41, status);
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
        requestInt = testbit(4, status); //is bit 4 on?
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
            requestInt = testbit(5, status); //is bit 5 on?
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
            requestInt = testbit(3, status); //is bit 3 on?
        }
    }

    //if changing modes request interrupt
    if(requestInt && (mode != currentmode))
    {
        //bit 1 LCDSTAT set - interrupt
        sharedMemory.write_mem(0xFF0F, (sharedMemory.read_mem(0xFF0F)|0x02));
    }

    //check the coincidence flag
    if(LY == LYC)
    {
        status |= 0x04; //set bit 2
        if (testbit(6, status)) //if bit 6 is on
            //bit 1 LCDSTAT set - interrupt
            sharedMemory.write_mem(0xFF0F, (sharedMemory.read_mem(0xFF0F)|0x02));
    }
    else
    {
        status &= 0xFB; //reset bit 2
    }
    sharedMemory.write_mem(0xFF41, status);
}

bool ppu::testbit(uint8_t num, uint8_t data) {
    return (data & (0x01 << num)) != 0;
}

void ppu::render_tiles() {
    uint16_t tileData = 0;
    uint16_t backgroundMemory = 0;
    bool unsig = true;

    uint8_t scrollX = SCX;
    uint8_t scrollY = SCY;
    uint8_t windowX = WX;
    uint8_t windowY = WY;

    bool windowOn = false;

    //is the window enabled?
    if(window_enabled())
    {
        //is the current scanline we're drawing within window Y pos?
        if (windowY <= LY)
            windowOn = true;
    }

    //where is the tile data?
    if(bg_window_tile_data())
    {
        tileData = 0x8000;
    }
    else
    {
        //IMPORTANT: this memory region uses signed bytes as tile IDs
        tileData = 0x8800;
        unsig = false;
    }

    //where is the background memory?
    if(!windowOn)
    {
        if(bg_tile_map_address())
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }
    else
    {
        //where is the window memory?
        if(window_tile_map_address())
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }

    uint8_t ypos = 0;

    //ypos is used to calculate which of 32 vertical tiles the
    //current scanline is drawing
    if(!windowOn)
        ypos = scrollY + LY;
    else
        ypos = LY - windowY;

    //which of the 8 vertical pixels of the current tile is
    //the scanline on?
    uint16_t tileRow = (((uint8_t)(ypos/8))*32);

    //time to start drawing the 160 horizontal pixels
    //for this scanline
    for (int pixel = 0; pixel < 160; pixel++)
    {
        uint8_t xpos = pixel+scrollX;

        //translate the current xpos into window space if needed
        if(windowOn && (pixel >= windowX))
        {
            xpos = pixel - windowX;
        }

        //which of the 32 horizontal tiles does this xpos fall within?
        uint16_t tileCol = xpos/8;
        int16_t tileNum;

        //get the tile ID. remember it can be signed or unsigned
        uint16_t tileAddress = backgroundMemory + tileRow + tileCol;
        if(unsig) tileNum = (uint8_t)sharedMemory.read_mem(tileAddress);
        else tileNum = (int8_t)sharedMemory.read_mem(tileAddress);

        //where's this tile ID in memory?
        uint16_t tileLocation = tileData;
        if(unsig) tileLocation += (tileNum * 16);
        else tileLocation += ((tileNum + 128) * 16);

        //find the correct vertical line we're on of the tile to get the tile data
        uint8_t line = ypos % 8;
        line *= 2; //each line takes 2 bytes of memory
        uint8_t data1 = sharedMemory.read_mem(tileLocation + line);
        uint8_t data2 = sharedMemory.read_mem(tileLocation + line + 1);

        //pixel 0 in the tile is bit 7 of data1 and data2
        //pixel 1 is bit 6, etc...
        int colorbit = xpos % 8;
        colorbit = -1 * (colorbit - 7);

        //combine data2 and data1 to get the color id for this pixel
        int colornum = getbitvalue(colorbit, data2);
        colornum <<= 1;
        colornum |= getbitvalue(colorbit, data1);

        //we have color ID. now look up from palette 0xFF47
        COLOR col = get_color(colornum, 0xFF47);
        uint8_t red = 0x00;
        uint8_t green = 0x00;
        uint8_t blue = 0x00;

        //setup the RGB values
        switch(col)
        {
            case WHITE: red = 0xFF; green = 0xFF; blue = 0xFF; break;
            case LIGHT_GRAY: red = 0xA9; green = 0xA9; blue = 0xA9; break;
            case DARK_GRAY: red = 0x84; green = 0x84; blue = 0x84; break;
            default: break;
        }

        int finalY = LY;

        //last check to ensure we're in the bounds of the screen
        if ((finalY<0)||(finalY>143)||(pixel<0)||(pixel>159))
            continue;

        //finally write the pixel!
        uint32_t pixeldata = (red << 24) + (green << 16) + (blue << 8) + 0xFF; //RGBA for SFML
        display[finalY * 160 + pixel] = pixeldata;
    }
}

void ppu::render_sprites() {

}

uint8_t ppu::getbitvalue(uint8_t num, uint8_t data) {
    return testbit(num, data)? 1 : 0;
}

COLOR ppu::get_color(int colornum, uint16_t palette_address) {
    COLOR res = WHITE;
    uint8_t palette = sharedMemory.read_mem(palette_address);
    int hi = 0;
    int lo = 0;

    //which bits of the palette does the color id map to?
    switch(colornum)
    {
        case 0: hi = 1; lo = 0; break;
        case 1: hi = 3; lo = 2; break;
        case 2: hi = 5; lo = 4; break;
        case 3: hi = 7; lo = 6; break;
    }

    //use the palette to get the color
    int color = 0;
    color = getbitvalue(hi, palette) << 1;
    color |= getbitvalue(lo, palette);

    //convert game color to emulator color
    switch(color)
    {
        case 0: res = WHITE; break;
        case 1: res = LIGHT_GRAY; break;
        case 2: res = DARK_GRAY; break;
        case 3: res = BLACK; break;
    }
    return res;
}
