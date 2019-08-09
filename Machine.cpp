#include <iostream>
#include <sys/select.h>
#include "Machine.h"

Machine::Machine()
{
    // Set the start program counter
    reg[R_PC] = PC_START;
}

void Machine::run()
{
    running = true;
    while (running) 
    {
        // Fetch
        uint16_t instruction = memRead(reg[R_PC]++);
        uint16_t opcode = instruction >> 12;

        uint16_t DR;
        uint16_t SR1;
        uint16_t SR2;
        uint16_t SR;
        uint16_t R;
        //printf("R_PC=%d opcode=%d\n", reg[R_PC], opcode);
        switch (opcode)
        {
            case OP_ADD:

                DR = (instruction >> 9) & 0x7;
                SR1 = (instruction >> 6) & 0x7;
                if ((instruction & 0x20) == 0x20)
                {
                    reg[DR] = reg[SR1] + sext(instruction, 5);
                } else
                {
                    SR2 = (instruction & 0x7);
                    reg[DR] = reg[SR1] + reg[SR2];
                }
                setFlags(DR);
                break;
            case OP_AND:
                DR = (instruction >> 9) & 0x7;
                SR1 = (instruction >> 6) & 0x7;
                if ((instruction & 0x20) == 0x20)
                {
                    reg[DR] = reg[SR1] & sext(instruction, 5);
                } else
                {
                    SR2 = (instruction & 0x7);
                    reg[DR] = reg[SR1] & reg[SR2];
                }
                setFlags(DR);
                break;
            case OP_NOT:
                DR = (instruction >> 9) & 0x7;
                SR = (instruction >> 6) & 0x7;
                reg[DR] = ~reg[SR];
                setFlags(DR);
                break;
            case OP_BR:
                if (instruction >> 9 & 0x7 & reg[R_COND])
                {
                    reg[R_PC] = reg[R_PC] + sext(instruction, 9);
                }

                break;
            case OP_JMP:
                reg[R_PC] = reg[(instruction >> 6) & 0x7];
                break;
            case OP_JSR:
                reg[R_R7] = reg[R_PC];
                if ((instruction & 0x0800) == 0x0800)
                {
                    reg[R_PC] += sext(instruction, 11);
                } else
                {
                    R = (instruction >> 6) & 0x7;
                    reg[R_PC] = reg[R];            
                }
                break;
            case OP_LD:
                DR = (instruction >> 9) & 0x7;
                reg[DR] = memRead(reg[R_PC] + sext(instruction, 9));
                setFlags(DR);
                break;
            case OP_LDI:
                DR = (instruction >> 9) & 0x7;
                reg[DR] = memRead(memRead(reg[R_PC] + sext(instruction, 9)));
                setFlags(DR);
               

                break;
            case OP_LDR:
                DR = (instruction >> 9) & 0x7;
                R = (instruction >> 6) & 0x7;
                reg[DR] = memRead(reg[R] +sext(instruction, 6));
                setFlags(DR);
                break;
            case OP_LEA:
                DR = (instruction >> 9) & 0x7;
                reg[DR] = reg[R_PC] + sext(instruction, 9);
                setFlags(DR);
                break;
            case OP_ST:
                SR = (instruction >> 9) & 0x7;
                memWrite(reg[R_PC] + sext(instruction, 9), reg[SR]);

                break;
            case OP_STI:
                SR = (instruction >> 9) & 0x7;
                memWrite(memRead(reg[R_PC] + sext(instruction, 6)), reg[SR]);
                break;
            case OP_STR:
                SR = (instruction >> 9) & 0x7;
                R = (instruction >> 6) & 0x7;
                memWrite(reg[R] + sext(instruction, 6), reg[SR]);
                break;
            case OP_TRAP:
                /* TRAP */
                trapFunc(instruction);

                break;
            case OP_RES:
            case OP_RTI:
            default:
                /* BAD OPCODE */
                abort();

                break;
        }
    }
}

int Machine::readImage(std::string filename)
{
    FILE* imageFile = fopen(filename.c_str(), "rb");
    if (!imageFile)
        return 1;
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, imageFile);
    origin = swap16(origin);

    uint16_t max_read = UINT16_MAX - origin;
    uint16_t *p = mem + origin;
    
    size_t wordsRead = fread(p, sizeof(uint16_t), max_read, imageFile);

    while(wordsRead-- > 0)
    {
        *p = swap16(*p);
        p++;
    }
    return 0;
}

uint16_t Machine::memRead(uint16_t addr)
{
    if (addr == MR_KBSR)
    {
        if (checkKey())
        {
            mem[MR_KBSR] = (1 << 15);
            mem[MR_KBDR] = getchar();
        }
        else
        {
            mem[MR_KBSR] = 0;
        }
        
    }
    return mem[addr];
}

void Machine::memWrite(uint16_t addr, uint16_t v)
{
    mem[addr] = v;
}

uint16_t Machine::sext(uint16_t A, int msb)
{
    if (A >> (msb-1) & 1)
    {
        return A | (0xFFFF << msb);
    } else
    {
        return A & (0xFFFF >> (16 - msb));
    }
}

uint16_t Machine::zext(uint16_t A, int msb)
{
    return A & ((0xFFFF >> (16 - msb)));
}

void Machine::setFlags(uint16_t R)
{
    if (reg[R] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }else if (reg[R] >> 15)
    {
        reg[R_COND] = FL_NEG;
    }else
    {
        reg[R_COND] = FL_POS;
    }
}

void Machine::trapFunc(uint16_t instruction)
{   
    //printf("Trap code opcode=%04x\n", instruction & 0xFF);
    uint16_t *c;
    switch (instruction & 0x00FF)
    {
    case TRAP_PUTS:
        c = mem + reg[R_R0];
        while (*c) 
        {
            putc((char)*c, stdout);
            c++;
        }   
        break;
    case TRAP_GETC:
        reg[R_R0] = (uint16_t)getchar();
        break;
    case TRAP_OUT:
        putc(char(reg[R_R0]), stdout);
        fflush(stdout);
        break;
    case TRAP_IN:
        printf("Enter a character");
        reg[R_R0] = (uint16_t)getchar();
        putc(char(reg[R_R0]), stdout);
        fflush(stdout);
        break;
    case TRAP_PUTSP:
        c = mem + reg[R_R0];
        while (*c) 
        {
            putc((char)((*c)&0xFF), stdout);
            if ((*c)>>8)
                putc((char)((*c) >> 8), stdout);
            c++;
        }
        fflush(stdout);
        break;
    case TRAP_HALT:
        running = false;
        printf("We are now done.\nKind Regards from Usey");
        
        break;
    default:
        break;
    }
}

uint16_t Machine::swap16(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

uint16_t Machine::checkKey()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}