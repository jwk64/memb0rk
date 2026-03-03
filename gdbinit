catch syscall exit_group
catch syscall exit

break processor_step:breakpoint

define select_proc
  # break only for processor n
  condition 3 proc==((*procs) + $arg0)
end

define proc_summary
  printf "Registers:\n"
  print (*proc).main
  print (*proc).reg
  printf "Program (PC ... PC + 4):\n"
  memdump/x (*proc).reg[REG_PC] 4
  printf "Instruction:\n"
  print act
  print reg
end

define memdump
  if $argc == 3
    print $arg0 (char[$arg2])memory[$arg1 % MEM_SIZE]
  end
  if $argc == 2
    print/x (char[$arg1])memory[$arg0 % MEM_SIZE]
  end
end

define mbnext
  if $argc == 0
    continue
  end
  if $argc > 0
    continue $arg0
  end
  proc_summary
end

start


# Needs tidying, but useful:

alias psrc = p (*objs)[0].data

alias ptgt = memdump/s ((*objs)[0].target) 100
