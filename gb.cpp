//
// Created by joshua on 5/23/20.
//

#include "gb.h"

gb::gb()
{
    //initialize PC
    PC = 0x100u;
    CB_instruction = false;
    IME = false;
    enable_interrupts = false;
    disable_interrupts = false;
    opcode = 0;
    cyclecount = 0;

    //instantiate PPU
    gpu = new ppu(reinterpret_cast<uint32_t *>(&video));

    //pandocs says these are the MGB initial values for the cpu regs
    A = 0xFF;
    B = 0x00;
    C = 0x13;
    D = 0x00;
    E = 0xD8;
    F = 0x00;
    set_Z(true);
    //set_N(false); //unneeded
    //set_H(false); //H and C are affected by cartridge checksum. init to 0?
    //set_C(false);
    H = 0x01;
    L = 0x4D;
    SP = 0xFFFE;
    //PC = 0x0100; //according to pandocs? I think they skip the bios though
    sharedMemory.write_mem(0xFF00, 0xFF); //P1 register - controller
    sharedMemory.write_mem(0xFF01, 0x00);
    sharedMemory.write_mem(0xFF02, 0x7E);
    sharedMemory.write_mem(0xFF04, 0xAB);
    sharedMemory.write_mem(0xFF05, 0x00);
    sharedMemory.write_mem(0xFF06, 0x00);
    sharedMemory.write_mem(0xFF07, 0xF8);
    sharedMemory.write_mem(0xFF0F, 0xE1); //IF register - interrupts
    sharedMemory.write_mem(0xFF10, 0x80);
    sharedMemory.write_mem(0xFF11, 0xBF);
    sharedMemory.write_mem(0xFF12, 0xF3);
    sharedMemory.write_mem(0xFF13, 0xFF);
    sharedMemory.write_mem(0xFF14, 0xBF);
    sharedMemory.write_mem(0xFF16, 0x3F);
    sharedMemory.write_mem(0xFF17, 0x00);
    sharedMemory.write_mem(0xFF18, 0xFF);
    sharedMemory.write_mem(0xFF19, 0xBF);
    sharedMemory.write_mem(0xFF1A, 0x7F);
    sharedMemory.write_mem(0xFF1B, 0xFF);
    sharedMemory.write_mem(0xFF1C, 0x9F);
    sharedMemory.write_mem(0xFF1D, 0xFF);
    sharedMemory.write_mem(0xFF1E, 0xBF);
    sharedMemory.write_mem(0xFF20, 0xFF);
    sharedMemory.write_mem(0xFF21, 0x00);
    sharedMemory.write_mem(0xFF22, 0x00);
    sharedMemory.write_mem(0xFF23, 0xBF);
    sharedMemory.write_mem(0xFF24, 0x77);
    sharedMemory.write_mem(0xFF25, 0xF3);
    sharedMemory.write_mem(0xFF26, 0xF1);
    sharedMemory.write_mem(0xFF40, 0x93);
    sharedMemory.write_mem(0xFF41, 0x85);
    sharedMemory.write_mem(0xFF42, 0x00);
    sharedMemory.write_mem(0xFF43, 0x00);
    sharedMemory.write_mem(0xFF44, 0x00);
    sharedMemory.write_mem(0xFF45, 0x00);
    sharedMemory.write_mem(0xFF46, 0xFF);
    sharedMemory.write_mem(0xFF47, 0xFC);
    sharedMemory.write_mem(0xFF48, 0xFF);
    sharedMemory.write_mem(0xFF49, 0xFF);
    sharedMemory.write_mem(0xFF4A, 0x00);
    sharedMemory.write_mem(0xFF4B, 0x00);
    sharedMemory.write_mem(0xFF4D, 0xFF);
    sharedMemory.write_mem(0xFF4F, 0xFF);
    sharedMemory.write_mem(0xFF50, 0x01); // if nonzero, bootrom is disabled
    sharedMemory.write_mem(0xFF51, 0xFF);
    sharedMemory.write_mem(0xFF52, 0xFF);
    sharedMemory.write_mem(0xFF53, 0xFF);
    sharedMemory.write_mem(0xFF54, 0xFF);
    sharedMemory.write_mem(0xFF55, 0xFF);
    sharedMemory.write_mem(0xFF56, 0xFF);
    sharedMemory.write_mem(0xFF68, 0xFF);
    sharedMemory.write_mem(0xFF69, 0xFF);
    sharedMemory.write_mem(0xFF6A, 0xFF);
    sharedMemory.write_mem(0xFF6B, 0xFF);
    sharedMemory.write_mem(0xFF70, 0xFF);
    sharedMemory.write_mem(0xFFFF, 0x00); //IE register - interrupts

    timer_counter = 1024;
    divider_counter = 0;
}


bool gb::any_interrupts() {
    //checks for and handles interrupts
    if(!IME) return false;
    else{
        uint8_t interrupts = sharedMemory.read_mem(0xFFFF) & sharedMemory.read_mem(0xFF0F);
        if(interrupts != 0){
            IME = false; //disable further interrupts
            uint16_t address;
            if(isbiton(0, interrupts)) address = 0x0040; //Vblank
            else if(isbiton(1, interrupts)) address = 0x0048; //LCD STAT
            else if(isbiton(2, interrupts)) address = 0x0050; //Timer
            else if(isbiton(3, interrupts)) address = 0x0058; //Serial
            else if(isbiton(4, interrupts)) address = 0x0060; //Joypad
            // jump to interrupt handler
            SP--;
            write_mem(SP--, (PC >> 8));
            write_mem(SP, (PC & 0xFF));
            PC = address;
            //cycle_delay(5); //pandocs speculates this is correct
            return true;
        }
        else return false;
    }

}

