all:
	gcc main.c ./arena/arena.c -Iarena -lm -Wall -o lang

clean:
	rm langs
