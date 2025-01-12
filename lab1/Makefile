# Makefile for CPE 453 Lab1

CC = gcc
CFLAGS = -Wall -g

pipeit: pipeit.o
	$(CC) $(CFLAGS) -o pipeit pipeit.o

pipeit.o: pipeit.c
	$(CC) $(CFLAGS) -c -o pipeit.o pipeit.c

clean:
	rm -f pipeit *.o
