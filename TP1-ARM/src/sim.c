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
#define B_OP        0b000101
#define BR_OP       0b11010110000
#define B_COND_OP   0b01010100
#define LSLR_IMM    0b110100110
#define STUR_OP     0b11111000000
#define STURB_OP    0b00111000000
#define STURH_OP    0b01111000000
#define LDUR        0b11111000010
#define LDURB       0b00111000010
#define LDURH       0b01111000010
#define MUL         0b10011011000  
#define MOVZ        0b110100101
#define CBZ         0b10110100
#define CBNZ        0b10110101

// Declaraciones de funciones
void update_flags(int64_t result);
void halt(uint32_t instruction);
void adds_subs_ext(uint32_t instruction, int update_flag, int addition);
void adds_subs_immediate(uint32_t instruction, int update_flag, int addition);
void logical_shifted_register(uint32_t instruction, int op);
void branch(uint32_t instruction);
void branch_register(uint32_t instruction);
void branch_conditional(uint32_t instruction);
void lslr_imm(uint32_t instruction);
void stur(uint32_t instruction);
void sturb(uint32_t instruction);
void sturh(uint32_t instruction);
int64_t sign_extend(int64_t value, int bits);
void ldur(uint32_t instruction);
void ldurbh(uint32_t instruction, int b);
void movz(uint32_t instruction);
void mul(uint32_t instruction);
void cbzn(uint32_t instruction, int bz);

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
            case LSLR_IMM:
                lslr_imm(instruction);
                break;
            case STUR_OP:
                stur(instruction);
                break;
            case STURB_OP:
                sturb(instruction);
                break;
            case STURH_OP:
                sturh(instruction);
                break;
            case LDUR:
                ldur(instruction);
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
                break;
            case CBNZ:
                cbzn(instruction, 0);
                break;
                
        }
    }
    CURRENT_STATE.REGS[31] = 0;
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}


void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
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
    uint64_t imm;
    if (shift == 0b00) {
        imm = (uint64_t)imm12;
    } else if (shift == 0b01){
        imm = (uint64_t)imm12 << 12;
    }
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
    switch(op) {
        case 0: // ANDS
            result = operand1 & operand2;
            break;
        case 1: // EOR
            result = operand1 ^ operand2;
            break;
        case 2: // OR
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
    int32_t imm26 = instruction & 0x3FFFFFF;  // 0x3FFFFFF = 0b11111111111111111111111111
    
    if ((imm26 >> 25) & 0b1) {  //chequea q el bit mas signofocativo sea 1
        imm26 |= ~0x3FFFFFF;  // aca estamos extendiendo el immediate con 6 unos digamos q van del bit 26 al 31 (los mas signficiativos)
    }
    
    int64_t offset = ((int64_t)imm26) << 2; //tenemos zero extend q funciona devolviendo un uint, podemos usarla en este caso q es int64_t?? dejamos esta linea o nos creamos una funcion zero extend para signed ints
    
    NEXT_STATE.PC = CURRENT_STATE.PC + offset;
}



void branch_register(uint32_t instruction) {
    uint32_t rn = instruction & 0b11111;  // aca sacamos los 5 bits del registro al q hay q ir y se va para ahi
    NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
}

