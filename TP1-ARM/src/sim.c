#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "shell.h"

#define ADD_EXT  0b10001011000
#define ADD_IMM  0b10010001
#define ADDS_EXT 0b10101011000
#define ADDS_IMM 0b10110001
#define SUBS_EXT 0b11101011000
#define SUBS_IMM 0b11110001
#define HLT      0b11010100010
#define CMP_EXT  0b11101011000
#define ANDS_SR  0b11101010000
#define EOR_SR   0b11001010000
#define ORR_SR   0b10101010000
#define B        0b000101
#define BR       0b11010110000
#define B_COND   0b01010100
#define LSLR_IMM 0b110100110
#define STUR     0b11111000000
#define STURB    0b00111000000
#define STURH    0b01111000000
#define LDUR     0b11111000010
#define LDURB    0b00111000010
#define LDURH    0b01111000010
#define MUL      0b10011011000  
#define MOVZ     0b110100101
#define CBZ      0b10110100
#define CBNZ     0b10110101

void update_flags(int64_t result);
void halt(uint32_t instruction);
void adds_subs_ext(uint32_t instruction, int update_flag, int addition);
void adds_subs_immediate(uint32_t instruction, int update_flag, int addition);
void logical_shifted_register(uint32_t instruction, int op);
void branch(uint32_t instruction);
void branch_register(uint32_t instruction);
void branch_conditional(uint32_t instruction);
void lslr_imm(uint32_t instruction);
void sturbh(uint32_t instruction, int sb);
int64_t sign_extend(int64_t value, int bits);
void ldurbh(uint32_t instruction, int b);
void movz(uint32_t instruction);
void mul(uint32_t instruction);
void cbzn(uint32_t instruction, int bz);
void add_sub(uint64_t operand1, uint64_t operand2, uint8_t rd, int update_flag, int addition);
uint8_t get_rd(uint32_t instruction);
uint8_t get_rn(uint32_t instruction);
uint8_t get_rm(uint32_t instruction);

void process_instruction() {
    uint32_t instruction;
    uint32_t opcode;
    instruction = mem_read_32(CURRENT_STATE.PC);
    for (int length = 11; length >= 6; length--) { 
        opcode = (instruction >> (32 - length)) & ((1 << length) - 1);
        switch(opcode) {
            case ADD_IMM:
                adds_subs_immediate(instruction, 0, 1);
                break;
            case ADDS_IMM:
                adds_subs_immediate(instruction, 1, 1);
                break;
            case ADD_EXT:
                adds_subs_ext(instruction, 0, 1);
                break;
            case ADDS_EXT:
                adds_subs_ext(instruction, 1, 1);
                break;
            case SUBS_IMM:
                adds_subs_immediate(instruction, 1, 0);
                break;
            case SUBS_EXT:
                adds_subs_ext(instruction, 1, 0);
                break;
            case ANDS_SR:
                logical_shifted_register(instruction, 0);
                break;
            case EOR_SR:
                logical_shifted_register(instruction, 1);
                break;
            case ORR_SR:
                logical_shifted_register(instruction, 2);
                break;
            case B:
                branch(instruction);
                NEXT_STATE.REGS[31] = 0;
                return;
            case BR:
                branch_register(instruction);
                NEXT_STATE.REGS[31] = 0;
                return;
            case B_COND:
                branch_conditional(instruction);
                NEXT_STATE.REGS[31] = 0;
                return;
            case HLT:
                halt(instruction);
                break;
            case LSLR_IMM:
                lslr_imm(instruction);
                break;
            case STUR:
                sturbh(instruction, 0);
                break;
            case STURB:
                sturbh(instruction, 1);
                break;
            case STURH:
                sturbh(instruction, 2);
                break;
            case LDUR:
                ldurbh(instruction, 2);
                break;
            case LDURB:
                ldurbh(instruction, 1);
                break;
            case LDURH:
                ldurbh(instruction, 0);
                break;
            case MOVZ:
                movz(instruction);
                break;
            case MUL:
                mul(instruction);
                break;
            case CBZ:
                cbzn(instruction, 1);
                return;
            case CBNZ:
                cbzn(instruction, 0);
                return;
        }
    }
    NEXT_STATE.REGS[31] = 0; 
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

uint8_t get_rd(uint32_t instruction) { return instruction & 0b11111; }
uint8_t get_rn(uint32_t instruction) { return (instruction >> 5) & 0b11111; }
uint8_t get_rm(uint32_t instruction) { return (instruction >> 16) & 0b11111; }

void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
}

