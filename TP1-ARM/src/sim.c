#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "shell.h"



/* Máscaras útiles para extraer campos de instrucciones */
#define OPCODE_MASK      0xFFFFFFFF
#define COND_MASK        0xF0000000
#define OP_MASK          0x1F000000
#define RD_MASK          0x0000001F  /* Bits 0-4 */
#define RN_MASK          0x000003E0  /* Bits 5-9 */
#define RM_MASK          0x001F0000  /* Bits 16-20 */
#define IMM12_MASK       0x003FFC00  /* Bits 10-21 */
#define IMM9_MASK        0x001FF000  /* Bits 12-20 */
#define IMM26_MASK       0x03FFFFFF  /* Bits 0-25 */
#define IMM19_MASK       0x00FFFFE0  /* Bits 5-23 */
#define IMM16_MASK       0x001FFFE0  /* Bits 5-20 */
#define SHAMT_MASK       0x00C00000  /* Bits 22-23 */
#define SHAMTW_MASK      0x0000FC00  /* Bits 10-15 (para LSL/LSR) */
#define N_MASK           0x00200000  /* Bit 21 */
#define ADDR_MASK        0x000003E0  /* Bits 5-9 */
#define SHIFT_MASK       0x00C00000  /* Bits 22-23 */

/* Valores de desplazamiento para extraer campos */
#define RD_SHIFT         0
#define RN_SHIFT         5
#define RM_SHIFT         16
#define IMM12_SHIFT      10
#define IMM9_SHIFT       12
#define IMM26_SHIFT      0
#define IMM19_SHIFT      5
#define IMM16_SHIFT      5
#define SHAMT_SHIFT      22
#define SHAMTW_SHIFT     10
#define N_SHIFT          21
#define ADDR_SHIFT       0


// /* Códigos de condición para B.cond */
// #define EQ               0x0
// #define NE               0x1
// #define GT               0xC
// #define LT               0xB
// #define GE               0xA
// #define LE               0xD


/* Códigos de operación (opcodes) */
#define ADD_EXT_OP  0b10001011001
#define ADD_IMM_OP  0b10010001
#define ADDS_EXT_OP 0b10101011000
#define ADDS_IMM_OP 0b10110001
#define SUBS_EXT_OP 0b11101011000
#define SUBS_IMM_OP 0b11110001
#define HLT_OP      0b11010100010
#define CMP_EXT_OP  0b11101011001
// #define CMP_IMM_OP  0b11110001 ??
#define ANDS_SR_OP  0b11101010000
#define EOR_SR_OP   0b11001010000
#define ORR_SR_OP   0b10101010000

void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
}
/*
 * Función principal para procesar una instrucción
 */
void process_instruction() {
    uint32_t instruction;
    uint32_t opcode;
    uint32_t rd, rn, rm, shamt;
    int64_t imm;
    int64_t offset;
    uint64_t addr;
    
    /* Obtener la instrucción actual de la memoria */
    instruction = mem_read_32(CURRENT_STATE.PC);
    printf("Instrucción: %x\n", instruction);
    // ACA TENDRIA Q HABER FUNCION PARA LEER EL OPCODE uint32_t opcode = (instruction >> 24) & 0b11111111;
    if(opcode == ADD_IMM_OP) {
        uint32_t rd = instruction & 0b11111;
        uint32_t rn = (instruction >> 5) & 0b11111;
        uint32_t shift = (instruction >> 22) & 0b11;
        uint32_t imm12 = (instruction >> 10) & 0b111111111111;

        uint64_t imm;
        if (shift == 0b00) {
            imm = (uint64_t)imm12;
        } else if (shift == 0b01){
            imm = (uint64_t)imm12 << 12;
        }
     
        uint64_t operand1 = CURRENT_STATE.REGS[rn];
        uint64_t result = operand1 + imm;
        
        NEXT_STATE.REGS[rd] = result;
    }
}