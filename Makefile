
memb0rk.o: memb0rk.c memb0rk.h
	gcc -Wextra -Werror -g -c memb0rk.c

main.o: main.c
	gcc -Wextra -Werror -g -c main.c


memb0rk: memb0rk.o main.o
	gcc -o memb0rk main.o memb0rk.o
