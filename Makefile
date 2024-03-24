# Makefile for compiling Trivial-Polymorphism-3.0.c

# Compiler settings - Can be customized.
CC = gcc
CFLAGS = 

# Name of the output file
TARGET = TrivialPolymorphism

# Source files
SOURCES = Trivial-Polymorphism-3.0.c
OBJECTS = $(SOURCES:.c=.o)

# Build target
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# To start over from scratch, type 'make clean'.
# This removes the executable file, as well as old .o object
# files and *~ backup files:
clear:
	rm -f $(TARGET) $(OBJECTS) *~
	rm -f *.mutex

# Dependencies
$(OBJECTS): $(SOURCES)

# Phony targets
.PHONY: clean
