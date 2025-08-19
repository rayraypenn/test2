# Simple Makefile for ripv2d
# Build: make
# Install (optional root): make install PREFIX=/usr/local
CXX ?= g++
CXXFLAGS ?= -O2 -g -std=gnu++17 -Wall -Wextra -Wpedantic -Wno-psabi
LDFLAGS ?=
LIBS = 
PREFIX ?= /usr/local

# We need Linux-specific netlink headers and linkage (no extra libs needed)
# Add pthread for future extensions if needed
# LIBS += -lpthread

SRC = ripv2d.cpp
BIN = ripv2d

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(BIN) *.o

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/sbin/$(BIN)
	install -d $(DESTDIR)/etc
	[ -f $(DESTDIR)/etc/ripv2d.conf ] || install -m 0644 ripv2d.conf.example $(DESTDIR)/etc/ripv2d.conf || true
	@echo "Installed $(BIN) to $(DESTDIR)$(PREFIX)/sbin"

.PHONY: all clean install
