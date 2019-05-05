CC=g++

DEBUG=0
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++17
LDFLAGS= -lm

LIBS     = -pthread

CFILES = test_harris.cpp

all: test_harris

test_harris: $(CFILES) 
	$(CC) $(CFLAGS) $(CFILES) $(LDFLAGS) $(LIBS)