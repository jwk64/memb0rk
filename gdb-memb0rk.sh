#!/bin/bash

# gdb wrapper ensuring .gdbinit is loaded

gdb --init-eval-command="add-auto-load-safe-path `dirname $0`"  --args memb0rk "$@"
