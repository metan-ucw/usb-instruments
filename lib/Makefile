CSOURCES=$(shell ls *.c)
OBJECTS=$(CSOURCES:.c=.o)
DEPS=$(CSOURCES:.c=.dep)
CFLAGS=-I../include/ -fPIC
LDFLAGS=
LIBNAME=usb-instruments

all: $(DEPS) $(LIBNAME).so $(LIBNAME).a

clean:
	@echo CLEAN $(OBJECTS) $(DEPS) $(LIBNAME).so $(LIBNAME).a
	@rm -rf $(OBJECTS) $(DEPS) $(LIBNAME).so $(LIBNAME).a

$(LIBNAME).so: $(OBJECTS)

-include $(DEPS)

%.dep: %.c
	@echo "DEP   $@"
	@$(CC) -MM $(CFLAGS) $< -o $@

%.so:
	@echo "LD    $@"
	@$(CC) --shared -Wl,-soname -Wl,$@.0 $(LDFLAGS) $(OBJECTS) -o $@

%.a:
	@echo "AR    $@"
	@$(AR) rcs $@ $(OBJECTS)

$(OBJECTS): %.o: %.c
	@echo "CC    $@"
	@$(CC) $(CFLAGS) -c $< -o $@
