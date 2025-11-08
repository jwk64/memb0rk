
cargs   = -Wextra -Werror -fsanitize=address -g

memb0rk.o: memb0rk.c memb0rk.h
	gcc ${cargs} -c memb0rk.c

main.o: main.c
	gcc ${cargs} -c main.c


memb0rk: memb0rk.o main.o
	gcc ${cargs} -o memb0rk main.o memb0rk.o
