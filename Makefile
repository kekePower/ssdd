# Compiler
CC = gcc

# Compiler and optimization flags
CFLAGS = `pkg-config --cflags gtk+-3.0` -O3
LDFLAGS = `pkg-config --libs gtk+-3.0`

# Source files
SRC = ssdd.c resources.c

# Output executable
TARGET = ssdd

# Resource files
RESOURCE_XML = resources.gresource.xml
RESOURCE_C = resources.c
RESOURCE_H = resources.h

# Installation directories
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/ssdd

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(RESOURCE_C) $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Compile resources
$(RESOURCE_C) $(RESOURCE_H): $(RESOURCE_XML)
	glib-compile-resources $(RESOURCE_XML) --generate-source --target=$(RESOURCE_C)
	glib-compile-resources $(RESOURCE_XML) --generate-header --target=$(RESOURCE_H)

# Install target
install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)
	install -d $(DATADIR)
	install -m 644 $(RESOURCE_XML) $(DATADIR)

# Uninstall target
uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -rf $(DATADIR)

# Clean target
clean:
	rm -f $(TARGET) $(RESOURCE_C) $(RESOURCE_H)

.PHONY: all clean install uninstall
