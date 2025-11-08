# `memb0rk`

A game of adversarial programming -- like battleships, but computers.

## Summary

A single memory address space shared by N processors (= "number of players"). At the beginning of
the game, each player's program is loaded to a random location in memory, with one of the processors
"pointing to" (i.e. `PC=`) it. Each player's program has a task to complete. The first program to
complete its task wins.

The players' tasks may be in direct conflict with each other -- e.g. a memcpy of different data to
the same location.

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

### Requirements

- No "halt" instruction -- too easy
- No crashing on invalid instructions -- unrecognised instructions are treated as no-op. `Goto <out
  of range>` should be valid -- mod is the obvious solution.
- Deterministic execution -- the only randomness in games is in the initial state
- Fair, i.e. no processor has any sort of "priority". Consider write collision, i.e. both processors
  try to write different value simultaneously to the same address. The most cool option would be
  that there is no "set" instruction, only "bitwise flip". To "set" (and the assembler should have a
  macro for this), one should first read, then XOR with the intended value to set, then bitwise flip
  the result to the address. Seems like the most elegant option.
- There should be a standard way to pass parameters to programs.
- The ISA should not be tied to a particular total memory size, perhaps not even to a word size.

### Brain-storming

#### Registers

- "Main": a. Get and put instructions get to / put from this.
- "Secondary Argument": b
- "Program Counter": pc. Always holds the value of the NEXT location to be loaded/executed.
- Arithmetic result registers: add, and, or, not, xor, rshift, lshift, fill. Always holds the value of the computation result.
- "Address": addr
- "Value at addresss": val. Holds the value in memory at location addr (mod max address).
- "Constants": zero, one
- "General Purpose": r1, r2, ..., rN

#### Instructions

- `put <reg>` -- copy value of y to reg. Technically we actually do: `<reg> ^= <reg> ^ a`. This is
  almost always equivalent, except in the case of write collisions to val, where this rule ensures
  fairness, as `a^b = b^a`
- `get <reg>` -- copy value of reg to y

#### Macros

- `INST >REG  =  INST; put REG` 
- `write <rval> <raddr> =  get <raddr> >x1; read >x2; get <rval> >x1; xor >x2; get <raddr> >x1; wrip`
- `negative-x2  =  zero >x1; not >x1; xor >x2; one >x1; add`
- `negative  =  get x1 >x2; negative-x2`
- `subtract    =  get x1 >rA; negative-x2 >x2; get rA >x1; add`
- `two  =  one >x1; lshift`, four, eight, etc. (note these do not write to x2)
- `twenty-three = seven >x2; sixteen >x1; add`, etc. (works only in this order, power of two last,
  to avoid recursive macros writing over registers
- `seven >rA; `

```
[invert]
get not
put b
get one
get add

[subtract (1, 2)]
get $1
.invert
put b
get $2
get add

# shorthand: "get" omitted, ">" instead of "put", and this may append to previous line.

[is_negative]
lrot >b
one
and
fill

[branch (cond, then, else, tmp)]
cond >b
$then
and >$tmp
b
not >b
$else
and >b
$tmp
or >pc

[inline_constant (const)]
n >b  # n is 5 + number of instructions to write m ({one; lrot; lrot} = 3 -> m=8)
pc
add >addr
m >b  # m is word length
add >pc
$const
val

[loop (cond, len)]
get pc
>$start
(body)
...
... # populate $cond
.inline_constant (N)
>b
pc
add >$end
.branch $cond $start $end $start


```
