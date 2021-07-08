all: dsh.c terminal.c network.c network.h
	gcc -o dsh dsh.c network.c -pthread
	gcc -o terminal terminal.c network.c -pthread

dsh: dsh.c
	gcc -o dsh dsh.c network.c -pthread

terminal: terminal.c
	gcc -o terminal terminal.c network.c -pthread

clean:
	rm -rf dsh terminal network.o