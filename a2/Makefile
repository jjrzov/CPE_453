# Makefile for LWP

CC = gcc

CFLAGS = -Wall -Wextra -g -fPIC

.PHONY: liblwp clean

all: liblwp

liblwp: liblwp.so liblwp.a

liblwp.a: lwp.o schedulers.o magic64.o
	ar rcs liblwp.a lwp.o schedulers.o magic64.o

liblwp.so: lwp.o schedulers.o magic64.o
	$(CC) $(CFLAGS) -shared -o liblwp.so lwp.o schedulers.o magic64.o

lwp.o: lwp.c lwp.h
	$(CC) $(CFLAGS) -c lwp.c -o lwp.o

schedulers.o: schedulers.c schedulers.h
	$(CC) $(CFLAGS) -c schedulers.c -o schedulers.o

magic64.o: magic64.S
	$(CC) $(CFLAGS) -c magic64.S -o magic64.o

clean:
	rm -rf *.o *.a *.so