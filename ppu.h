//
// Created by joshua on 3/26/22.
//
#include <cstdint>

#ifndef GBEMUJM_PPU_H
#define GBEMUJM_PPU_H

#define SCY (memory[0xFF42])
#define SCX (memory[0xFF43])
#define LY (memory[0xFF44])
#define LYC (memory[0xFF45]) //register of Y coordinate to generate interrupt (FF41)
#define DMA (memory[0xFF46])
#define BGP (memory[0xFF47])
#define OBP0 (memory[0xFF48])
#define OBP1 (memory[0xFF49])
#define WY (memory[0xFF4A])
#define WX (memory[0xFF4B])

class ppu {
    //is oam memory locked??
    bool VRAM_OAM_lock; //todo: implement this later

    //need a pointer to the memory of the SOC
    uint8_t *memory;
    //need a pointer to the display buffer
    uint32_t *display;
    uint16_t const vram_start = 0x8000;
    uint16_t const oam_start = 0xFE00;
    // OAM memory last entry is 0xFE9C, incrementing by 4 bytes each. 40 entries total
    // Ypos, Xpos, Tile number, Flags

    // 144 lines. in each:
    //   20 clocks OAM search
    //   43 clocks pixel transfer
    //   51 clocks H-blank
    // 10 lines of V-blank

    // 114x154 = 17556 clocks per screen. At 1MHz, that's 59.7 Hz refresh

    ppu(uint8_t* mem, uint32_t* video);

    void oam_search();
    void draw_line();
    void h_blank();
    void check_lyc();

    void write_from_CPU(uint16_t address, uint8_t data); //if locked, silently fail
    uint8_t read_to_CPU(uint16_t address); //if locked, return FF

private:
    int line_count = 0;

    //LCD Control register
    bool display_enabled(){return (memory[0xFF40] & 0x80) != 0;}; //lighter than white
    bool window_tile_map_address(){return (memory[0xFF40] & 0x40) != 0;}; //0 for 9800, 1 for 9C00
    bool window_enabled(){return (memory[0xFF40] & 0x20) != 0;}; //1=on
    bool bg_window_tile_data(){return (memory[0xFF40] & 0x10) != 0;}; //0=8800, 1=8000
    bool bg_tile_map_address(){return (memory[0xFF40] & 0x08) != 0;}; //0=9800, 1=9C00
    bool obj_size(){return (memory[0xFF40] & 0x04) != 0;}; //0=8x8, 1=8x16
    bool obj_enable(){return (memory[0xFF40] & 0x02) != 0;}; //sprites enabled?
    bool bg_enable(){return (memory[0xFF40] & 0x01) != 0;}; //background (and window?) enabled?
    //LCD STATUS register
    // the interrupt bits determine what can cause an LCD STAT interrupt
    bool LY_interrupt(){return (memory[0xFF41] & 0x40) != 0;}; //LYC interrupt source
    bool mode_2_oam_interrupt(){return (memory[0xFF41] & 0x20) != 0;}; //OAM interrupt source
    bool mode_1_vblank_interrupt(){return (memory[0xFF41] & 0x10) != 0;}; //VB interrupt source
    bool mode_0_hblank_interrupt(){return (memory[0xFF41] & 0x08) != 0;}; //HB interrupt source
    bool LY_flag(){return (memory[0xFF41] & 0x04) != 0;}; //is LY=LYC?
    uint8_t Mode(){return memory[0xFF41] & 0x03;}; //where in the process are we?
    // 0: H blank
    // 1: V blank
    // 2: Searching OAM
    // 3: transferring data to LCD controller


};


#endif //GBEMUJM_PPU_H
