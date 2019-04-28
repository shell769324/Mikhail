CC=g++

DEBUG=0
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++17
LDFLAGS= -lm

LIBS     = -latomic -pthread

CFILES = SkipList.cpp

all: SkipList

SkipList: $(CFILES) 
	$(CC) $(CFLAGS) $(CFILES) $(LDFLAGS) $(LIBS)