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
                    cycle_delay(1);
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
                                    OP_LDH(get_bits_0_2(opcode));
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
                                    OP_LDH(get_bits_0_2(opcode));
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
//todo: this function
}

void gb::OP_RST(uint8_t xxx) {
//todo: this function
}

void gb::OP_LD(uint8_t xxx, uint8_t yyy) {
    uint8_t delay = 1;
    uint8_t * destination, * source;
    if (xxx != 0x6)
        destination = decode_register(xxx);
    else {
        destination = &memory[HL()];
        delay = 2;
    }
    if (yyy != 0x6)
        source = decode_register(xxx);
    else {
        source = &memory[HL()];
        delay = 2;
    }
    *destination = *source;
    cycle_delay(delay);
}

void gb::OP_LD_imm(uint8_t xxx) {
    uint8_t delay = 2;
    uint8_t * destination;
    if (xxx != 0x6)
        destination = decode_register(xxx);
    else {
        destination = &memory[HL()];
        delay = 3;
    }
    uint8_t imm = memory[PC++];
    *destination = imm;
    cycle_delay(delay);
}

void gb::OP_LD_mem(uint8_t xxx) {
    uint8_t delay = 2;
    uint8_t * destination, * source;
    bool increment = false, decrement = false;
    if(xxx % 2 == 1)
    {
        //load from mem to A
        destination = &A;
        switch (xxx) {
            case 1:
                source = &memory[BC()];
                break;
            case 3:
                source = &memory[DE()];
                break;
            case 5:
                source = &memory[HL()];
                increment = true;
                break;
            case 7:
                source = &memory[HL()];
                decrement = true;
                break;
            default:
                source = nullptr;
        }
    }
    else
    {
        //load from A to mem
        source = &A;
        switch (xxx) {
            case 0:
                destination = &memory[BC()];
                break;
            case 2:
                destination = &memory[DE()];
                break;
            case 4:
                destination = &memory[HL()];
                increment = true;
                break;
            case 6:
                destination = &memory[HL()];
                decrement = true;
                break;
            default:
                destination = nullptr;
        }
    }
    *destination = *source;
    if (increment) inc_HL();
    if (decrement) dec_HL();
    cycle_delay(delay);
}

void gb::OP_LDH(uint8_t yyy) {
    uint8_t delay = 2;
    uint16_t address = 0xFF00;
    if (yyy == 2)
        address += C;
    else
    {
        address += memory[PC++];
        delay++;
    }
    if (isbiton(4, opcode))
    {
        //destination is A
        A = memory[address];
    }
    else
    {
        //destination is mem
        memory[address] = A;
    }
    cycle_delay(delay);
}

void gb::OP_LD_imm16(uint8_t xx) {
    uint16_t imm;
    imm = memory[PC++];
    imm += (memory[PC++] << 8);
    switch (xx)
    {
        case 0:
            write_BC(imm);
            break;
        case 1:
            write_DE(imm);
            break;
        case 2:
            write_HL(imm);
            break;
        case 3:
            SP = imm;
            break;
        default:
            break;
    }
    cycle_delay(3);
}

void gb::OP_STORE_SP() {
    uint16_t address = memory[PC++];
    address += (memory[PC++] << 8);
    memory[address] = (SP & 0xFF);
    memory[address+1] = (SP >> 8);
    cycle_delay(5);
}

void gb::OP_INC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 3;
        reg = &memory[HL()];
    }
    (*reg)++;
    cycle_delay(delay);
}

void gb::OP_DEC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 3;
        reg = &memory[HL()];
    }
    (*reg)--;
    cycle_delay(delay);
}

void gb::OP_ADD16(uint8_t xx) {
//todo: this function
}

void gb::OP_INC16(uint8_t xx) {
//todo: this function
}

void gb::OP_DEC16(uint8_t xx) {
//todo: this function
}

void gb::OP_DAA() {
//todo: this function
}

void gb::OP_CPL() {
//todo: this function
}

void gb::OP_CCF() {
//todo: this function
}

void gb::OP_SCF() {
//todo: this function
}

void gb::OP_STOP() {
//todo: this function
}

void gb::OP_rot_shift_A(uint8_t xxx) {
//todo: this function
}

void gb::OP_JR() {
//todo: this function
}

void gb::OP_JR_test(uint8_t cc) {
//todo: this function
}

void gb::OP_ADD_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A += *reg;
    cycle_delay(delay);
}

void gb::OP_ADC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A += *reg + get_C();
    cycle_delay(delay);
}

void gb::OP_SUB_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A -= *reg;
    cycle_delay(delay);
}

void gb::OP_SBC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A -= *reg - get_C();
    cycle_delay(delay);
}

void gb::OP_AND_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A &= *reg;
    cycle_delay(delay);
}

void gb::OP_OR_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A |= *reg;
    cycle_delay(delay);
}

void gb::OP_XOR_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    A ^= *reg;
    cycle_delay(delay);
}

