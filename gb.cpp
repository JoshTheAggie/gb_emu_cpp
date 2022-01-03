//
// Created by joshua on 5/23/20.
//

#include "gb.h"

gb::gb()
{
    //initialize PC
    PC = 0x0u;
    CB_instruction = false;

    //todo: initialize all register values that have a fixed value
    memory[0xFF00] |= 0xFF;

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
            memory[i] = buffer[i];
        }

        //free the buffer
        delete[] buffer;
    }

}

void gb::LoadROM(const char *filename)
{
    //todo: add banking capability
    //open file as binary stream and move file pointer to end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(file.is_open())
    {
        //get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        //go back to beginning of file and fill buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        //load ROM contents into the gb's memory, starting at 0x100
        for (int i = 0; i < size; i++)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        //free the buffer
        delete[] buffer;
    }
}

void gb::Cycle() {
    //fetch instruction
    opcode = memory[PC];
    //output for debug
    std::printf("pc: %x\topcode: %x\n", PC, opcode);

    //increment pc before doing anything
    PC++;

    if (CB_instruction)
    {
        switch ((opcode & 0xF0u) >> 6)
        {
            case 0: //00
                switch(get_bits_3_5(opcode))
                {
                    case 0:
                        CB_RLC(get_bits_0_2(opcode));
                        break;
                    case 1:
                        CB_RRC(get_bits_0_2(opcode));
                        break;
                    case 2:
                        CB_RL(get_bits_0_2(opcode));
                        break;
                    case 3:
                        CB_RR(get_bits_0_2(opcode));
                        break;
                    case 4:
                        CB_SLA(get_bits_0_2(opcode));
                        break;
                    case 5:
                        CB_SRA(get_bits_0_2(opcode));
                        break;
                    case 6:
                        CB_SWAP(get_bits_0_2(opcode));
                        break;
                    case 7:
                        CB_SRL(get_bits_0_2(opcode));
                        break;
                }
                break;
            case 1: //01
                CB_BIT(get_bits_3_5(opcode), get_bits_0_2(opcode));
                break;
            case 2: //10
                CB_RES(get_bits_3_5(opcode), get_bits_0_2(opcode));
                break;
            case 3: //11
                CB_SET(get_bits_3_5(opcode), get_bits_0_2(opcode));
                break;
        }
        CB_instruction = false;
    }
    else
        switch ((opcode & 0xF0u) >> 6)
        {
            case 0: //00
                switch (opcode & 0x07)
                {
                    case 0:
                        if (opcode == 0x08) OP_STORE_SP();
                        else if (opcode == 0x10) OP_STOP();
                        else if (opcode == 0x18) OP_JR();
                        else if (isbiton(5, opcode)) OP_JR_test(get_bits_3_4(opcode));
                        else OP_NOP();
                        break;
                    case 1:
                        if (isbiton(3, opcode)) OP_ADD16(get_bits_4_5(opcode));
                        else OP_LD_imm16(get_bits_4_5(opcode));
                        break;
                    case 2:
                        OP_LD_mem(get_bits_3_5(opcode));
                        break;
                    case 3:
                        if (isbiton(3, opcode)) OP_DEC16(get_bits_4_5(opcode));
                        else OP_INC16(get_bits_4_5(opcode));
                            break;
                    case 4:
                        OP_INC_r(get_bits_3_5(opcode));
                        break;
                    case 5:
                        OP_DEC_r(get_bits_3_5(opcode));
                        break;
                    case 6:
                        OP_LD_imm(get_bits_3_5(opcode));
                        break;
                    case 7:
                        if(isbiton(5, opcode))
                        {
                            if(opcode == 0x27) OP_DAA();
                            else if (opcode == 0x2F) OP_CPL();
                            else if (opcode == 0x3F) OP_CCF();
                            else if (opcode == 0x37) OP_SCF();
                        }
                        else
                            OP_rot_shift_A(get_bits_3_5(opcode));
                        break;
                }
                break;
            case 1: //01
                if(opcode == 0x76) OP_HALT();
                else OP_LD(get_bits_3_5(opcode), get_bits_0_2(opcode));
                break;
            case 2: //10
                switch (get_bits_3_5(opcode))
                {
                    case 0:
                        OP_ADD_r(get_bits_0_2(opcode));
                        break;
                    case 1:
                        OP_ADC_r(get_bits_0_2(opcode));
                        break;
                    case 2:
                        OP_SUB_r(get_bits_0_2(opcode));
                        break;
                    case 3:
                        OP_SBC_r(get_bits_0_2(opcode));
                        break;
                    case 4:
                        OP_AND_r(get_bits_0_2(opcode));
                        break;
                    case 5:
                        OP_OR_r(get_bits_0_2(opcode));
                        break;
                    case 6:
                        OP_XOR_r(get_bits_0_2(opcode));
                        break;
                    case 7:
                        OP_CP_r(get_bits_0_2(opcode));
                        break;
                }
                break;
            case 3: //11
                if (opcode == 0xCB) {
                    //todo: one cycle delay here
                    CB_instruction = true;
                }
                else
                    switch (get_bits_0_2(opcode)) {
                        case 0:
                            if (isbiton(5, opcode))
                            {
                                if (isbiton(3, opcode)) {
                                    if (isbiton(4, opcode))
                                        OP_LD_HL_SP_offset();
                                    else
                                        OP_ADD_SP();
                                }
                                else
                                    OP_LDH(get_bits_3_5(opcode), get_bits_0_2(opcode));
                            }
                            else
                                OP_RET_test(get_bits_3_4(opcode));
                            break;
                        case 1:
                            switch (get_bits_3_5(opcode)) {
                                case 1:
                                    OP_RET();
                                    break;
                                case 3:
                                    OP_RETI();
                                    break;
                                case 5:
                                    OP_JP_HL();
                                    break;
                                case 7:
                                    OP_LD_SP_HL();
                                    break;
                                case 0:
                                case 2:
                                case 4:
                                case 6:
                                    OP_POP_r(get_bits_4_5(opcode));
                            }
                            break;
                        case 2:
                            if (isbiton(5, opcode)) {
                                if (isbiton(3, opcode))
                                    OP_LD_mem16(get_bits_3_5(opcode), get_bits_0_2(opcode));
                                else
                                    OP_LDH(get_bits_3_5(opcode), get_bits_0_2(opcode));
                            }
                            else
                                OP_JP_test(get_bits_3_4(opcode));
                            break;
                        case 3:
                            if (isbiton(4, opcode))
                            {
                                if (isbiton(3, opcode))
                                    OP_EI();
                                else
                                    OP_DI();
                            }
                            else
                                OP_JP();
                            break;
                        case 4:
                            OP_CALL_test(get_bits_3_4(opcode));
                            break;
                        case 5:
                            if (isbiton(3, opcode))
                                OP_CALL();
                            else
                                OP_PUSH_r(get_bits_4_5(opcode));
                            break;
                        case 6:
                            switch(get_bits_3_5(opcode)){
                                case 0:
                                    OP_ADD_A_imm();
                                    break;
                                case 1:
                                    OP_ADC_A_imm();
                                    break;
                                case 2:
                                    OP_SUB_A_imm();
                                    break;
                                case 3:
                                    OP_SBC_A_imm();
                                    break;
                                case 4:
                                    OP_AND_A_imm();
                                    break;
                                case 5:
                                    OP_XOR_A_imm();
                                    break;
                                case 6:
                                    OP_OR_A_imm();
                                    break;
                                case 7:
                                    OP_CP_A_imm();
                                    break;
                            }
                            break;
                        case 7:
                            OP_RST(get_bits_3_5(opcode));
                            break;
                    }
                break;
        }
    //todo: double check that this is correct operation of DI and EI
    if (enable_interrupts)
    {
        interrupts_enabled = true;
        enable_interrupts = false;
    }
    if (disable_interrupts)
    {
        interrupts_enabled = false;
        disable_interrupts = false;
    }
}

