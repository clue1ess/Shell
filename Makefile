shell: shell.o main.o
	gcc shell.o main.o -o shell
shell.o: shell.c shell.h
	gcc -c shell.c
main.o: main.c shell.h
	gcc -c main.c

