
gcc_args       = -Wextra -Werror -g
gcc_debug_args = -fsanitize=address -DDEBUG -O0 -ggdb3

ifeq (${debug},true)
	gcc_args := ${gcc_args} ${gcc_debug_args}
endif

memb0rk.o: memb0rk.c memb0rk.h
	gcc ${gcc_args} -c memb0rk.c

main.o: main.c
	gcc ${gcc_args} -c main.c


memb0rk: memb0rk.o main.o
	gcc ${gcc_args} -o memb0rk main.o memb0rk.o

clean:
	rm memb0rk *.o
