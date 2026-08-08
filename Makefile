# Compiler
CC = gcc

# Compiler and optimization flags
CFLAGS ?= -O2 -Wall -Wextra
GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LIBS := $(shell pkg-config --libs gtk4)
VERSION_FLAGS = -DGTK_VERSION_MAX_ALLOWED=GTK_VERSION_4_0 \
	-DGTK_VERSION_MIN_REQUIRED=GTK_VERSION_4_0 \
	-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_0 \
	-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_0

# Source files
SRC = ssdd.c resources.c

# Output executable
TARGET = ssdd

# Resource files
RESOURCE_XML = resources.gresource.xml
RESOURCE_C = resources.c
RESOURCE_FILES = ssdd-icon.png

# Installation directories
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/ssdd

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GTK_CFLAGS) $(VERSION_FLAGS) \
		-o $(TARGET) $(SRC) $(LDFLAGS) $(GTK_LIBS)

# Compile resources
$(RESOURCE_C): $(RESOURCE_XML) $(RESOURCE_FILES)
	glib-compile-resources $(RESOURCE_XML) --generate-source --target=$(RESOURCE_C)

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
	rm -f $(TARGET) $(RESOURCE_C)

.PHONY: all clean install uninstall
