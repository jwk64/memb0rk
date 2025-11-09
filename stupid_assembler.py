#!/usr/bin/env python3

# A rubbish throw-away "assembler" for initial testing
#
# Usage: stupid_assembler.py <in-file> <out-file>
#
# If "-" is given as in or out file, stdin/stdout is used respectively.
#
# TODO: stdout doesn't work because binary mode

enable_debug=False

def debug(s):
    if enable_debug:
        print(s)

import sys

reg_l = [
    'op',
    'pc',
    'mar',
    'mdr',
    'add',
    'and',
    'or',
    'xor',
    'not',
    'rot',
    'norm',
    'zero',
    'one',
]

reg = { f'g{i}' : i for i in range(115) }
for i, v in enumerate(reg_l):
    reg[v] = i+115
assert len(reg) == 128

state = 'await get'
symbol = ''

fin = open(sys.argv[1], 'r') if len(sys.argv)>1 and sys.argv[1]!='-' else sys.stdin
fout = open(sys.argv[2], 'wb') if len(sys.argv)>2 and sys.argv[2]!='-' else sys.stdout

while True:
    try:
        c = fin.read(1)
    except EOFError:
        break
    if c == '':
        break

    if state == 'comment':
        if c == '\n':
            state = 'await get'

    elif 'await' in state:
        if state == 'await get' and c == '>':
            state = 'await put'
        elif state == 'await get' and c == '\\':
            state = 'await hex'
        elif c.isalnum():
            state = state.replace('await', 'read')
            symbol = c
        elif c == '#':
            state = 'comment'
        elif c.isspace():
            pass
        else:
            assert False

    elif 'read' in state:
        if c.isalnum():
            symbol += c
        elif c.isspace() or c == '#':
            if state == 'read get':
                s = reg[symbol]
                n = 1
            elif state == 'read put':
                s = reg[symbol] | 128
                n = 1
            elif state == 'read hex':
                s = int(symbol, 16)
                debug(f'Hex: {symbol} {s}')
                assert len(symbol) %2 == 0
                n = len(symbol) // 2
            fout.write(s.to_bytes(n))
    
            symbol = ''

            if c == '#':
                state = 'comment'
            else:
                state = 'await get'
        

        