void branch_conditional(uint32_t instruction) {
    uint32_t cond = instruction & 0b1111;  // los primeros 4 bits de la instruccion son el condicional
    
    int32_t imm19 = ((instruction >> 5) & 0x7FFFF);  
    
    // lo mismo d antes d si el bit mas significativo del immm19 es 1 entonces extiende a 32 bits poniendi unos adelantes
    if ((imm19 >> 18) & 0b1) {  
        imm19 |= ~0x7FFFF;  // Extender el signo rellenando con 1s
    }
    
    int64_t offset = ((int64_t)imm19) << 2;   //aca d nuevo estamos haciendo zero extend preguntar bien q onda si se pUede usar el otro 

    int should_branch = 0;
    switch(cond) {
        case 0b0000: // EQ
            should_branch = CURRENT_STATE.FLAG_Z;
            break;
        case 0b0001: // NE
            should_branch = !CURRENT_STATE.FLAG_Z;
            break;
        case 0b1010: // GT
            should_branch = !CURRENT_STATE.FLAG_N;
            break;
        case 0b1011: // LT
            should_branch = CURRENT_STATE.FLAG_N;
            break;
        case 0b1100: // GE
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

void lslr_imm(uint32_t instruction){
    uint8_t N = (instruction >> 22) & 0b1;
    uint8_t imms = (instruction >> 10) & 0b111111;
    uint8_t Rn = (instruction >> 5) & 0b11111;
    uint8_t Rd = instruction & 0b11111;
    uint64_t operand1 = CURRENT_STATE.REGS[Rn]; 
        // PREGUNTAR SI chequeo q se cumpla la condicion esa  N xq onda si es 64 si se deberia cumplir tipo asumo. 
        uint64_t result;
    if (N == 1 && imms != 0b111111){
        uint8_t shift = 63 - imms;
        result = operand1 << shift;
    }
    else if (N ==1 && imms == 0b111111){
        uint8_t shift = (instruction >> 16) & 0b111111;
        result = operand1 >> shift; 
    }
    NEXT_STATE.REGS[Rd] = result;
}

void stur(uint32_t instruction) {
    uint32_t rt = instruction & 0b11111;         
    uint32_t rn = (instruction >> 5) & 0b11111;  
    uint32_t imm9 = (instruction >> 12) & 0b111111111;

    int64_t offset;
    if ((imm9 >> 8) & 0b1) {
        offset = imm9 | ~0x1FF;
    } else {
        offset = imm9;
    }
    
    uint64_t address;
    if (rn == 31) {  // Si es SP
        address = CURRENT_STATE.REGS[31];  // SP
    } else {
        address = CURRENT_STATE.REGS[rn];
    }
    
    address = address + offset;
    
    uint64_t value = CURRENT_STATE.REGS[rt];
    mem_write_32(address, value & 0xFFFFFFFF);        // 32 bits inferiores
    mem_write_32(address + 4, value >> 32);           // 32 bits superiores
}

void sturb(uint32_t instruction) {
    uint32_t rt = instruction & 0b11111;         
    uint32_t rn = (instruction >> 5) & 0b11111;  
    uint32_t imm9 = (instruction >> 12) & 0b111111111;

    int64_t offset;
    if ((imm9 >> 8) & 0b1) {
        offset = imm9 | ~0x1FF;
    } else {
        offset = imm9;
    }
    
    uint64_t address;
    if (rn == 31) {  // Si es SP
        address = CURRENT_STATE.REGS[31];  // SP
    } else {
        address = CURRENT_STATE.REGS[rn];
    }
    
    address = address + offset;
    
    
    uint32_t current_value = mem_read_32(address);
    
    uint8_t byte_offset = offset & 0x3;
    
    uint32_t mask = ~(0xFF << (byte_offset * 8));
    
    uint32_t new_value = (current_value & mask) | ((CURRENT_STATE.REGS[rt] & 0xFF) << (byte_offset * 8));
    
    mem_write_32(address, new_value);
}

void sturh(uint32_t instruction) {
    uint32_t rt = instruction & 0b11111;         
    uint32_t rn = (instruction >> 5) & 0b11111;  
    uint32_t imm9 = (instruction >> 12) & 0b111111111;
    
    int64_t offset;
    if ((imm9 >> 8) & 0b1) {
        offset = imm9 | ~0x1FF;
    } else {
        offset = imm9;
    }
    
    uint64_t address;
    if (rn == 31) {  // Si es SP
        address = CURRENT_STATE.REGS[31];  // SP
    } else {
        address = CURRENT_STATE.REGS[rn];
    }
    
    address = address + offset;
    
    uint32_t current_value = mem_read_32(address);
    
    uint8_t halfword_offset = (offset & 0x2) ? 16 : 0;
    
    uint32_t mask = ~(0xFFFF << halfword_offset);
    
    uint32_t new_value = (current_value & mask) | ((CURRENT_STATE.REGS[rt] & 0xFFFF) << halfword_offset);
    
    mem_write_32(address, new_value);
}

int64_t sign_extend(int64_t value, int bits) {
    int64_t mask = 1LL << (bits - 1);
    return (value ^ mask) - mask;
}
void ldur(uint32_t instruction) {
    uint8_t Rn = (instruction >> 5) & 0b11111;    
    uint8_t Rt = instruction & 0b11111;           
    int16_t imm9 = (instruction >> 12) & 0b111111111;     

    int64_t offset = sign_extend(imm9, 9); //genera que se extienda el imm a 64 bits sin perder el signo ni el numero

    uint64_t address = (uint64_t)CURRENT_STATE.REGS[Rn]; //lee los 64 bits de rn q es el regisyto de donde se lee 
    address = address + offset;
    // Como porgramammos para 64 bits tengo q hacer 2 lecturas lo q solo leo de a 32 
    uint64_t lower = (uint64_t)mem_read_32(address);
    uint64_t upper = (uint64_t)mem_read_32(address + 4);
    uint64_t value = lower | (upper << 32);

    NEXT_STATE.REGS[Rt] = value;
}

void ldurbh(uint32_t instruction, int b) {
    uint8_t Rn = (instruction >> 5) & 0b11111;    
    uint8_t Rt = instruction & 0b11111;           
    int16_t imm9 = (instruction >> 12) & 0b111111111;    
    int64_t offset = sign_extend(imm9, 9);
    uint64_t address = (uint64_t)CURRENT_STATE.REGS[Rn] + offset;
    uint32_t word = mem_read_32(address);
    uint32_t result_value = (b == 1) ? (word & 0b11111111) : (word & 0b1111111111111111);
    NEXT_STATE.REGS[Rt] = result_value;
}

void movz(uint32_t instruction) {
    uint8_t rd = instruction & 0b11111;         // Rd
    uint16_t imm16 = (instruction >> 5) & 0xFFFF; // imm16

    uint64_t result = (uint64_t)imm16;
    
    // Guardamos el resultado en el registro destino
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
    uint32_t imm = (instruction >> 5) & 0b1111111111111111111;
    uint64_t offset = sign_extend(imm << 2, 21);
    uint64_t operand1 = CURRENT_STATE.REGS[Rt];
    if (bz == 1) {
        if (operand1 == 0){
            NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        }
    }
    else if (bz == 0) {
        if (operand1 != 0){
            NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        }
    }
}


