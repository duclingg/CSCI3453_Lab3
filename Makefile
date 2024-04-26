# Makefile for 'main' program

# Our output program name
PROG = main

# The compiler
CC = gcc.SUFFIXES : .cc
.SUFFIXES : .c
.SUFFIXES : .cpp

INCDIR =
LIBDIR =

CC = g++

CXXFLAGS = -g

LIBS = -lm

OBJS = main.o

TARGET = main

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBDIR) $(LIBS)

.cc.o:
	$(CC) -c $(CXXFLAGS) $(INCDIR) $<

.c.o:
	$(CC) -c $(CXXFLAGS) $(INCDIR) $<

.cpp.o:
	$(CC) -c $(CXXFLAGS) $(INCDIR) $<

clean:
	rm -f $(OBJS) $(TARGET) core

# END OF MAKE FILE

# Debugging flags
CFLAGS = -lpthread -o

all: main.cpp
	  $(CC) $(CFLAGS) main.o main.cpp

# Clean removes build files that are created
clean:
	rm main.o