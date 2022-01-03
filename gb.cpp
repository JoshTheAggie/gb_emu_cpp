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
        //get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        //go back to beginning of file and fill buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        //load ROM contents into the gb's memory, starting at 0x0
        for (int i = 0; i < size; i++)
        {
            memory[i] = buffer[i];
        }

        //free the buffer
        delete[] buffer;
    }

}

void gb::LoadROM(const char *filename)
{
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
                break;
            case 1: //01
                break;
            case 2: //10
                break;
            case 3: //11
                break;
            default:
                OP_NOP();
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
                break;
            default:
                OP_NOP();
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
//todo: Opcode implementations here

