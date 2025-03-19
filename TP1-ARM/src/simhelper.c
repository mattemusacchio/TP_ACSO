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
#define ADDS_IMM_OPCODE  0x91000000
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

/* Códigos de condición para B.cond */
#define EQ               0x0
#define NE               0x1
#define GT               0xC
#define LT               0xB
#define GE               0xA
#define LE               0xD

/* 
 * Función para actualizar las flags de acuerdo al resultado
 * FLAG_N: Negativo (bit más significativo = 1)
 * FLAG_Z: Cero (resultado = 0)
 */
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
    
    /* Incrementar el PC para apuntar a la siguiente instrucción por defecto
     * Instrucciones de salto pueden sobreescribir este valor
     */
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    
    /* Hacer una copia del estado actual para modificar */
    memcpy(&NEXT_STATE, &CURRENT_STATE, sizeof(CPU_State));
    
    /* Decodificación de la instrucción basada en el opcode */
    
    /* BR Xn - Salto a dirección en registro */
    if ((instruction & 0xFFE0FC00) == 0xD61F0000) {
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
    }
    /* B.cond - Salto condicional */
    else if ((instruction & 0xFE000000) == 0x54000000) {
        imm = ((instruction & IMM19_MASK) >> IMM19_SHIFT);
        
        /* Signo extendido para imm19 */
        if (imm & 0x40000) {
            imm |= 0xFFFFFFFFFFF80000;
        }
        
        /* Obtener la condición */
        uint32_t cond = instruction & 0xF;
        
        /* Evaluar la condición */
        int take_branch = 0;
        
        switch (cond) {
            case EQ: /* Equal */
                take_branch = CURRENT_STATE.FLAG_Z == 1;
                break;
            case NE: /* Not Equal */
                take_branch = CURRENT_STATE.FLAG_Z == 0;
                break;
            case GT: /* Greater Than (signed) */
                take_branch = (CURRENT_STATE.FLAG_Z == 0) && (CURRENT_STATE.FLAG_N == 0);
                break;
            case LT: /* Less Than (signed) */
                take_branch = CURRENT_STATE.FLAG_N == 1;
                break;
            case GE: /* Greater Than or Equal (signed) */
                take_branch = CURRENT_STATE.FLAG_N == 0;
                break;
            case LE: /* Less Than or Equal (signed) */
                take_branch = (CURRENT_STATE.FLAG_Z == 1) || (CURRENT_STATE.FLAG_N == 1);
                break;
        }
        
        if (take_branch) {
            NEXT_STATE.PC = CURRENT_STATE.PC + (imm << 2);
        }
    }
    /* B - Salto incondicional */
    else if ((instruction & 0xFC000000) == 0x14000000) {
        imm = instruction & IMM26_MASK;
        
        /* Signo extendido para imm26 */
        if (imm & 0x2000000) {
            imm |= 0xFFFFFFFFFC000000;
        }
        
        NEXT_STATE.PC = CURRENT_STATE.PC + (imm << 2);
    }
    /* CBZ - Compare and Branch if Zero */
    else if ((instruction & 0xFC000000) == 0xB4000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        imm = ((instruction & IMM19_MASK) >> IMM19_SHIFT);
        
        /* Signo extendido para imm19 */
        if (imm & 0x40000) {
            imm |= 0xFFFFFFFFFFF80000;
        }
        
        if (CURRENT_STATE.REGS[rd] == 0) {
            NEXT_STATE.PC = CURRENT_STATE.PC + (imm << 2);
        }
    }
    /* CBNZ - Compare and Branch if Not Zero */
    else if ((instruction & 0xFC000000) == 0xB5000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        imm = ((instruction & IMM19_MASK) >> IMM19_SHIFT);
        
        /* Signo extendido para imm19 */
        if (imm & 0x40000) {
            imm |= 0xFFFFFFFFFFF80000;
        }
        
        if (CURRENT_STATE.REGS[rd] != 0) {
            NEXT_STATE.PC = CURRENT_STATE.PC + (imm << 2);
        }
    }
    /* MOVZ - Move Immediate with Zero */
    else if ((instruction & 0xFF800000) == 0xD2800000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        imm = (instruction & 0xFFFF0000) >> 16;
        
        /* Solo implementamos para hw=0 (sin shift) */
        NEXT_STATE.REGS[rd] = imm;
    }
    /* ORR - Bitwise OR */
    else if ((instruction & 0xFFE0FC00) == 0xAA000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] | CURRENT_STATE.REGS[rm];
    }
    /* EOR - Bitwise XOR */
    else if ((instruction & 0xFFE0FC00) == 0xCA000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] ^ CURRENT_STATE.REGS[rm];
    }
    /* ANDS - Bitwise AND and set flags */
    else if ((instruction & 0xFFE0FC00) == 0xEA000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] & CURRENT_STATE.REGS[rm];
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* ADDS (Extended Register) */
    else if ((instruction & 0xFFE0FC00) == 0xAB000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + CURRENT_STATE.REGS[rm];
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* ADDS (Immediate) - Common case shift 00 */
    else if ((instruction & 0xFF200000) == 0x91000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = (instruction & IMM12_MASK) >> IMM12_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm;
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* ADDS (Immediate) - Case shift 01 (LSL 12) */
    else if ((instruction & 0xFF200000) == 0x91400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = ((instruction & IMM12_MASK) >> IMM12_SHIFT) << 12;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm;
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* SUBS (Extended Register) */
    else if ((instruction & 0xFFE0FC00) == 0xEB000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* SUBS (Immediate) - Common case shift 00 */
    else if ((instruction & 0xFF200000) == 0xD1000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = (instruction & IMM12_MASK) >> IMM12_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] - imm;
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* SUBS (Immediate) - Case shift 01 (LSL 12) */
    else if ((instruction & 0xFF200000) == 0xD1400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = ((instruction & IMM12_MASK) >> IMM12_SHIFT) << 12;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] - imm;
        update_flags(NEXT_STATE.REGS[rd]);
    }
    /* CMP (Extended Register) (SUBS con XZR como destino) */
    else if ((instruction & 0xFFE0FC1F) == 0xEB00001F) {
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        int64_t result = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
        update_flags(result);
    }
    /* CMP (Immediate) (SUBS con XZR como destino) */
    else if ((instruction & 0xFF20001F) == 0xF100001F) {
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = (instruction & IMM12_MASK) >> IMM12_SHIFT;
        
        int64_t result = CURRENT_STATE.REGS[rn] - imm;
        update_flags(result);
    }
    /* MUL - Multiplication */
    else if ((instruction & 0x7FE0FFE0) == 0x1B007C00) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] * CURRENT_STATE.REGS[rm];
    }
    /* ADD (Extended Register) */
    else if ((instruction & 0xFFE0FC00) == 0x8B000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        rm = (instruction & RM_MASK) >> RM_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + CURRENT_STATE.REGS[rm];
    }
    /* ADD (Immediate) - Common case shift 00 */
    else if ((instruction & 0xFF200000) == 0x91000000 && (instruction & RD_MASK) != 0x1F) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = (instruction & IMM12_MASK) >> IMM12_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm;
    }
    /* ADD (Immediate) - Case shift 01 (LSL 12) */
    else if ((instruction & 0xFF200000) == 0x91400000 && (instruction & RD_MASK) != 0x1F) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        imm = ((instruction & IMM12_MASK) >> IMM12_SHIFT) << 12;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm;
    }
    /* LSL (Immediate) - Logical Shift Left */
    else if ((instruction & 0xFFE0FC00) == 0xD3400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        shamt = (instruction & SHAMTW_MASK) >> SHAMTW_SHIFT;
        
        NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] << shamt;
    }
    /* LSR (Immediate) - Logical Shift Right */
    else if ((instruction & 0xFFE0FC00) == 0xD3400000 && (instruction & 0x400000)) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        shamt = (instruction & SHAMTW_MASK) >> SHAMTW_SHIFT;
        
        NEXT_STATE.REGS[rd] = (uint64_t)CURRENT_STATE.REGS[rn] >> shamt;
    }
    /* STUR - Store Register (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0xF8000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        /* Almacenando 64 bits (8 bytes) en memoria */
        mem_write_32(addr, (uint32_t)CURRENT_STATE.REGS[rd]);
        mem_write_32(addr + 4, (uint32_t)(CURRENT_STATE.REGS[rd] >> 32));
    }
    /* STURB - Store Register Byte (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0x38000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        uint32_t word = mem_read_32(addr & ~0x3);  /* Leemos palabra alineada */
        uint32_t byte_offset = addr & 0x3;         /* Calcular desplazamiento del byte */
        uint32_t byte_mask = 0xFF << (byte_offset * 8);  /* Máscara para el byte */
        uint8_t byte_value = (CURRENT_STATE.REGS[rd] & 0xFF);
        
        /* Reemplazar el byte y conservar el resto */
        word = (word & ~byte_mask) | (byte_value << (byte_offset * 8));
        mem_write_32(addr & ~0x3, word);
    }
    /* STURH - Store Register Halfword (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0x78000000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        uint32_t word = mem_read_32(addr & ~0x3);  /* Leemos palabra alineada */
        uint32_t hw_offset = (addr & 0x3) >> 1;    /* Calcular desplazamiento del halfword */
        uint32_t hw_mask = 0xFFFF << (hw_offset * 16);  /* Máscara para el halfword */
        uint16_t hw_value = (CURRENT_STATE.REGS[rd] & 0xFFFF);
        
        /* Reemplazar el halfword y conservar el resto */
        word = (word & ~hw_mask) | (hw_value << (hw_offset * 16));
        mem_write_32(addr & ~0x3, word);
    }
    /* LDUR - Load Register (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0xF8400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        /* Cargando 64 bits (8 bytes) de memoria */
        uint64_t value = (uint64_t)mem_read_32(addr) | ((uint64_t)mem_read_32(addr + 4) << 32);
        NEXT_STATE.REGS[rd] = value;
    }
    /* LDURB - Load Register Byte (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0x38400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        uint32_t word = mem_read_32(addr & ~0x3);  /* Leemos palabra alineada */
        uint32_t byte_offset = addr & 0x3;         /* Calcular desplazamiento del byte */
        uint8_t byte_value = (word >> (byte_offset * 8)) & 0xFF;
        
        /* X1 = 56'b0, byte */
        NEXT_STATE.REGS[rd] = byte_value;  /* Ceros extendidos (56'b0, byte) */
    }
    /* LDURH - Load Register Halfword (Unscaled) */
    else if ((instruction & 0xFFC00000) == 0x78400000) {
        rd = (instruction & RD_MASK) >> RD_SHIFT;
        rn = (instruction & RN_MASK) >> RN_SHIFT;
        offset = (instruction & IMM9_MASK) >> IMM9_SHIFT;
        
        /* Signo extendido para imm9 */
        if (offset & 0x100) {
            offset |= 0xFFFFFFFFFFFFFE00;
        }
        
        addr = CURRENT_STATE.REGS[rn] + offset;
        uint32_t word = mem_read_32(addr & ~0x3);  /* Leemos palabra alineada */
        uint32_t hw_offset = (addr & 0x3) >> 1;    /* Calcular desplazamiento del halfword */
        uint16_t hw_value = (word >> (hw_offset * 16)) & 0xFFFF;
        
        /* X1 = 48'b0, halfword */
        NEXT_STATE.REGS[rd] = hw_value;  /* Ceros extendidos (48'b0, halfword) */
    }
    /* HLT - Detener la simulación */
    else if (instruction == 0x6A2) {
        RUN_BIT = 0;
    }
}
