CC = gcc
objects = packets_delay.c

all: packets_delay

packets_delay: $(objects)
	$(CC) -o $@ $+

%.o:%.c
	$(CC) -c $+
