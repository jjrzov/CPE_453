# Makefile for Malloc assignment

CC = gcc

CFLAGS = -Wall -g -fpic

malloc: malloc.o
	$(CC) $(CFLAGS) malloc.o -o malloc

malloc.o: malloc.c
	$(CC) $(CFLAGS) -c malloc.c

clean:
	rm -rf *.o malloc




# C = gcc
# CFLAGS = -Wall -g -fPIC

# .PHONY: malloc clean run test

# all: intel-all malloc

# # Libraries
# malloc: libmalloc.so libmalloc.a

# libmalloc.a: malloc.o
# 	ar rcs libmalloc.a malloc.o

# libmalloc.so: malloc.o
# 	$(CC) $(CFLAGS) -shared -o libmalloc.so malloc.o

# malloc.o: malloc.c intel-all
# 	$(CC) $(CFLAGS) -c malloc.c

# # Nico Code
# #intel-all: lib/libmalloc.so lib64/libmalloc.so
# intel-all: lib64/libmalloc.so

# #lib/libmalloc.so: lib malloc32.o
# #	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

# lib64/libmalloc.so: lib64 malloc64.o
# 	$(CC) $(CFLAGS) -shared -o $@ malloc64.o
                            
# lib:
# 	mkdir lib

# lib64:
# 	mkdir lib64

# #malloc32.o: malloc.c
# #	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c
# malloc64.o: malloc.c
# 	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c
# # Testing Code
# main: main.o malloc
# 	$(CC) -g -L./lib64 -o main main.o -lmalloc
# main.o: main.c
# 	$(CC) -Wall -g -c -o main.o main.c

# run: main
# 	./main

# gdb: main
# 	gdb ./main

# clean:
# 	rm -rf *.o *.a *.so main lib lib64 

# test:
# 	~pn-cs453/demos/tryAsgn1 > test.log