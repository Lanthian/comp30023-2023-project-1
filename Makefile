EXE=allocate

$(EXE): main.o proc.o ll.o
	gcc -Wall -o $(EXE) main.o proc.o ll.o

main.o: main.c ll.h
	gcc -Wall -o main.o main.c -c

proc.o: proc.c proc.h
	gcc -Wall -o proc.o proc.c -c

ll.o: ll.c ll.h
	gcc -Wall -o ll.o ll.c -c

format:
	clang-format -style=file -i *.c


clean:
	rm *.o