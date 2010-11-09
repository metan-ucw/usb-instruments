CC=gcc
CFLAGS=-W -Wall -g -ggdb -I../include/
LDFLAGS=-lm
PROGRAMS=serial-test counter vameter
OBJECTS=$(PROGRAMS:=.o)

all: $(PROGRAMS) vameter_gtk

$(PROGRAMS): $(OBJECTS)
	@echo "LD   $@"
	@$(CC) $(LDFLAGS) $@.o ../lib/*.a -o $@

.c.o:
	@echo "CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

vameter_gtk.o: vameter_gtk.c
	@echo "CC   vameter_gtk.o"
	@$(CC) $(CFLAGS) -std=c99 `pkg-config --cflags gtk+-2.0` vameter_gtk.c -c -o vameter_gtk.o

vameter_gtk: vameter_gtk.o
	@echo "LD   vameter_gtk"
	@$(CC) $(CFLAGS) `pkg-config --libs gtk+-2.0` vameter_gtk.o ../lib/*.a -o vameter_gtk

clean:
	@echo CLEAN $(OBJECTS) $(PROGRAMS) vameter_gtk.o vameter_gtk
	@rm -rf $(OBJECTS) $(PROGRAMS) vameter_gtk.o vameter_gtk
