EXAMPLES = $(shell find . -mindepth 1 -maxdepth 1 -type d)

all: $(EXAMPLES)

$(EXAMPLES):
	@$(MAKE) -C $@

clean:
	@for i in $(EXAMPLES); do $(MAKE) -C $$i clean; done

.PHONY: $(EXAMPLES) clean

