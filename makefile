# Programm: Feedback Arc Set
# Author: Johannes Zottele 11911133

CC = gcc
compile_flags = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SCID_SOURCE -D_POSIX_C_SOURCE=200809L -g

library_flags = -lrt -lpthread

all: supervisor generator

supervisor: supervisor.o circularBuffer.o
	$(CC) $(compile_flags) -o $@ $^ $(library_flags)

generator: generator.o circularBuffer.o
	$(CC) $(compile_flags) -o $@ $^ $(library_flags)

circularBuffer: circularBuffer.o  
	$(CC) $(compile_flags) -o $@ $^ $(library_flags)

%.o: %.c
	$(CC) $(compile_flags) -c -o $@ $<

supervisor.o: supervisor.c circularBuffer.h
generator.o: generator.c circularBuffer.h
circularBuffer.o: circularBuffer.c circularBuffer.h

clean:
	rm -rf *.o supervisor generator circularBuffer