void gb::Joypad() {
    if(!(memory[0xFF00] & 0b00100000)) //if bit 5 is set 0
    {
        memory[0xFF00] = (memory[0xFF00] & 0xF0) + (directions & 0x0F);
    }
    else if(!(memory[0xFF00] & 0b00010000)) //if bit 4 is set 0
    {
        memory[0xFF00] = (memory[0xFF00] & 0xF0) + (buttons & 0x0F);
    }
}

void gb::set_Z(bool condition) {
    if(condition)
        F = F | 0x80;
    else
        F = F & 0x7F;
}

void gb::set_N(bool condition) {
    if(condition)
        F = F | 0x40; //0100
    else
        F = F & 0xBF; //1011
}

void gb::set_H(bool condition) {
    if(condition)
        F = F | 0x20; //0010
    else
        F = F & 0xDF; //1101
}

void gb::set_C(bool condition) {
    if(condition)
        F = F | 0x10;
    else
        F = F & 0xEF;
}

void gb::OP_HALT() {

}

void gb::OP_RST(uint8_t xxx) {

}

void gb::OP_LD(uint8_t xxx, uint8_t yyy) {

}

void gb::OP_LD_imm(uint8_t xxx) {

}

void gb::OP_LD_mem(uint8_t xxx) {

}

