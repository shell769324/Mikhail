CC=g++

DEBUG=0
CFLAGS=-g -pg -O3 -Wall -DDEBUG=$(DEBUG) -std=c++17
LDFLAGS= -lm

LIBS     = -pthread

CFILES = SkipList.cpp

all: SkipList

SkipList: $(CFILES) 
	$(CC) $(CFLAGS) $(CFILES) $(LDFLAGS) $(LIBS)