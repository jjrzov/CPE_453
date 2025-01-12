# Makefile for Malloc assignment

CC = gcc
CFLAGS = -Wall -g

heap: heap.o
	$(CC) $(CFLAGS) -o heap heap.o

heap.o: heap.c
	$(CC) $(CFLAGS) -c -o heap.o heap.c

clean:
	rm -f heap *.o