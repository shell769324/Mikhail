CC=g++

DEBUG=0
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++17
LDFLAGS= -lm

LIBS     = -pthread

CFILES = test_mikhail.cpp

all: test_mikhail

test_mikhail: $(CFILES) 
	$(CC) $(CFLAGS) $(CFILES) $(LDFLAGS) $(LIBS)