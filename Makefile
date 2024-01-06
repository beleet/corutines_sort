GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant

all: main.c
	gcc $(GCC_FLAGS) libcoro.c solution.c

clean:
	rm a.out