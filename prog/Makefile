CC=gcc
CFLAGS=-W -Wall -g -ggdb -I../include/
LDFLAGS=-lm
PROGRAMS=serial-test counter vameter generator
OBJECTS=$(PROGRAMS:=.o)
GTK_PROGRAMS=vameter_gtk counter_gtk
GTK_OBJECTS=$(GTK_PROGRAMS:=.o)
GTK_OBJECTS+=gtk_common.o

all: $(PROGRAMS) $(GTK_PROGRAMS)

$(PROGRAMS): ../lib/*.a
$(GTK_PROGRAMS): ../lib/*.a gtk_common.o gtk_common.h

$(PROGRAMS): $(OBJECTS)
	@echo "LD   $@"
	@$(CC) $(LDFLAGS) $@.o ../lib/*.a -o $@

$(OBJECTS): %.o: %.c
	@echo "CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(GTK_OBJECTS): %.o: %.c
	@echo "CC   $<"
	$(CC) $(CFLAGS) -std=c99 `pkg-config --cflags gtk+-2.0` $< -c -o $@

$(GTK_PROGRAMS): $(GTK_OBJECTS)
	@echo "LD   $@"
	$(CC) $(CFLAGS) `pkg-config --libs gtk+-2.0` $@.o gtk_common.o ../lib/*.a -o $@

clean:
	@echo CLEAN $(OBJECTS) $(PROGRAMS) $(GTK_OBJECTS) $(GTK_PROGRAMS)
	@rm -rf $(OBJECTS) $(PROGRAMS) $(GTK_OBJECTS) $(GTK_PROGRAMS)
