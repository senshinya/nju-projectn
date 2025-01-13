/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

typedef enum {
	CSR_MSTATUS = 0x300,
	CSR_MTVEC   = 0x305,
	CSR_MEPC    = 0x341,
	CSR_MCAUSE  = 0x342,
} csr_id;

static inline word_t get_csr_val_by_id(int csr_id) {
  switch (csr_id)
  {
  case CSR_MSTATUS:
    return cpu.csrs.mstatus;
  case CSR_MTVEC:
    return cpu.csrs.mtvec;
  case CSR_MEPC:
    return cpu.csrs.mepc;
  case CSR_MCAUSE:
    return cpu.csrs.mcause;
  default:
    panic("unsupported csr id %.8x\n", csr_id);
  }
}

static inline void set_csr_val_by_id(int csr_id, word_t val) {
  switch (csr_id)
  {
  case CSR_MSTATUS:
    cpu.csrs.mstatus = val;
    break;
  case CSR_MTVEC:
    cpu.csrs.mtvec = val;
    break;
  case CSR_MEPC:
    cpu.csrs.mepc = val;
    break;
  case CSR_MCAUSE:
    cpu.csrs.mcause = val;
    break;
  default:
    panic("unsupported csr id %.8x\n", csr_id);
  }
}
 
#define read_csr(idx) (get_csr_val_by_id(idx)) 
#define write_csr(idx, val) (set_csr_val_by_id(idx, val))

#endif
