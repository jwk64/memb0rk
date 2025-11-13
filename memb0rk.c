#include "memb0rk.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

char memory[MEM_SIZE];
struct processor (*procs)[];
struct objective (*objs)[];
char (*targets)[][TGT_SIZE];

int procs_len;
int objs_len;

static uint32_t mem_read(uint32_t address) {
  uint32_t ret = 0;

  for (int i=0; i<WORD_LEN; i++)
    ret |= (memory[(address + i) % MEM_SIZE] & 255) << ((WORD_LEN - 1 - i) * 8);

  return ret;
}

// (Not used during game, see mem_wrip)
void mem_write(uint32_t address, uint32_t value) {
  for (int i=0; i<WORD_LEN; i++)
    memory[(address + i) % MEM_SIZE] = value >> ((WORD_LEN - 1 - i) * 8);
}

//  I.e. "write-flip"
static void mem_wrip(uint32_t address, uint32_t value) {
  for (int i=0; i<WORD_LEN; i++) {
    /* For each objective, if currently met at this byte decrement progress
       counter */
    for (int j = 0; j < objs_len; j++) {
      if ((address + i) % MEM_SIZE < TGT_SIZE
          && memory[(address + i) % MEM_SIZE]
          == (*targets)[j][(address + i)])
        (*objs)[j].progress--;
    }
    memory[(address + i) % MEM_SIZE] ^= value >> ((WORD_LEN - 1 - i) * 8);
    /* For each objective, if now met at this byte increment progress counter */
    for (int j = 0; j < objs_len; j++) {
      if ((address + i) % MEM_SIZE < TGT_SIZE
          && memory[(address + i) % MEM_SIZE]
          == (*targets)[j][(address + i)])
        (*objs)[j].progress++;
    }
  }
}

static void processor_step(struct processor *proc) {
  char inst = memory[proc->reg[REG_PC] % MEM_SIZE];
  enum action act = inst & 128 ? ACT_PUT : ACT_GET;
  enum register_ reg = inst & 127;

 breakpoint: /* Useful point to set breakpoint in gdb */

  proc->reg[REG_PC]++;

  if (reg <= REG_MAR) {
    // normal register, just get or put normally:
    if (act == ACT_GET)
      proc->main = proc->reg[reg];
    else
      proc->reg[reg] = proc->main;
  }
  else {
    // special register:
    if (act == ACT_PUT) {
      // all are read-only except mem
      if (reg == REG_MDR)
        proc->flipping = mem_read(proc->reg[REG_MAR]) ^ proc->main;
      return;
    }
    switch (reg) {
    case REG_MDR:
      proc->main = mem_read(proc->reg[REG_MAR]);
      break;
    case REG_ADD:
      proc->main += proc->reg[REG_OP];
      break;
    case REG_AND:
      proc->main &= proc->reg[REG_OP];
      break;
    case REG_OR:
      proc->main |= proc->reg[REG_OP];
      break;
    case REG_XOR:
      proc->main ^= proc->reg[REG_OP];
      break;
    case REG_NOT:
      proc->main = ~proc->main;
      break;
    case REG_ROT:
      uint32_t rotation = proc->reg[REG_OP] % 32;
      if (rotation)
        proc->main = (proc->main << rotation) | (proc->main >> (32-rotation));
      break;
    case REG_NORM:
      proc->main = proc->main ? 1 : 0;
      break;
    case REG_ZERO:
      proc->main = 0;
      break;
    case REG_ONE:
      proc->main = 1;
      break;
    }
  }
}

// Returns index in *objs of winner, or -1 if no winner yet
static int game_step() {
  for (int i = 0; i < procs_len; i++)
    processor_step(&(*procs)[i]);

  // Process memory writes:
  for (int i = 0; i < procs_len; i++)
    if ((*procs)[i].flipping) {
      mem_wrip((*procs)[i].reg[REG_MAR], (*procs)[i].flipping);
      (*procs)[i].flipping = 0;
    }

  // Check for completed objectives, i.e. a winner:
  for (int j = 0; j < objs_len; j++)
    if ((*objs)[j].progress == TGT_SIZE)
      /* objective j acheived. We assume that objectives are contradictory, so
         no others will be acheived, so we return j. */
      return j;

  return -1;
}

int game(long long max_steps) {
  int result;

  for (long long i=0; i<max_steps; i++)
    if ((result = game_step()) != -1)
      break;

  return result;
}
