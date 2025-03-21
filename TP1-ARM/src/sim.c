#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "shell.h"

// opcodes
#define ADD_EXT_OP  0b10001011001
#define ADD_IMM_OP  0b10010001
#define ADDS_EXT_OP 0b10101011000
#define ADDS_IMM_OP 0b10110001
#define SUBS_EXT_OP 0b11101011000
#define SUBS_IMM_OP 0b11110001
#define HLT_OP      0b11010100010
#define CMP_EXT_OP  0b11101011001
#define ANDS_SR_OP  0b11101010000
#define EOR_SR_OP   0b11001010000
#define ORR_SR_OP   0b10101010000

// Declaraciones de funciones
void update_flags(int64_t result);
uint64_t zero_extend(uint32_t shift, uint32_t imm12);
void add_immediate(uint32_t instruction, int update_flag);
void halt(uint32_t instruction);
void adds_subs_ext(uint32_t instruction, int addition);
void subs_cmp_immediate(uint32_t instruction, int update_flag);


void process_instruction() {
    uint32_t instruction;
    uint32_t opcode;
    uint32_t rd, rn, rm, shamt;
    int64_t imm;
    int64_t offset;
    uint64_t addr;

    instruction = mem_read_32(CURRENT_STATE.PC);

    for (int length = 11; length >= 6; length--) { 
        uint32_t opcode = (instruction >> (32 - length)) & ((1 << length) - 1);

        if(opcode == ADD_IMM_OP) {
            add_immediate(instruction, 0);
        }
        if (opcode == ADDS_IMM_OP) {
            add_immediate(instruction, 1);
        }
        if (opcode == ADDS_EXT_OP) {
            adds_subs_ext(instruction, 1);
        }
        if (opcode == SUBS_IMM_OP) {
            subs_cmp_immediate(instruction, 1);
        }
        if (opcode == SUBS_EXT_OP) {
            adds_subs_ext(instruction, 0);
        }
        if( opcode == HLT_OP) {
            halt(instruction);
        }
    }
}


void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
}

uint64_t zero_extend(uint32_t shift, uint32_t imm12){
    uint64_t imm;
    if (shift == 0b00) {
        imm = (uint64_t)imm12;
    } else if (shift == 0b01){
        imm = (uint64_t)imm12 << 12;
    }
    return imm;
}

void add_immediate(uint32_t instruction, int update_flag){
    uint16_t rd = instruction & 0b11111;
    uint16_t rn = (instruction >> 5) & 0b11111;
    uint16_t shift = (instruction >> 22) & 0b11;
    uint16_t imm12 = (instruction >> 10) & 0b111111111111;
    uint64_t imm = zero_extend(shift, imm12);
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t result = operand1 + imm;
    NEXT_STATE.REGS[rd] = result;
    if (update_flag == 1){ 
        update_flags(result);
    }
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

void halt(uint32_t instruction){
    RUN_BIT = 0;
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

void adds_subs_ext(uint32_t instruction, int addition) {
    uint32_t rd = instruction & 0b11111;
    uint32_t rn = (instruction >> 5) & 0b11111;
    uint32_t rm = (instruction >> 16) & 0b11111;
    
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    uint64_t result;
    if (addition == 1){
        result = operand1 + operand2;
    }
    else if (addition == 0) {
        result = operand1 - operand2;
    }
    NEXT_STATE.REGS[rd] = result;
    update_flags(result);
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

void subs_cmp_immediate(uint32_t instruction, int update_flag){
    uint16_t rd = instruction & 0b11111;
    uint16_t rn = (instruction >> 5) & 0b11111;
    uint16_t shift = (instruction >> 22) & 0b11;
    uint16_t imm12 = (instruction >> 10) & 0b111111111111;
    uint64_t imm = zero_extend(shift, imm12);
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t result = operand1 - imm;
    if (rd != 31){
        NEXT_STATE.REGS[rd] = result;
    }
    if (update_flag == 1){ 
        update_flags(result);
    }
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}