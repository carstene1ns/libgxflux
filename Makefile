lib:
	@$(MAKE) -C src

all: examples

install:
	@$(MAKE) -C src install-all

examples: install
	@$(MAKE) -C examples

clean:
	@$(MAKE) -C src clean
	@$(MAKE) -C examples clean

.PHONY: lib examples install clean
