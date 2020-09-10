main: main.c timeit.h
	gcc -o $@ $^ -O3 -Wall -Werror -Wextra -Wno-unused-but-set-variable
