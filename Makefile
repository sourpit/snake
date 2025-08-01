.PHONY: all clean

all: clean snake

snake:
	gcc -g -Wall -pedantic -o snake snake.c

clean:
	rm -f snake