void halt(uint32_t instruction){
    RUN_BIT = 0;
}

void add_sub(uint64_t operand1, uint64_t operand2, uint8_t rd, int update_flag, int addition) {
    uint64_t result = addition ? operand1 + operand2 : operand1 - operand2;
    NEXT_STATE.REGS[rd] = result;
    if (update_flag) {
        update_flags(result);
    }
}

void adds_subs_ext(uint32_t instruction, int update_flag, int addition) {
    uint8_t rd = get_rd(instruction);
    uint8_t rn = get_rn(instruction);
    uint8_t rm = get_rm(instruction);
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    uint64_t result;
    add_sub(operand1, operand2, rd, update_flag, addition);
}

void adds_subs_immediate(uint32_t instruction, int update_flag, int addition){
    uint8_t rd = get_rd(instruction);
    uint8_t rn = get_rn(instruction);
    uint8_t shift = (instruction >> 22) & 0b11;
    uint16_t imm12 = (instruction >> 10) & 0b111111111111;
    uint64_t imm = (shift == 0b00) ? (uint64_t)imm12 : (uint64_t)imm12 << 12;
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    add_sub(operand1, imm, rd, update_flag, addition);
}

int64_t sign_extend(int64_t value, int bits) {
    int64_t mask = 1LL << (bits - 1);
    return (value ^ mask) - mask;
}

void logical_shifted_register(uint32_t instruction, int op) {
    uint8_t rd = get_rd(instruction);
    uint8_t rn = get_rn(instruction);
    uint8_t rm = get_rm(instruction);
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    uint64_t result;
    switch(op) {
        case 0: 
            result = operand1 & operand2;
            break;
        case 1: 
            result = operand1 ^ operand2;
            break;
        case 2:
            result = operand1 | operand2;
            break;
        default:
            return;
    }
    NEXT_STATE.REGS[rd] = result;
    if (op == 0) {
        update_flags(result);
    }
}

void branch(uint32_t instruction) {
    int32_t imm26 = instruction & 0b11111111111111111111111111; 
    int64_t offset = sign_extend(imm26, 26) << 2; 
    NEXT_STATE.PC = CURRENT_STATE.PC + offset;
}

void branch_register(uint32_t instruction) {
    uint8_t rn = instruction & 0b11111;   // chequea aca tete q estas sacando los primer 5 bits en vez de los del bit 5 al 9!!
    NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
}

