# Compiler
CC = gcc

# Compiler and optimization flags
CFLAGS ?= -Wall -O2
CFLAGS += $(shell pkg-config --cflags gtk4)
CFLAGS += -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_0 -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_0
LDFLAGS ?=
LDFLAGS += $(shell pkg-config --libs gtk4)

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
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DATADIR)
	install -m 644 $(RESOURCE_XML) $(DESTDIR)$(DATADIR)

# Uninstall target
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(DATADIR)

# Clean target
clean:
	rm -f $(TARGET) $(RESOURCE_C) $(RESOURCE_H)

.PHONY: all clean install uninstall
