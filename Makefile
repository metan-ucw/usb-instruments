SUBDIRS=lib prog

all: $(SUBDIRS)
clean: $(SUBDIRS)

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	@echo DIR $@
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS)