void branch_conditional(uint32_t instruction) {
    uint8_t cond = instruction & 0b1111; 
    int32_t imm19 = ((instruction >> 5) & 0b1111111111111111111);  
    int64_t offset = sign_extend(imm19, 19) << 2;  
    int should_branch = 0;
    switch(cond) {
        case 0b0000: 
            should_branch = CURRENT_STATE.FLAG_Z;
            break;
        case 0b0001: 
            should_branch = !CURRENT_STATE.FLAG_Z;
            break;
        case 0b1010:
            should_branch = (!CURRENT_STATE.FLAG_N || CURRENT_STATE.FLAG_Z);
            break;
        case 0b1011:
            should_branch = (CURRENT_STATE.FLAG_N && !CURRENT_STATE.FLAG_Z);
            break;
        case 0b1100: 
            should_branch = (!CURRENT_STATE.FLAG_Z && !CURRENT_STATE.FLAG_N);
            break;
        case 0b1101:
            should_branch = (CURRENT_STATE.FLAG_Z || CURRENT_STATE.FLAG_N);
            break;
    }
    if (should_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void lslr_imm(uint32_t instruction){
    uint8_t N = (instruction >> 22) & 0b1;
    uint8_t imm6 = (instruction >> 10) & 0b111111;
    uint8_t Rn = (instruction >> 5) & 0b11111;
    uint8_t Rd = instruction & 0b11111;
    uint64_t operand1 = CURRENT_STATE.REGS[Rn]; 
    uint64_t result;
    if (N == 1 && imm6 != 0b111111){
        uint8_t shift = 63 - imm6;
        result = operand1 << shift;
    }
    else if (N ==1 && imm6 == 0b111111){
        uint8_t shift = (instruction >> 16) & 0b111111;
        result = operand1 >> shift; 
    }
    NEXT_STATE.REGS[Rd] = result;
}

void sturbh(uint32_t instruction, int sb) {
    uint8_t rt = instruction & 0b11111;         
    uint8_t rn = (instruction >> 5) & 0b11111;  
    uint16_t imm9 = (instruction >> 12) & 0b111111111;
    int64_t offset = sign_extend(imm9, 9);
    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t current_value = CURRENT_STATE.REGS[rt];
    if (sb == 0) {
        mem_write_32(address, current_value & 0b11111111111111111111111111111111);      
        mem_write_32(address + 4, current_value >> 32);  
        return;
    }
    uint64_t aligned_address = address & ~0b11;
    uint8_t byte_position = address & 0b11;
    uint32_t word = mem_read_32(aligned_address);
    uint64_t new_word;
    if (sb == 1) {
        uint8_t value = current_value & 0b11111111;
        uint32_t mask = ~(0b11111111 << (byte_position * 8));
        new_word = (word & mask) | (value << (byte_position * 8));
        mem_write_32(aligned_address, new_word);
    }
    else if (sb == 2) {
        uint16_t value = current_value & 0b1111111111111111; 
        if (byte_position == 3) {
            uint32_t mask1 = ~(0b11111111 << 24);   
            uint32_t insert1 = (value & 0b11111111) << 24; 
            new_word = (word & mask1) | insert1; 
            mem_write_32(aligned_address, new_word);
            uint32_t word2 = mem_read_32(aligned_address + 4);
            uint32_t mask2 = ~0b11111111; 
            uint32_t insert2 = (value >> 8) & 0b11111111; 
            uint32_t new_word2 = (word2 & mask2) | insert2;
            mem_write_32(aligned_address + 4, new_word2);
            return;
        } else {
            uint32_t mask = ~(0b1111111111111111 << (byte_position * 8)); 
            uint32_t insert = value << (byte_position * 8);
            new_word = (word & mask) | insert;
            mem_write_32(aligned_address, new_word);
        }
    }
} 

void ldurbh(uint32_t instruction, int b) {
    uint8_t Rn = (instruction >> 5) & 0b11111;    
    uint8_t Rt = instruction & 0b11111;           
    int16_t imm9 = (instruction >> 12) & 0b111111111;    
    int64_t offset = sign_extend(imm9, 9);
    uint64_t address = (uint64_t)CURRENT_STATE.REGS[Rn] + offset;
    uint64_t result_value;
    if (b == 0 || b == 1){
        uint32_t word = mem_read_32(address);
        result_value = (b == 1) ? (word & 0b11111111) : (word & 0b1111111111111111);
    }
    else if (b == 2){
        uint64_t lower = (uint64_t)mem_read_32(address);
        uint64_t upper = (uint64_t)mem_read_32(address + 4);
        result_value = lower | (upper << 32);
    }
    NEXT_STATE.REGS[Rt] = result_value;
}

void movz(uint32_t instruction) {
    uint8_t rd = instruction & 0b11111;         
    uint16_t imm16 = (instruction >> 5) & 0b1111111111111111;  
    uint64_t result = (uint64_t)imm16;
    NEXT_STATE.REGS[rd] = result;
}

void mul(uint32_t instruction){
    uint8_t Rd = instruction & 0b11111;
    uint8_t Rn = (instruction >> 5) & 0b11111;
    uint8_t Rm = (instruction >> 16) & 0b11111;
    uint64_t result;
    result = CURRENT_STATE.REGS[Rn] * CURRENT_STATE.REGS[Rm];
    NEXT_STATE.REGS[Rd] = result;
}

void cbzn(uint32_t instruction, int bz){
    uint8_t Rt = instruction & 0b11111;
    uint32_t imm19 = (instruction >> 5) & 0b1111111111111111111;
    uint64_t offset = sign_extend(imm19 << 2, 21);
    uint64_t operand1 = CURRENT_STATE.REGS[Rt];
    if ((bz == 1 && operand1 == 0) || (bz == 0 && operand1 != 0)) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}



