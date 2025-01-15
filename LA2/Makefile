# Makefile

# Compiler
CC = gcc

# Targets and their dependencies
all: run

parent: parent.c
	$(CC) -o parent parent.c

child: child.c
	$(CC) -o child child.c

dummy: dummy.c
	$(CC) -o dummy dummy.c

run: parent child dummy
	./parent 10

clean:
	rm -f parent child