void gb::request_interrupt(uint8_t irq_num) {
    sharedMemory.write_mem(0xFF0F,
                           (sharedMemory.read_mem(0xFF0F) | (0x01 << irq_num)));
}

void gb::CPU_execute_op() {
    cyclecount = 0;
    //TODO: ensure interrupt handler works
    if(!any_interrupts()) {
        //fetch instruction
        opcode = sharedMemory.read_mem(PC);
        //output for debug
#ifdef DEBUG
        std::printf("pc:  %x\topcode:  %x\t\t", PC, opcode);
        while(PC == 0xCB44);
        //opscompleted++;
#endif
        //increment pc before doing anything
        PC++;

        if (CB_instruction) {
            cyclecount += 1;
            //cycle_delay(1);
            switch ((opcode & 0xF0u) >> 6) {
                case 0: //00
                    switch (get_bits_3_5(opcode)) {
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
        } else
            switch ((opcode & 0xF0u) >> 6) {
                case 0: //00
                    switch (opcode & 0x07) {
                        case 0:
                            if (opcode == 0x08) OP_STORE_SP();
                            else if (opcode == 0x10) OP_STOP();
                            else if (opcode == 0x18) OP_JR();
                            else if (isbiton(5, opcode)) OP_JR_test(get_bits_3_4(opcode));
                            else {
#ifdef DEBUG
                                printf("NOP\n");
#endif
                                OP_NOP();
                            }
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
                            if (isbiton(5, opcode)) {
                                if (opcode == 0x27) OP_DAA();
                                else if (opcode == 0x2F) OP_CPL();
                                else if (opcode == 0x3F) OP_CCF();
                                else if (opcode == 0x37) OP_SCF();
                            } else
                                OP_rot_shift_A(get_bits_3_5(opcode));
                            break;
                    }
                    break;
                case 1: //01
                    if (opcode == 0x76) OP_HALT();
                    else OP_LD(get_bits_3_5(opcode), get_bits_0_2(opcode));
                    break;
                case 2: //10
                    switch (get_bits_3_5(opcode)) {
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
                            OP_XOR_r(get_bits_0_2(opcode));
                            break;
                        case 6:
                            OP_OR_r(get_bits_0_2(opcode));
                            break;
                        case 7:
                            OP_CP_r(get_bits_0_2(opcode));
                            break;
                    }
                    break;
                case 3: //11
                    if (opcode == 0xCB) {
#ifdef DEBUG
                        std::printf("\n");
#endif
                        cyclecount++;
                        CB_instruction = true;
                    } else
                        switch (get_bits_0_2(opcode)) {
                            case 0:
                                if (isbiton(5, opcode)) {
                                    if (isbiton(3, opcode)) {
                                        if (isbiton(4, opcode))
                                            OP_LD_HL_SP_offset();
                                        else
                                            OP_ADD_SP();
                                    } else
                                        OP_LDH(get_bits_0_2(opcode));
                                } else
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
                                        break;
                                }
                                break;
                            case 2:
                                if (isbiton(5, opcode)) {
                                    if (isbiton(3, opcode))
                                        OP_LD_mem16();
                                    else
                                        OP_LDH(get_bits_0_2(opcode));
                                } else
                                    OP_JP_test(get_bits_3_4(opcode));
                                break;
                            case 3:
                                if (isbiton(4, opcode)) {
                                    if (isbiton(3, opcode))
                                        OP_EI();
                                    else
                                        OP_DI();
                                } else
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
                                switch (get_bits_3_5(opcode)) {
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
        if (enable_interrupts) {
            IME = true;
            enable_interrupts = false;
        }
        if (disable_interrupts) {
            IME = false;
            disable_interrupts = false;
        }
    }
#ifdef DEBUG
    opscompleted++;
#endif
}

void gb::update_joypad_reg() {
    if(!(sharedMemory.read_mem(0xFF00) & 0b00100000)) //if bit 5 is set 0
    {
        write_mem(0xFF00, ((sharedMemory.read_mem(0xFF00) & 0xF0) + (sharedMemory.directions & 0x0F)));
    }
    else if(!(sharedMemory.read_mem(0xFF00) & 0b00010000)) //if bit 4 is set 0
    {
        write_mem(0xFF00, ((sharedMemory.read_mem(0xFF00) & 0xF0) + (sharedMemory.buttons & 0x0F)));
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
#ifdef DEBUG
    printf("HALT\n");
#endif
    //HALT: power down CPU until an interrupt
    if ((sharedMemory.read_mem(0xFF0F) & sharedMemory.read_mem(0xFFFF)) == 0)
        PC--;
}

void gb::OP_STOP() {
    //todo: handle LCD once implemented
#ifdef DEBUG
    printf("STOP\n");
#endif
    //STOP: halt CPU and LCD until button press
    while (isbiton(4, sharedMemory.read_mem(0xFF0F))) ;
}

void gb::OP_RST(uint8_t xxx) {
    uint8_t reset = 0;
    switch (xxx) {
        case 0:
            reset = 0x00;
            break;
        case 1:
            reset = 0x08;
            break;
        case 2:
            reset = 0x10;
            break;
        case 3:
            reset = 0x18;
            break;
        case 4:
            reset = 0x20;
            break;
        case 5:
            reset = 0x28;
            break;
        case 6:
            reset = 0x30;
            break;
        case 7:
            reset = 0x38;
            break;
        default:
            break;
    }
#ifdef DEBUG
    printf("RST %X\n", reset);
#endif
    PC = reset;
    cyclecount += 8;
    //cycle_delay(8);
}

void gb::OP_LD(uint8_t xxx, uint8_t yyy) {
    uint8_t delay = 1;
    uint8_t * destination, * source;
    uint8_t val;

    if (yyy != 0x6) {
        source = decode_register(yyy);
        val = *source;
    }
    else {
        val = sharedMemory.read_mem(HL());
        delay = 2;
    }
    if (xxx != 0x6) {
        destination = decode_register(xxx);
        *destination = val;
    }
    else {
        sharedMemory.write_mem(HL(), val);
        delay = 2;
    }
#ifdef DEBUG
    printf("LD reg reg\n");
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_LD_imm(uint8_t xxx) {
    uint8_t delay = 2;
    uint8_t * destination;

    uint8_t imm = sharedMemory.read_mem(PC++);

    if (xxx != 0x6) {
        destination = decode_register(xxx);
        *destination = imm;
    }
    else {
        sharedMemory.write_mem(HL(), imm);
        delay = 3;
    }
#ifdef DEBUG
    printf("LD imm\n");
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_LD_mem(uint8_t xxx) {
    uint8_t delay = 2;
    uint8_t * destination, * source;
    uint8_t val;
    bool increment = false, decrement = false;
    if(xxx % 2 == 1)
    {
        //load from mem to A
        switch (xxx) {
            case 1:
                val = sharedMemory.read_mem(BC());
#ifdef DEBUG
                printf("LD A, (BC)\n");
#endif
                break;
            case 3:
                val = sharedMemory.read_mem(DE());
#ifdef DEBUG
                printf("LD A, (DE)\n");
#endif
                break;
            case 5:
                val = sharedMemory.read_mem(HL());
#ifdef DEBUG
                printf("LD A, (HL+)\n");
#endif
                increment = true;
                break;
            case 7:
                val = sharedMemory.read_mem(HL());
#ifdef DEBUG
                printf("LD A, (HL-)\n");
#endif
                decrement = true;
                break;
            default:
                source = nullptr;
        }
        A = val;
    }
    else
    {
        //load from A to mem
        switch (xxx) {
            case 0:
                sharedMemory.write_mem(BC(), A);
#ifdef DEBUG
                printf("LD (BC), A\n");
#endif
                break;
            case 2:
                sharedMemory.write_mem(DE(), A);
#ifdef DEBUG
                printf("LD (DE), A\n");
#endif
                break;
            case 4:
                sharedMemory.write_mem(HL(), A);
#ifdef DEBUG
                printf("LD (HL+), A\n");
#endif
                increment = true;
                break;
            case 6:
                sharedMemory.write_mem(HL(), A);
#ifdef DEBUG
                printf("LD (HL-), A\n");
#endif
                decrement = true;
                break;
            default:
                break;
        }
    }
    if (increment) inc_HL();
    if (decrement) dec_HL();
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_LDH(uint8_t yyy) {
    uint8_t delay = 2;
    uint16_t address = 0xFF00;
    if (yyy == 2) {
#ifdef DEBUG
        printf("LDH (C) ");
#endif
        address += C;
    }
    else
    {
#ifdef DEBUG
        printf("LDH (a8) ");
#endif
        address += sharedMemory.read_mem(PC++);
        delay++;
    }
    if (isbiton(4, opcode))
    {
        //destination is A
#ifdef DEBUG
        printf("to A\n");
#endif
        A = sharedMemory.read_mem(address);
    }
    else
    {
        //destination is mem
#ifdef DEBUG
        printf("from A\n");
#endif
        write_mem(address, A);
    }
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_LD_imm16(uint8_t xx) {
    uint16_t imm;
    imm = sharedMemory.read_mem(PC++);
    imm += (sharedMemory.read_mem(PC++) << 8);
    switch (xx)
    {
        case 0:
            write_BC(imm);
#ifdef DEBUG
            printf("LD BC, $%X\n", imm);
#endif
            break;
        case 1:
            write_DE(imm);
#ifdef DEBUG
            printf("LD DE, $%X\n", imm);
#endif
            break;
        case 2:
            write_HL(imm);
#ifdef DEBUG
            printf("LD HL, $%X\n", imm);
#endif
            break;
        case 3:
            SP = imm;
#ifdef DEBUG
            printf("LD SP, $%X\n", imm);
#endif
            break;
        default:
            break;
    }
    cyclecount += 3;
    //cycle_delay(3);
}

void gb::OP_STORE_SP() {
    uint16_t address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    write_mem(address, (SP & 0xFF));
    write_mem(address+1, (SP >> 8));
#ifdef DEBUG
    printf("LD ($%X) SP)\n", address);
#endif
    cyclecount += 5;
    //cycle_delay(5);
}

void gb::OP_INC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t temp;
    uint8_t val;
    if (xxx == 0x6) {
        delay = 3;
        val = sharedMemory.read_mem(HL());
        temp = val;
        val++;
        sharedMemory.write_mem(HL(), val);
        set_Z(val == 0);
    }
    else {
        val = *reg;
        temp = val;
        (*reg)++;
        set_Z(*reg == 0);
    }
#ifdef DEBUG
    printf("INC r\n");
#endif
    set_N(false);
    set_H(((temp & 0xF) + 1) > 0xF);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_DEC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t temp;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
        delay = 3;
        val = sharedMemory.read_mem(HL());
        temp = val;
        val--;
        sharedMemory.write_mem(HL(), val);
        set_Z(val == 0);
    }
    else {
        val = *reg;
        temp = val;
        (*reg)--;
        set_Z(*reg == 0);
    }
#ifdef DEBUG
    printf("DEC r\n");
#endif
    set_N(true);
    set_H(((temp & 0xF)) == 0x00);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_ADD16(uint8_t xx) {
    //ADD HL, n
    set_N(false);
    switch (xx) {
        case 0:
            set_H(((HL() & 0x0FFF) + (BC() & 0x0FFF)) > 0x0FFF);
            set_C(((uint32_t)HL() + (uint32_t)BC()) > 0x0000FFFF);
            write_HL(HL() + BC());
            break;
        case 1:
            set_H(((HL() & 0x0FFF) + (DE() & 0x0FFF)) > 0x0FFF);
            set_C(((uint32_t)HL() + (uint32_t)DE()) > 0x0000FFFF);
            write_HL(HL() + DE());
            break;
        case 2:
            set_H(((HL() & 0x0FFF) + (HL() & 0x0FFF)) > 0x0FFF);
            set_C(((uint32_t)HL() + (uint32_t)HL()) > 0x0000FFFF);
            write_HL(HL() + HL());
            break;
        case 3:
            set_H(((HL() & 0x0FFF) + (SP & 0x0FFF)) > 0x0FFF);
            set_C(((uint32_t)HL() + (uint32_t)SP) > 0x0000FFFF);
            write_HL(HL() + SP);
            break;
        default:
            break;
    }
#ifdef DEBUG
    printf("ADD HL, rr\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_INC16(uint8_t xx) {
    //INC nn
    switch (xx) {
        case 0:
            write_BC(BC() + 1);
            break;
        case 1:
            write_DE(DE() + 1);
            break;
        case 2:
            write_HL(HL() + 1);
            break;
        case 3:
            SP++;
            break;
        default:
            break;
    }
#ifdef DEBUG
    printf("INC rr\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_DEC16(uint8_t xx) {
    //DEC nn
    switch (xx) {
        case 0:
            write_BC(BC() - 1);
            break;
        case 1:
            write_DE(DE() - 1);
            break;
        case 2:
            write_HL(HL() - 1);
            break;
        case 3:
            SP--;
            break;
        default:
            break;
    }
#ifdef DEBUG
    printf("DEC rr\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_DAA() {
    //DAA decimal adjust A (BCD format)
    //uint8_t ones = A;
    //uint8_t tens = A/10;
    //ones %= 10;
    //A = (tens << 4) + ones;

    if(get_N()){
        if(get_C())
            A -= 0x60;
        if(get_H())
            A -= 0x6;
    }
    else{
        if(get_C() || A > 0x99){
            A += 0x60;
            set_C(true);
        }
        if(get_H() || (A & 0xF) > 0x9)
            A += 0x6;
    }

    set_Z(A == 0);
    set_H(false);
    //set_C(tens > 9);
#ifdef DEBUG
    printf("DAA\n");
#endif
    cyclecount += 1;
    //cycle_delay(1);
}

void gb::OP_CPL() {
    A = ~A;
    //cycle_delay(1);
    cyclecount += 1;
#ifdef DEBUG
    printf("CPL\n");
#endif
    set_N(true);
    set_H(true);
}

void gb::OP_CCF() {
    //complement carry flag
    if (get_C() == 1)
        set_C(false);
    else
        set_C(true);
    set_H(false);
    set_N(false);
#ifdef DEBUG
    printf("CCF\n");
#endif
    cyclecount += 1;
    //cycle_delay(1);
}

void gb::OP_SCF() {
    //set carry flag
    set_C(true);
    set_H(false);
    set_N(false);
#ifdef DEBUG
    printf("SCF r\n");
#endif
    cyclecount += 1;
    //cycle_delay(1);
}

void gb::OP_rot_shift_A(uint8_t xxx) {
    // 000 RLCA, 001 RRCA, 010 RLA, 011 RRA
    uint16_t newA = 0;
    switch (xxx) {
        case 0:
            //RLCA
#ifdef DEBUG
            printf("RLCA\n");
#endif
            newA = (A << 1) + (A >> 7);
            set_C(isbiton(7, A));
            break;
        case 1:
            //RRCA
#ifdef DEBUG
            printf("RRCA\n");
#endif
            newA = (A >> 1) + (A << 7);
            set_C(isbiton(0, A));
            break;
        case 2:
            //RLA
#ifdef DEBUG
            printf("RLA\n");
#endif
            newA = A << 1;
            newA += get_C();
            set_C((newA >> 8) == 1);
            break;
        case 3:
            //RRA
#ifdef DEBUG
            printf("RRA\n");
#endif
            newA = A >> 1;
            newA += (get_C() << 7);
            set_C(isbiton(0, A));
            break;
        default:
            break;
    }
    A = newA;
    set_H(false);
    set_Z(false);
    set_N(false);
    cyclecount += 1;
    //cycle_delay(1);
}

void gb::OP_ADD_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
        delay = 2;
        val = sharedMemory.read_mem(HL());
#ifdef DEBUG
        printf("ADD (HL)\n");
#endif
    }
    else {
#ifdef DEBUG
        printf("ADD r\n");
#endif
        val = *reg;
    }
    set_N(false);
    set_H(((A & 0xF) + (val & 0xF)) > 0x0F);
    set_C(((uint16_t)A + (uint16_t)(val)) > 0xFF);
    A += val;
    set_Z(A == 0x00);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_ADC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    uint8_t tempC = get_C();
    if (xxx == 0x6) {
#ifdef DEBUG
        printf("ADC (HL)\n");
#endif
        delay = 2;
        val = sharedMemory.read_mem(HL());
    }
    else {
#ifdef DEBUG
        printf("ADC r\n");
#endif
        val = *reg;
    }
    set_N(false);
    set_H(((A & 0xF) + (val & 0xF) + tempC) > 0xF);
    set_C(((uint16_t)A + (uint16_t)val + tempC) > 0xFF);
    A += (val + tempC);
    set_Z(A == 0x00);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_SUB_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
        delay = 2;
        val = sharedMemory.read_mem(HL());
#ifdef DEBUG
        printf("SUB (HL)\n");
#endif
    }
    else {
#ifdef DEBUG
        printf("SUB r\n");
#endif
        val = *reg;
    }
    set_N(true);
    set_H((A & 0xF) < (val & 0xF));
    set_C(A < val);
    A -= val;
    set_Z(A == 0x00);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_SBC_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    uint8_t tempC = get_C();
    if (xxx == 0x6) {
        delay = 2;
        val = sharedMemory.read_mem(HL());
#ifdef DEBUG
        printf("SBC (HL)\n");
#endif
    }
    else {
#ifdef DEBUG
        printf("SBC r\n");
#endif
        val = *reg;
    }
    set_N(true);
    set_H((A & 0xF) < ((val & 0xF) + tempC));
    set_C(A < (val + tempC));
    A -= (val + tempC);
    set_Z(A == 0x00);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_AND_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
#ifdef DEBUG
        printf("AND (HL)\n");
#endif
        delay = 2;
        val = sharedMemory.read_mem(HL());
    }
    else{
#ifdef DEBUG
        printf("AND r\n");
#endif
        val = *reg;
}
    A &= val;
    set_Z(A==0x00);
    set_N(false);
    set_H(true);
    set_C(false);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_OR_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
#ifdef DEBUG
        printf("OR (HL)\n");
#endif
        delay = 2;
        val = sharedMemory.read_mem(HL());
    }
    else{
#ifdef DEBUG
        printf("OR r\n");
#endif
        val = *reg;
    }
    A |= val;
    set_Z(A==0x00);
    set_N(false);
    set_H(false);
    set_C(false);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_XOR_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
#ifdef DEBUG
        printf("XOR (HL)\n");
#endif
        delay = 2;
        val = sharedMemory.read_mem(HL());
    }
    else{
#ifdef DEBUG
        printf("XOR r\n");
#endif
        val = *reg;
    }
    A ^= val;
    set_Z(A==0x00);
    set_N(false);
    set_H(false);
    set_C(false);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_CP_r(uint8_t xxx) {
    uint8_t delay = 1;
    uint8_t * reg = decode_register(xxx);
    uint8_t val;
    if (xxx == 0x6) {
#ifdef DEBUG
        printf("CP (HL)\n");
#endif
        delay = 2;
        val = sharedMemory.read_mem(HL());
    }
    else{
#ifdef DEBUG
        printf("CP r\n");
#endif
        val = *reg;
    }
    uint8_t compare_val;
    compare_val = A - val;
    set_Z(compare_val==0x00);
    set_N(true);
    set_H((A & 0xF) < (val & 0xF));
    set_C(A < val);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_LD_mem16() {
    uint16_t address;
    address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    if (isbiton(4, opcode)){
#ifdef DEBUG
        printf("LD A (a16)\n");
#endif
        //destination is A
        A = sharedMemory.read_mem(address);
    }
    else
    {
#ifdef DEBUG
        printf("LD (a16) A\n");
#endif
        //destination is mem
        write_mem(address, A);
    }
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::OP_LD_SP_HL() {
    SP = HL();
#ifdef DEBUG
    printf("LD SP HL\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_PUSH_r(uint8_t xx) {
    uint16_t value;
    switch (xx) {
        case 0:
            value = BC();
#ifdef DEBUG
            printf("PUSH BC\n");
#endif
            break;
        case 1:
            value = DE();
#ifdef DEBUG
            printf("PUSH DE\n");
#endif
            break;
        case 2:
            value = HL();
#ifdef DEBUG
            printf("PUSH HL\n");
#endif
            break;
        case 3:
            value = AF();
#ifdef DEBUG
            printf("PUSH AF\n");
#endif
            break;
        default:
            break;
    }
    SP--;
    write_mem(SP, (value >> 8));
    SP--;
    write_mem(SP, (value & 0xFF));
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::OP_POP_r(uint8_t xx) {
    uint16_t data;
    data = sharedMemory.read_mem(SP++);
    data += (sharedMemory.read_mem(SP++) << 8);
    switch (xx) {
        case 0:
            write_BC(data);
#ifdef DEBUG
            printf("POP BC\n");
#endif
            break;
        case 1:
            write_DE(data);
#ifdef DEBUG
            printf("POP DE\n");
#endif
            break;
        case 2:
            write_HL(data);
#ifdef DEBUG
            printf("POP HL\n");
#endif
            break;
        case 3:
            write_AF(data & 0xFFF0);
#ifdef DEBUG
            printf("POP AF\n");
#endif
            break;
        default:
            break;
    }
    cyclecount += 3;
    //cycle_delay(3);
}

void gb::OP_LD_HL_SP_offset() {
    uint16_t offset;
    offset = sharedMemory.read_mem(PC++);
    if (isbiton(7, offset))
        offset += 0xFF00;
    write_HL(SP + offset);
#ifdef DEBUG
    printf("LD HL SP+s8\n");
#endif
    set_Z(false);
    set_N(false);
    //set_H(((uint32_t)offset & 0x0FFF) + ((uint32_t)SP & 0x0FFF) > 0x0FFF);
    //set_C(((uint32_t)offset + (uint32_t)SP) > 0xFFFF);
    set_H(((offset & 0x000F) + (SP & 0x000F)) > 0x000F);
    set_C(((offset & 0x00FF) + (SP & 0x00FF)) > 0x00FF);
    cyclecount += 3;
    //cycle_delay(3);
}

void gb::OP_ADD_SP() {
    uint16_t offset;
    offset = sharedMemory.read_mem(PC++);
    if (isbiton(7, offset))
        offset += 0xFF00;
#ifdef DEBUG
    printf("ADD SP $%X\n", offset);
#endif
    set_Z(false);
    set_N(false);
    set_H(((offset & 0x000F) + (SP & 0x000F)) > 0x000F);
    set_C(((offset & 0x00FF) + (SP & 0x00FF)) > 0x00FF);
    SP += offset;
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::OP_DI() {
    disable_interrupts = true;
    cyclecount += 1;
#ifdef DEBUG
    printf("DI\n");
#endif
    //cycle_delay(1);
}

void gb::OP_EI() {
    enable_interrupts = true;
    cyclecount += 1;
#ifdef DEBUG
    printf("EI\n");
#endif
    //cycle_delay(1);
}

void gb::OP_ADD_A_imm() {
    uint8_t temp = sharedMemory.read_mem(PC++);
    set_N(false);
    set_H(((A & 0xF) + (temp & 0xF)) > 0x0F);
    set_C(((uint16_t)A + (uint16_t)(temp)) > 0xFF);
    A += temp;
#ifdef DEBUG
    printf("ADD A, 0x%X\n", temp);
#endif
    set_Z(A == 0x00);
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_ADC_A_imm() {
    uint8_t temp = sharedMemory.read_mem(PC++);
    uint8_t tempC = get_C();
    set_N(false);
    set_H(((A & 0xF) + (temp & 0xF) + tempC) > 0xF);
    set_C(((uint16_t)A + (uint16_t)(temp) + tempC) > 0xFF);
    A += (temp + tempC);
#ifdef DEBUG
    printf("ADC A, 0x%X\n", temp);
#endif
    set_Z(A == 0x00);
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_SUB_A_imm() {
    uint8_t temp = sharedMemory.read_mem(PC++);
    set_N(true);
    set_H((A & 0xF) < (temp & 0xF));
    set_C((A < temp));
    A -= temp;
#ifdef DEBUG
    printf("SUB A, 0x%X\n", temp);
#endif
    set_Z(A == 0x00);
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_SBC_A_imm() {
    uint8_t temp = sharedMemory.read_mem(PC++);
    uint8_t tempC = get_C();
    set_N(true);
    set_H((A & 0xF) < ((temp & 0xF) + tempC));
    set_C((A < (temp + tempC)));
    A -= (temp + tempC);
#ifdef DEBUG
    printf("SBC A, 0x%X\n", temp);
#endif
    set_Z(A == 0x00);
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_AND_A_imm() {
    A &= sharedMemory.read_mem(PC++);
    set_Z(A == 0x00);
    set_N(false);
    set_H(true);
    set_C(false);
#ifdef DEBUG
    printf("AND A, imm\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_OR_A_imm() {
    A |= sharedMemory.read_mem(PC++);
    set_Z(A == 0x00);
    set_N(false);
    set_H(false);
    set_C(false);
#ifdef DEBUG
    printf("OR A, imm\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_XOR_A_imm() {
    A ^= sharedMemory.read_mem(PC++);
    set_Z(A == 0x00);
    set_N(false);
    set_H(false);
    set_C(false);
#ifdef DEBUG
    printf("XOR A, imm\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_CP_A_imm() {
    uint8_t temp = sharedMemory.read_mem(PC++);
    set_N(true);
    set_H((A & 0xF) < (temp & 0xF));
    set_C((A < temp));
    set_Z(A == temp);
#ifdef DEBUG
    printf("CP A, imm\n");
#endif
    cyclecount += 2;
    //cycle_delay(2);
}

void gb::OP_JP() {
    uint16_t address;
    address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    PC = address;
#ifdef DEBUG
    printf("JP 0x%X\n", address);
#endif
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::OP_JP_HL() {
    PC = HL();
#ifdef DEBUG
    printf("JP (HL)\n");
#endif
    cyclecount += 1;
    //cycle_delay(1);
}

void gb::OP_JP_test(uint8_t xx) {
    uint8_t delay = 3;
    uint16_t address;
    address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    //printf("JR cc %X\n", address);
    if(test_condition(xx)){
        //printf("Jumping now...\n");
        PC = address;
        delay++;
    }
#ifdef DEBUG
    printf("JP cc 0x%X\n", address);
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_JR() {
    uint16_t offset = sharedMemory.read_mem(PC++);
    if (isbiton(7, offset)) offset += 0xFF00;
    PC = PC + offset;
#ifdef DEBUG
    printf("JR 0x%X\n", PC);
#endif
    cyclecount += 3;
    //cycle_delay(3);
}

void gb::OP_JR_test(uint8_t cc) {
    uint8_t delay = 2;
    uint16_t offset = sharedMemory.read_mem(PC++);
    if (isbiton(7, offset)) offset += 0xFF00;
    if(test_condition(cc)){
        //printf("Jumping now... ");
        PC = (PC + offset) & 0xFFFF;
        delay++;
#ifdef DEBUG
        printf("JR cc 0x%X, branch taken\n", PC);
#endif
    }
#ifdef DEBUG
    else
        printf("JR cc 0x%X\n", PC);
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_CALL() {
    uint16_t address;
    address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    //printf("CALL %X, pushing %X onto stack\n", address, PC);
    SP--;
    write_mem(SP--, (PC >> 8));
    write_mem(SP, (PC & 0xFF));
    PC = address;
#ifdef DEBUG
    printf("CALL 0x%X\n", address);
#endif
    cyclecount += 6;
    //cycle_delay(6);
}

void gb::OP_CALL_test(uint8_t xx) {
    uint8_t delay = 3;
    uint16_t address;
    address = sharedMemory.read_mem(PC++);
    address += (sharedMemory.read_mem(PC++) << 8);
    if(test_condition(xx)) {
        SP--;
        write_mem(SP--, (PC >> 8));
        write_mem(SP, (PC & 0xFF));
        PC = address;
        delay += 3;
    }
#ifdef DEBUG
    printf("CALL cc 0x%X\n", address);
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_RET() {
    uint16_t newPC = sharedMemory.read_mem(SP++);
    newPC += (sharedMemory.read_mem(SP++) << 8);
    //printf("RET %X\n", newPC);
    PC = newPC;
#ifdef DEBUG
    printf("RET to 0x%X\n", newPC);
#endif
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::OP_RET_test(uint8_t xx) {
    uint8_t delay = 2;
    if(test_condition(xx))
    {
        delay += 3;
        uint16_t newPC = sharedMemory.read_mem(SP++);
        newPC += (sharedMemory.read_mem(SP++) << 8);
        PC = newPC;
#ifdef DEBUG
        printf("RET cc to 0x%X\n", newPC);
#endif
    }
#ifdef DEBUG
    else printf("RET cc not taken\n");
#endif
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::OP_RETI() {
    uint16_t newPC = sharedMemory.read_mem(SP++);
    newPC += (sharedMemory.read_mem(SP++) << 8);
    PC = newPC;
    IME = true;
#ifdef DEBUG
    printf("RETI to 0x%X\n", newPC);
#endif
    cyclecount += 4;
    //cycle_delay(4);
}

void gb::CB_RLC(uint8_t xxx) {
#ifdef DEBUG
    printf("RLC r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        tempHi = temp << 1;
        temp = temp >> 7;
        result = temp + tempHi;

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        tempHi = temp << 1;
        temp = temp >> 7;
        result = temp + tempHi;

        *reg = result;
    }
    set_C(temp == 1);
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_RL(uint8_t xxx) {
#ifdef DEBUG
    printf("RL r\n");
#endif
    uint8_t delay = 2;
    uint8_t result;
    uint16_t temp;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());
        temp = temp << 1;
        result = temp | get_C();
        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;
        temp = temp << 1;
        result = temp | get_C();
        *reg = result;
    }
    set_C(temp >> 8 != 0);
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_RRC(uint8_t xxx) {
#ifdef DEBUG
    printf("RRC r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        tempHi = temp << 7;
        temp = temp >> 1;
        result = temp + tempHi;

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        tempHi = temp << 7;
        temp = temp >> 1;
        result = temp + tempHi;

        *reg = result;
    }
    set_C(tempHi != 0);
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_RR(uint8_t xxx) {
#ifdef DEBUG
    printf("RR r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        tempHi = temp << 7;
        temp = temp >> 1;
        result = temp + (get_C() << 7);

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        tempHi = temp << 7;
        temp = temp >> 1;
        result = temp + (get_C() << 7);

        *reg = result;
    }
    set_C(tempHi != 0);
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_SLA(uint8_t xxx) {
#ifdef DEBUG
    printf("SLA r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        result = temp << 1;

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        result = temp << 1;

        *reg = result;
    }
    set_C(isbiton(7, temp));
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_SRA(uint8_t xxx) {
#ifdef DEBUG
    printf("SRA r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        result = temp >> 1;
        if (isbiton(7, temp))
            result += 0x80;

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        result = temp >> 1;
        if (isbiton(7, temp))
            result += 0x80;

        *reg = result;
    }
    set_C(isbiton(0, temp));
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_SWAP(uint8_t xxx) {
#ifdef DEBUG
    printf("SWAP r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());
        tempHi = temp << 4;
        temp = temp >> 4;
        result = temp + tempHi;
        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;
        tempHi = temp << 4;
        temp = temp >> 4;
        result = temp + tempHi;
        *reg = result;
    }
    set_C(false);
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_SRL(uint8_t xxx) {
#ifdef DEBUG
    printf("SRL r\n");
#endif
    uint8_t delay = 2;
    uint8_t temp, tempHi, result;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());

        result = temp >> 1;

        write_mem(HL(), result);
        delay = 4;
    }
    else {
        temp = *reg;

        result = temp >> 1;

        *reg = result;
    }
    set_C(isbiton(0, temp));
    set_H(false);
    set_N(false);
    set_Z(result == 0);
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_BIT(uint8_t bbb, uint8_t xxx) {
#ifdef DEBUG
    printf("BIT %d r\n", bbb);
#endif
    uint8_t delay = 2;
    uint8_t temp;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());
        delay = 4;
    }
    else {
        temp = *reg;
    }
    set_H(true);
    set_N(false);
    set_Z(!isbiton(bbb, temp));
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_SET(uint8_t bbb, uint8_t xxx) {
#ifdef DEBUG
    printf("SET %d r\n", bbb);
#endif
    uint8_t delay = 2;
    uint8_t temp;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());
        temp |= (0x01 << bbb);
        write_mem(HL(), temp);
        delay = 4;
    }
    else {
        temp = *reg;
        temp |= (0x01 << bbb);
        *reg = temp;
    }
    //flags are not affected here
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::CB_RES(uint8_t bbb, uint8_t xxx) {
#ifdef DEBUG
    printf("RES %d r\n", bbb);
#endif
    uint8_t delay = 2;
    uint8_t temp;
    uint8_t * reg = decode_register(xxx);
    if (xxx == 0x06) {
        temp = sharedMemory.read_mem(HL());
        temp &= ~(0x01 << bbb);
        write_mem(HL(), temp);
        delay = 4;
    }
    else {
        temp = *reg;
        temp &= ~(0x01 << bbb);
        *reg = temp;
    }
    //flags are not affected here
    cyclecount += delay;
    //cycle_delay(delay);
}

void gb::write_mem(uint16_t address, uint8_t value) {
#ifdef DEBUG
    //printf("write_mem. A: 0x%X D: 0x%X\n",address, value);
#endif
    if (address == 0xFF07)
    {
        //update timer controller!
        sharedMemory.write_mem(address, value);
        set_clock_freq();
    }
    else
        sharedMemory.write_mem(address, value);
}

uint8_t *gb::decode_register(uint8_t xxx) {
    switch (xxx)
    {
        case 0:
#ifdef DEBUG
            printf("B ");
#endif
            return &B;
        case 1:
#ifdef DEBUG
            printf("C ");
#endif
            return &C;
        case 2:
#ifdef DEBUG
            printf("D ");
#endif
            return &D;
        case 3:
#ifdef DEBUG
            printf("E ");
#endif
            return &E;
        case 4:
#ifdef DEBUG
            printf("H ");
#endif
            return &H;
        case 5:
#ifdef DEBUG
            printf("L ");
#endif
            return &L;
        case 6:
            return nullptr;
        case 7:
#ifdef DEBUG
            printf("A ");
#endif
            return  &A;
        default:
            return nullptr;
    }
}

/*void gb::cycle_delay(uint8_t cycles) {
    cycles_ran += cycles;
    if (cycles_ran >= 4194) //approximately the number of cycles in 1 ms (4194.304)
    {
        //clock.restart();
        cycles_ran = 0;
        //while(clock.getElapsedTime().asMilliseconds() < 1); //yep, just NOP
    }
}*/

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

uint16_t gb::AF() const {
    return (A << 8) + F;
}

void gb::write_AF(uint16_t value) {
    A = (value >> 8);
    F = (value & 0xFF);
}


uint8_t gb::get_C() const {
    return ((F & 0x10) >> 4);
}

uint8_t gb::get_H() const {
    return ((F & 0x20) >> 5);
}

uint8_t gb::get_N() const {
    return ((F & 0x40) >> 6);
}

bool gb::test_condition(uint8_t cc) const {
    bool test;
    switch (cc) {
        case 0:
            //NZ
            test = !isbiton(7, F);
            break;
        case 2:
            //NC
            test = !isbiton(4, F);
            break;
        case 1:
            //Z
            test = isbiton(7, F);
            break;
        case 3:
            //C
            test = isbiton(4, F);
            break;
        default:
            break;
    }
    return test;
}


void gb::update_timers(uint32_t cycles) {
    handle_div_reg(cycles*4);
    if(isclockenabled()){
        timer_counter -= (cycles * 4);

        // have enough cpu cycles happened to update timer?
        if (timer_counter <= 0)
        {
            //reset timer_counter to correct value
            set_clock_freq();

            //is timer about to overflow?
            if(sharedMemory.read_mem(0xFF05) == 255)
            {
                write_mem(0xFF05, sharedMemory.read_mem(0xFF06));
                request_interrupt(2);
            }
            else
            {
                write_mem(0xFF05, (sharedMemory.read_mem(0xFF05)+1));
            }
        }
    }

}

bool gb::isclockenabled() {
    return isbiton(2, sharedMemory.read_mem(0xFF07));
}

void gb::handle_div_reg(uint32_t cycles) {
    divider_counter += cycles;
    if(divider_counter >= 255)
    {
        divider_counter = 0;
        sharedMemory.write_mem(0xFF04, sharedMemory.read_mem(0xFF04)+1);
    }
}

void gb::set_clock_freq() {
    uint8_t freq = sharedMemory.read_mem(0xFF07) & 0x03;
    switch (freq)
    {
        case 0: timer_counter = 1024;
        case 1: timer_counter = 16;
        case 2: timer_counter = 64;
        case 3: timer_counter = 256;
    }
}
