#Sample Makefile. You can make changes to this file according to your need
# The executable must be named proxy

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread -pthread

OBJS = proxyT.o csapp.o

all: proxyt

proxyt: $(OBJS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxyT.o: proxyT.c
	$(CC) $(CFLAGS) -c proxyT.c
clean:
	rm -f *~ *.o proxyt 

