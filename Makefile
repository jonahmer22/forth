CC     = gcc
CFLAGS = -Wall -Iinclude -Iarena -Icliargs -lm

SRCS = main.c           \
       src/stack.c      \
       src/token.c      \
       src/dict.c       \
       src/builtins.c   \
       arena/arena.c    \
       cliargs/cliargs.c

all:
	$(CC) $(SRCS) $(CFLAGS) -o forth

clean:
	rm forth
