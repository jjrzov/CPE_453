CC = gcc
CFLAGS = -Wall -Werror -g

.PHONY = all clean

all: dine

dine: dine.o print.o
	$(CC) $(CFLAGS) -o dine dine.o print.o -lpthread -lrt

dine.o: dine.c dine.h
	$(CC) $(CFLAGS) -c dine.c -o dine.o

print.o: print.c print.h
	$(CC) $(CFLAGS) -c print.c -o print.o

clean:
	rm -rf *.o dine

test:
	~pn-cs453/demos/tryAsgn3