void gb::OP_LDH(uint8_t xxx, uint8_t yyy) {

}

void gb::OP_LD_imm16(uint8_t xx) {

}

void gb::OP_STORE_SP() {

}

void gb::OP_INC_r(uint8_t xxx) {

}

void gb::OP_DEC_r(uint8_t xxx) {

}

void gb::OP_ADD16(uint8_t xx) {

}

void gb::OP_INC16(uint8_t xx) {

}

void gb::OP_DEC16(uint8_t xx) {

}

void gb::OP_DAA() {

}

void gb::OP_CPL() {

}

void gb::OP_CCF() {

}

void gb::OP_SCF() {

}

void gb::OP_STOP() {

}

void gb::OP_rot_shift_A(uint8_t xxx) {

}

void gb::OP_JR() {

}

void gb::OP_JR_test(uint8_t cc) {

}

void gb::OP_ADD_r(uint8_t xxx) {

}

void gb::OP_ADC_r(uint8_t xxx) {

}

void gb::OP_SUB_r(uint8_t xxx) {

}

void gb::OP_SBC_r(uint8_t xxx) {

}

void gb::OP_AND_r(uint8_t xxx) {

}

void gb::OP_OR_r(uint8_t xxx) {

}

void gb::OP_XOR_r(uint8_t xxx) {

}

void gb::OP_CP_r(uint8_t xxx) {

}

void gb::OP_LD_mem16(uint8_t xxx, uint8_t yyy) {

}

void gb::OP_LD_SP_HL() {

}

void gb::OP_PUSH_r(uint8_t xx) {

}

void gb::OP_POP_r(uint8_t xx) {

}

void gb::OP_LD_HL_SP_offset() {

}

void gb::OP_ADD_SP() {

}

void gb::OP_DI() {
    disable_interrupts = true;
}

void gb::OP_EI() {
    enable_interrupts = true;
}

void gb::OP_ADD_A_imm() {

}

void gb::OP_ADC_A_imm() {

}

void gb::OP_SUB_A_imm() {

}

void gb::OP_SBC_A_imm() {

}

void gb::OP_AND_A_imm() {

}

void gb::OP_OR_A_imm() {

}

void gb::OP_XOR_A_imm() {

}

void gb::OP_CP_A_imm() {

}

void gb::OP_JP() {

}

void gb::OP_JP_HL() {

}

void gb::OP_JP_test(uint8_t xx) {

}

void gb::OP_CALL() {

}

void gb::OP_CALL_test(uint8_t xx) {

}

void gb::OP_RET() {

}

void gb::OP_RET_test(uint8_t xx) {

}

void gb::OP_RETI() {

}

void gb::CB_RLC(uint8_t xxx) {

}

void gb::CB_RL(uint8_t xxx) {

}

void gb::CB_RRC(uint8_t xxx) {

}

void gb::CB_RR(uint8_t xxx) {

}

void gb::CB_SLA(uint8_t xxx) {

}

void gb::CB_SRA(uint8_t xxx) {

}

void gb::CB_SWAP(uint8_t xxx) {

}

void gb::CB_SRL(uint8_t xxx) {

}

void gb::CB_BIT(uint8_t bbb, uint8_t xxx) {

}

void gb::CB_SET(uint8_t bbb, uint8_t xxx) {

}

void gb::CB_RES(uint8_t bbb, uint8_t xxx) {

}

void gb::write_mem(uint16_t address, uint8_t value) {
    memory[address] = value;
}

uint8_t gb::read_mem(uint16_t address) {
    return memory[address];
}

uint8_t *gb::decode_register(uint8_t xxx) {
    switch (xxx)
    {
        case 0:
            return &B;
            break;
        case 1:
            return &C;
            break;
        case 2:
            return &D;
            break;
        case 3:
            return &E;
            break;
        case 4:
            return &H;
            break;
        case 5:
            return &L;
            break;
        case 6:
            return nullptr;
            break;
        case 7:
            return  &A;
            break;
        default:
            return nullptr;
            break;
    }
}


