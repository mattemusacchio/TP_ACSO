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

/* Códigos de operación (opcodes) */
#define HLT_OPCODE       0x6A2
#define ADDS_EXT_OPCODE  0xAB000000
#define ADDS_IMM_OPCODE  0b10010001
#define SUBS_EXT_OPCODE  0xEB000000
#define SUBS_IMM_OPCODE  0xD1000000
#define ANDS_OPCODE      0xEA000000
#define EOR_OPCODE       0xCA000000
#define ORR_OPCODE       0xAA000000
#define B_OPCODE         0x14000000
#define BR_OPCODE        0xD61F0000
#define BCOND_OPCODE     0x54000000
#define LSL_OPCODE       0xD3400000
#define LSR_OPCODE       0xD340FC00
#define STUR_OPCODE      0xF8000000
#define STURB_OPCODE     0x38000000
#define STURH_OPCODE     0x78000000
#define LDUR_OPCODE      0xF8400000
#define LDURB_OPCODE     0x38400000
#define LDURH_OPCODE     0x78400000
#define MOVZ_OPCODE      0xD2800000
#define CBZ_OPCODE       0xB4000000
#define CBNZ_OPCODE      0xB5000000
#define CMP_OPCODE       0xEB00001F
#define MUL_OPCODE       0x9B000000
#define ADD_EXT_OPCODE   0x8B000000
#define ADD_IMM_OPCODE   0x91000000


// /* Códigos de condición para B.cond */
// #define EQ               0x0
// #define NE               0x1
// #define GT               0xC
// #define LT               0xB
// #define GE               0xA
// #define LE               0xD


/* Códigos de operación (opcodes) */
#define ADD_IMM_OP  0b10010001
#define ADD_EXT_OP  0b10001011001
// #define ADD_SR_OP   0b10001011  NO PUSIMOS ADD SHIFTED REGISTER NINGUNO D ESOS 
#define ADDS_EXT_OP 0b10101011001
#define ADDS_IMM_OP 0b10110001


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
}