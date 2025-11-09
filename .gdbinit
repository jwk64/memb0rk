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
    print $arg0 (char[$arg2])(*memory)[$arg1 % mem_size]
  end
  if $argc == 2
    print/x (char[$arg1])(*memory)[$arg0 % mem_size]
  end
end

document memdump
Usage: memdump i [n] [z|t|c]
end
  
start
