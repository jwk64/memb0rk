#include <stdint.h>

/* Total memory size: */
#define MEM_SIZE (16 * 1024 * 1024) // = 16M
/* Size of memcpy target for each player: */
#define TGT_SIZE 1024
#define WORD_LEN 4

struct processor {
  uint32_t main;
  uint32_t flipping;
  uint32_t reg[128];
};

struct objective {
  uint32_t src;
  uint32_t progress;
};

extern char memory[MEM_SIZE];
extern struct processor (*procs)[];
extern struct objective (*objs)[];
extern char (*targets)[][TGT_SIZE];

extern int procs_len;
extern int objs_len;

extern void mem_write(uint32_t address, uint32_t value);
extern int game(long long max_steps);

enum action {
  ACT_GET,
  ACT_PUT,
};

enum register_ {
  // ===== True registers: =====

  // (no enums provided for general purpose registers, except final one)
  REG_MAX_GP = 114, // "Maximum general purpose register"

  REG_OP, // "(other) operand"
  REG_PC,  // Program Counter
  REG_MAR, // Memory Address Register

  // ===== Pseudo-registers: =====
  // Writable:
  REG_MDR, // Memory Data Register

  // Read only, ALU results:
  REG_ADD,
  REG_AND,
  REG_OR,
  REG_XOR,
  REG_NOT,
  REG_ROT,  // "rotate" (left)
  REG_NORM, // "normalised", i.e. x ? 1 : 0

  // Read only, constant:
  REG_ZERO,
  REG_ONE,

  // =====
  REG_TOTAL,
};

_Static_assert(REG_TOTAL == 128);
