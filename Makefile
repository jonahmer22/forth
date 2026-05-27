all:
	gcc main.c ./src/*.c -Iinclude -Wpedantic -Wall -o lang

clean:
	rm langs
