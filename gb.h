//
// Created by joshua on 5/23/20.
//

#include <cstdint>
#include <fstream>
#include <string.h>

#ifndef GBEMUJM_GB_H
#define GBEMUJM_GB_H

const unsigned int START_ADDRESS = 0x100;
const unsigned int VIDEO_WIDTH = 160;
const unsigned int VIDEO_HEIGHT = 144;

class gb {
    uint8_t opcode;          //8-bit instructions
    uint8_t memory [0x10000]{};    //64KiB of memory
    /*
  0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
  4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
  8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
  A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
  FE00-FE9F   Sprite Attribute Table (OAM)
  FEA0-FEFF   Not Usable
  FF00-FF7F   I/O Ports
  FF80-FFFE   High RAM (HRAM)
  FFFF        Interrupt Enable Register
     */
    uint8_t A{}, F{}, B{}, C{}, D{}, E{}, H{}, L{}; //AF, BC, DE, HL
    /*
     * FLAGS
     * 7 = Zero
     * 6 = n (add/sub flag)
     * 5 = h (half-carry)
     * 4 = Carry
     * 3-0 always 0
     */
    uint16_t PC{};              //16-bit program counter
    uint16_t SP{};              //16-bit stack pointer

    bool CB_instruction;
    bool interrupts_enabled;
    bool enable_interrupts;
    bool disable_interrupts;

    //getter and setters for flags
    void set_Z(bool condition);
    void set_N(bool condition);
    void set_H(bool condition);
    void set_C(bool condition);


    //helper functions
    static uint8_t get_bits_0_2(uint8_t op){ return (op & 0x07); };
    static uint8_t get_bits_3_5(uint8_t op){ return (op & 0x38) >> 3; };
    static uint8_t get_bits_4_5(uint8_t op){ return (op & 0x30) >> 4; };
    static uint8_t get_bits_3_4(uint8_t op){ return (op & 0x18) >> 3; };
    static bool isbiton(uint8_t num, uint8_t testbyte){ return ((0x01 << num) & testbyte) != 0x0;};

    void OP_NOP(){ /*todo: insert 1 cycle delay*/ };
    void OP_HALT();
    void OP_RST(uint8_t xxx);
    //todo: organize this list
    void OP_LD(uint8_t xxx, uint8_t yyy);
    void OP_LD_imm(uint8_t xxx);
    void OP_LD_mem(uint8_t xxx);
    void OP_LDH(uint8_t xxx, uint8_t yyy);
    void OP_LD_imm16(uint8_t xx);
    void OP_STORE_SP();
    void OP_INC_r(uint8_t xxx);
    void OP_DEC_r(uint8_t xxx);
    void OP_ADD16(uint8_t xx);
    void OP_INC16(uint8_t xx);
    void OP_DEC16(uint8_t xx);
    void OP_DAA();
    void OP_CPL();
    void OP_CCF();
    void OP_SCF();
    void OP_STOP();
    void OP_rot_shift_A(uint8_t xxx);
    void OP_JR();
    void OP_JR_test(uint8_t cc);
    void OP_ADD_r(uint8_t xxx);
    void OP_ADC_r(uint8_t xxx);
    void OP_SUB_r(uint8_t xxx);
    void OP_SBC_r(uint8_t xxx);
    void OP_AND_r(uint8_t xxx);
    void OP_OR_r(uint8_t xxx);
    void OP_XOR_r(uint8_t xxx);
    void OP_CP_r(uint8_t xxx);
    void OP_LD_mem16(uint8_t xxx, uint8_t yyy);
    void OP_LD_SP_HL();
    void OP_PUSH_r(uint8_t xx);
    void OP_POP_r(uint8_t xx);
    void OP_LD_HL_SP_offset();
    void OP_ADD_SP();
    void OP_DI();
    void OP_EI();
    void OP_ADD_A_imm();
    void OP_ADC_A_imm();
    void OP_SUB_A_imm();
    void OP_SBC_A_imm();
    void OP_AND_A_imm();
    void OP_OR_A_imm();
    void OP_XOR_A_imm();
    void OP_CP_A_imm();
    void OP_JP();
    void OP_JP_HL();
    void OP_JP_test(uint8_t xx);
    void OP_CALL();
    void OP_CALL_test(uint8_t xx);
    void OP_RET();
    void OP_RET_test(uint8_t xx);
    void OP_RETI();
    //CB instructions
    void CB_RLC(uint8_t xxx);
    void CB_RL(uint8_t xxx);
    void CB_RRC(uint8_t xxx);
    void CB_RR(uint8_t xxx);
    void CB_SLA(uint8_t xxx);
    void CB_SRA(uint8_t xxx);
    void CB_SWAP(uint8_t xxx);
    void CB_SRL(uint8_t xxx);
    void CB_BIT(uint8_t bbb, uint8_t xxx);
    void CB_SET(uint8_t bbb, uint8_t xxx);
    void CB_RES(uint8_t bbb, uint8_t xxx);
    //helper functions for memory IO.
    //makes it much easier to go back and add banking
    //for now will be basic
    void write_mem(uint16_t address, uint8_t value);
    uint8_t read_mem(uint16_t address);

    //helper functions for registers
    uint8_t * decode_register(uint8_t xxx);


public:
    //graphics
    uint32_t video [VIDEO_WIDTH * VIDEO_HEIGHT]{};

    uint8_t directions{0xFF}, buttons{0xFF};

    gb();
    void LoadROM(char const *filename);
    void Cycle();
    void Joypad();
};

#endif //GBEMUJM_GB_H
