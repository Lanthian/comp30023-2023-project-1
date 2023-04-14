CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm
EXE=allocate
OBJ=main.o proc.o ll.o

$(EXE): main.o proc.o ll.o
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LDFLAGS)

main.o: main.c ll.h
	gcc -Wall -o main.o main.c -c

proc.o: proc.c proc.h
	gcc -Wall -o proc.o proc.c -c

ll.o: ll.c ll.h
	gcc -Wall -o ll.o ll.c -c


clean:
	rm -f *.o $(EXE)
