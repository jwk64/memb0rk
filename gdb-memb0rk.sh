#!/bin/bash

# gdb wrapper ensuring .gdbinit is loaded

gdb --command='./gdbinit' --args memb0rk "$@"
