CC = gcc
CFLAGS = -Wall -g -std=gnu99

.PHONY = all clean

all: mincommon minls minget

minls: minls.o mincommon.o
	$(CC) $(CFLAGS) $^ -o $@ 

minls.o: minls.c
	$(CC) $(CFLAGS) -c $< -o $@

minget: minget.o mincommon.o
	$(CC) $(CFLAGS) $^ -o $@ 

minget.o: minget.c
	$(CC) $(CFLAGS) -c $< -o $@

mincommon: mincommon.o
	@echo "mincommon built"

mincommon.o: mincommon.c mincommon.h
	$(CC) $(CFLAGS) -c $< -o $@


clean: 
	rm -rf *.o minls minget