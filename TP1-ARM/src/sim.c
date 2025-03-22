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
#define CMP_EXT_OP  0b11101011000
#define ANDS_SR_OP  0b11101010000
#define EOR_SR_OP   0b11001010000
#define ORR_SR_OP   0b10101010000
#define B_OP   0b000101
#define BR_OP   0b11010110000
#define B_COND_OP   0b01010100

// Declaraciones de funciones
void update_flags(int64_t result);
uint64_t zero_extend(uint32_t shift, uint32_t imm12);
void halt(uint32_t instruction);
void adds_subs_ext(uint32_t instruction, int update_flag, int addition);
void adds_subs_immediate(uint32_t instruction, int update_flag, int addition);
void logical_shifted_register(uint32_t instruction, int op);
void branch(uint32_t instruction);
void branch_register(uint32_t instruction);
void branch_conditional(uint32_t instruction);


void process_instruction() {
    uint32_t instruction;
    uint32_t opcode;
    uint32_t rd, rn, rm, shamt;
    int64_t imm;
    int64_t imm19;
    int64_t imm26;
    int64_t cond;
    int64_t offset;
    uint64_t addr;

    instruction = mem_read_32(CURRENT_STATE.PC);

    for (int length = 11; length >= 6; length--) { 
        uint32_t opcode = (instruction >> (32 - length)) & ((1 << length) - 1);

        switch(opcode) {
            case ADD_IMM_OP:
                adds_subs_immediate(instruction, 0, 1);
                break;
            case ADDS_IMM_OP:
                adds_subs_immediate(instruction, 1, 1);
                break;
            case ADD_EXT_OP:
                adds_subs_ext(instruction, 0, 1);
                break;
            case ADDS_EXT_OP:
                adds_subs_ext(instruction, 1, 1);
                break;
            case SUBS_IMM_OP:
                adds_subs_immediate(instruction, 1, 0);
                break;
            case SUBS_EXT_OP:
                adds_subs_ext(instruction, 1, 0);
                break;
            case ANDS_SR_OP:
                logical_shifted_register(instruction, 0);
                break;
            case EOR_SR_OP:
                logical_shifted_register(instruction, 1);
                break;
            case ORR_SR_OP:
                logical_shifted_register(instruction, 2);
                break;
            case B_OP:
                branch(instruction);
                return;
            case BR_OP:
                branch_register(instruction);
                return;
            case B_COND_OP:
                branch_conditional(instruction);
                return;
            case HLT_OP:
                halt(instruction);
                break;
        }
    }
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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

void halt(uint32_t instruction){
    RUN_BIT = 0;
}

void adds_subs_ext(uint32_t instruction, int update_flag, int addition) {
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
    if (rd != 31) {
        NEXT_STATE.REGS[rd] = result;
    }
    if (update_flag == 1){
        update_flags(result);
    }
}

void adds_subs_immediate(uint32_t instruction, int update_flag, int addition){
    uint16_t rd = instruction & 0b11111;
    uint16_t rn = (instruction >> 5) & 0b11111;
    uint16_t shift = (instruction >> 22) & 0b11;
    uint16_t imm12 = (instruction >> 10) & 0b111111111111;
    uint64_t imm = zero_extend(shift, imm12);
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t result;
    if (addition == 1){
        result = operand1 + imm;
    }
    else if (addition == 0){
        result = operand1 - imm;
    }
    if (rd != 31){
        NEXT_STATE.REGS[rd] = result;
    }
    if (update_flag == 1){ 
        update_flags(result);
    }
}

void logical_shifted_register(uint32_t instruction, int op) {
    uint32_t rd = instruction & 0b11111;
    uint32_t rn = (instruction >> 5) & 0b11111;
    uint32_t rm = (instruction >> 16) & 0b11111;
    
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    uint64_t result;

    // Realizar la operación lógica según el opcode
    switch(op) {
        case 0: // ANDS
            result = operand1 & operand2;
            break;
        case 1: // EOR
            result = operand1 ^ operand2;
            break;
        case 2: // ORR
            result = operand1 | operand2;
            break;
        default:
            return;
    }
    if (rd != 31) {
        NEXT_STATE.REGS[rd] = result;
    }
    if (op == 0) {
        update_flags(result);
    }
}

void branch(uint32_t instruction) {
    // Extraer los 26 bits del immediate
    int32_t imm26 = instruction & 0x3FFFFFF;  // 0x3FFFFFF = 0b11111111111111111111111111
    
    // Si el bit más significativo de imm26 está activado (es negativo)
    if (imm26 & 0x2000000) {  // 0x2000000 es el bit 26 (0b10000000000000000000000000)
        imm26 |= ~0x3FFFFFF;  // Extender el signo rellenando con 1s
    }
    
    // Concatenar con '00' (shift left por 2)
    int64_t offset = ((int64_t)imm26) << 2;
    
    NEXT_STATE.PC = CURRENT_STATE.PC + offset;
}

void branch_register(uint32_t instruction) {
    uint32_t rn = instruction & 0b11111;  // Extraer los 5 bits del registro
    NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
}

void branch_conditional(uint32_t instruction) {
    uint32_t cond = instruction & 0b1111;  // Extraer los 4 bits de condición (bits [3:0])
    
    // Modificación aquí: necesitamos hacer la extensión de signo correctamente
    int32_t imm19 = ((instruction >> 5) & 0x7FFFF);  // Extraer los 19 bits
    
    // Si el bit más significativo de imm19 está activado (es negativo)
    if (imm19 & 0x40000) {  // 0x40000 es el bit 19 (0b100000000000000000)
        imm19 |= ~0x7FFFF;  // Extender el signo rellenando con 1s
    }
    
    int64_t offset = ((int64_t)imm19) << 2;   // Concatena con '00'

    int should_branch = 0;
    switch(cond) {
        case 0b0000: // EQ
            should_branch = CURRENT_STATE.FLAG_Z;
            break;
        case 0b0001: // NE
            should_branch = !CURRENT_STATE.FLAG_Z;
            break;
        case 0b1010: // GE
            should_branch = !CURRENT_STATE.FLAG_N;
            break;
        case 0b1011: // LT
            should_branch = CURRENT_STATE.FLAG_N;
            break;
        case 0b1100: // GT
            should_branch = (!CURRENT_STATE.FLAG_Z && !CURRENT_STATE.FLAG_N);
            break;
        case 0b1101: // LE
            should_branch = (CURRENT_STATE.FLAG_Z || CURRENT_STATE.FLAG_N);
            break;
    }

    if (should_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

