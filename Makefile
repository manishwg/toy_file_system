run:
	gcc -c init.c createDisk.c main.c get.c read.c write.c  entry.c
	gcc -g init.o createDisk.o main.o get.o read.o write.o entry.o -lm
	./a.out vfs.bin
clean:
	rm a.out *.o
	rm *.bin