void gb::OP_CP_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x6) {
        delay = 2;
        reg = &memory[HL()];
    }
    uint8_t compare_val;
    compare_val = A - *reg;
    cycle_delay(delay);
}

void gb::OP_LD_mem16(uint8_t xxx, uint8_t yyy) {
//todo: this function
}

void gb::OP_LD_SP_HL() {
    SP = HL();
    cycle_delay(2);
}

void gb::OP_PUSH_r(uint8_t xx) {
    uint16_t value;
    switch (xx) {
        case 0:
            value = BC();
            break;
        case 1:
            value = DE();
            break;
        case 2:
            value = HL();
            break;
        case 3:
            value = SP;
            break;
        default:
            break;
    }
    SP--;
    write_mem(SP, (value >> 8));
    SP--;
    write_mem(SP, (value & 0xFF));
    cycle_delay(4);
}

void gb::OP_POP_r(uint8_t xx) {
    uint16_t data;
    data = memory[SP++];
    data += (memory[SP++] << 8);
    switch (xx) {
        case 0:
            write_BC(data);
            break;
        case 1:
            write_DE(data);
            break;
        case 2:
            write_HL(data);
            break;
        case 3:
            SP = data;
            break;
        default:
            break;
    }
    cycle_delay(3);
}

void gb::OP_LD_HL_SP_offset() {
    uint16_t offset;
    offset = memory[PC++];
    if (isbiton(7, offset))
        offset += 0xFF00;
    write_HL(SP + offset);
    cycle_delay(3);
}

void gb::OP_ADD_SP() {
    uint16_t offset;
    offset = memory[PC++];
    if (isbiton(7, offset))
        offset += 0xFF00;
    SP += offset;
    cycle_delay(4);
}

void gb::OP_DI() {
    disable_interrupts = true;
}

void gb::OP_EI() {
    enable_interrupts = true;
}

void gb::OP_ADD_A_imm() {
//todo: this function
}

void gb::OP_ADC_A_imm() {
//todo: this function
}

void gb::OP_SUB_A_imm() {
//todo: this function
}

void gb::OP_SBC_A_imm() {
//todo: this function
}

void gb::OP_AND_A_imm() {
//todo: this function
}

void gb::OP_OR_A_imm() {
//todo: this function
}

void gb::OP_XOR_A_imm() {
//todo: this function
}

void gb::OP_CP_A_imm() {
//todo: this function
}

void gb::OP_JP() {
//todo: this function
}

void gb::OP_JP_HL() {
//todo: this function
}

void gb::OP_JP_test(uint8_t xx) {
//todo: this function
}

void gb::OP_CALL() {
//todo: this function
}

void gb::OP_CALL_test(uint8_t xx) {
//todo: this function
}

void gb::OP_RET() {
//todo: this function
}

void gb::OP_RET_test(uint8_t xx) {
//todo: this function
}

void gb::OP_RETI() {
//todo: this function
}

void gb::CB_RLC(uint8_t xxx) {
//todo: this function
}

void gb::CB_RL(uint8_t xxx) {
//todo: this function
}

void gb::CB_RRC(uint8_t xxx) {
//todo: this function
}

void gb::CB_RR(uint8_t xxx) {
//todo: this function
}

void gb::CB_SLA(uint8_t xxx) {
//todo: this function
}

void gb::CB_SRA(uint8_t xxx) {
//todo: this function
}

void gb::CB_SWAP(uint8_t xxx) {
//todo: this function
}

void gb::CB_SRL(uint8_t xxx) {
//todo: this function
}

void gb::CB_BIT(uint8_t bbb, uint8_t xxx) {
//todo: this function
}

void gb::CB_SET(uint8_t bbb, uint8_t xxx) {
//todo: this function
}

void gb::CB_RES(uint8_t bbb, uint8_t xxx) {
//todo: this function
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
        case 1:
            return &C;
        case 2:
            return &D;
        case 3:
            return &E;
        case 4:
            return &H;
        case 5:
            return &L;
        case 6:
            return nullptr;
        case 7:
            return  &A;
        default:
            return nullptr;
    }
}

void gb::cycle_delay(uint8_t cycles) {
    //todo: incorporate delay based on number of cycles
}

uint16_t gb::HL() const {
    return (H << 8) + L;
}

uint16_t gb::BC() const {
    return (B << 8) + C;
}

uint16_t gb::DE() const {
    return (D << 8) + E;
}

void gb::inc_HL() {
    uint16_t current_val = HL();
    current_val++;
    H = current_val >> 8;
    L = current_val & 0xFF;
}

void gb::dec_HL() {
    uint16_t current_val = HL();
    current_val--;
    H = current_val >> 8;
    L = current_val & 0xFF;
}

void gb::write_HL(uint16_t value) {
    H = value >> 8;
    L = value & 0xFF;
}

void gb::write_BC(uint16_t value) {
    B = value >> 8;
    C = value & 0xFF;
}

void gb::write_DE(uint16_t value) {
    D = value >> 8;
    E = value & 0xFF;
}

uint8_t gb::get_C() const {
    return ((F & 0x10) >> 4);
}


