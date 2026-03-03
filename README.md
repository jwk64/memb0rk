# `memb0rk`

A game of adversarial programming -- like battleships, but computers.

## Quick Start

- Try `./demo.sh`

## Summary

A single memory address space shared by N processors (= "number of players"). At the beginning of
the game, each player's program is loaded to a random location in memory, with one of the processors
"pointing to" (i.e. `PC=`) it. Each player's program has a task to complete. The first program to
complete its task wins.

The objective is a memcpy, and taking inspiration from actual C memcpy, the program is provided
arguments dest, src, n. These are loaded immediately before the program, in that order.

```
void *memcpy(void dest[restrict .n], const void src[restrict .n],
                    size_t n);
```

There is a number of instruction executions after which if no player's program has completed their
task, the game is a draw. Many games are played with different (random) starting locations of the
programs to determine a win/loss/draw rate.

## ISA

### Specification

#### Registers

- "Main" (`main`): Get and put instructions get to / put from this.
- "Secondary Argument" (`op`)
- "Program Counter" (`pc`): Always holds the value of the NEXT location to be loaded/executed.
- Arithmetic result registers (`add`, `and`, `or`, `xor`, `not`, `rot`, `norm`): Always
  hold the value of the result.
- "Memory address" (`mar`)
- "Value at addresss": (`mdr`): Holds the value in memory at location addr (mod max address).
- "Constants" (`zero`, `one`)
- "General Purpose": (`g0`, `g1`, ..., `g114`)

#### Instructions

Each instruction is 1 byte, made up of an "action" (1 bit) and a register (7 bits). The "action" is
either "get" (0) or "put" (1), which do the following:

- Put: copy the value of `main` to the specified register. At least that's usually what it does --
  technically the actually behaviour is: `R ^= R^Main`. This is almost always equivalent, except in
  the case of write collisions to `mdr`, where this rule ensures fairness, as `a^b = b^a`. If R is
  read-only (e.g. `add`, `zero` etc.), then this is a no-op.
- Get: copy the value from the specified register to `main`.

## Implementation

- `memb0rk.[ch]` implements the actual machine emulator
- Different front-ends could be devised. For example, a graphical one, where the destination of the
  memcpy is taken as representing a "screen", to which players are then fighting to draw different
  images. Alternatively, an ncurses/TUI front-end, where the memcpy'd data represents text. The game
  would be shown "live" in these examples.
- `main.c` provides a simple front-end (nothing shown live, just a winner declared at the end)
- Build with `make ./memb0rk`
- Run with `./memb0rk`
- `./gdb-memb0rk.sh` takes the exact same arguments as `./memb0rk`, but wrapped by gdb, with some
  helpful aliases defined in `./gdbinit`. Use `select_proc <N>` to select a "processor"
  (i.e. player) of interest, then `continue` single steps on that processor, and `proc_summary`
  prints information on that processor etc.
- `stupid_assembler.py` provides an extremely basic assembler for writing memb0rk programs. See
  examples in ./examples.
- `assembler.rkt` (and `examples/hello-world-2.mba`) is the beginning of a better assembler written
  in racket.

## Assembly

- `R` (where R is a register name): get from R
- `>R` (where R is a register name): put to R
- `#` begins a comment
- `\1234abcd` bytes given literally as hex
- No requirement for instructions to be on separate lines. Just separated by whitespace.

### Examples (/macros?)

```
NEGATIVE:
# Make value of main negaitve, i.e. main = -main:
  not >op         # op = ~main
  one add         # main = op + 1

SUBTRACT(X, Y):
  X NEGATIVE >op    # op = -X
  Y add           # main = op + Y

IS_NEGATIVE:
# Check if main is "negative" (i.e. most significant bit is set)
  >g0          # g0 = main
  one          # main = 1
  g0 rot >op   # op = (g0 << 1) | (g0 >> 7)    <-- rotate left by 1
  one and      # main = op & 1
  norm         # main = main ? 1 : 0

BRANCH(COND, THEN, ELSE):
  zero not >op   # op = -1
  COND norm      # main = COND ? 1 : 0
  add not >op    # op = ~(main -1)       <-- i.e. COND ? 0xff : 0
  THEN and >g0   # g0 = op & THEN
  op not >op     # op = ~op              <-- i.e. COND ? 0 : 0xff
  ELSE and >op   # op = op & ELSE
  g0 or >pc      # pc = op | g0

INLINE_CONSTANT (CONST):
  N >op         # where N = 5 + number of instructions to write m ({one; lrot; lrot} = 3 -> m=8)
  pc
  add >addr
  m >b  # m is word length
  add >pc
  $const
  mdr

TWO:
# (for four, eight etc. just add more rots)
  one >op  # op = 1
  rot      # op = 1 << 1

TWENTY_THREE:
# (for example)
  SEVEN >g0
  SIXTEEN >g1   # the power of two must come last, as this does not use g0
  add

INLINE_CONSTANT(CONST):
  EIGHT >g0   # for relative location of inline value
  N >g1       # where N is the length of the inline constant
  pc
  >op         # (0) op = pc. Will number instructions relative to here from now.
              #     Note that pc points to the NEXT instruction, so it held a
              #     pointer to the ">op" when read.
  g0          # (1)
  add         # (2)
  >mar        # (3)
  >op         # (4) op = mar = op + 8   <-- i.e. pointer to inline value ahead

  g1          # (5)
  add         # (6)
  >pc         # (7) pc = op + N   <-- i.e. jump to 8+N

  \1234abcd   # (8) inline value

  mdr         # (8+N) main = mdr


LOOP:
  pc >g0                   # g0 = here

  # (body)
  # ...
  # ...
  # >g2                    # set g2 to true to continue, false to end loop

  INLINE_CONSTANT(N) >op
  pc add >g2               # g2 = here + N
  BRANCH(g1, g0, g2)

  # N just needs to be large enough to point somewhere beyond end of BRANCH code
```
