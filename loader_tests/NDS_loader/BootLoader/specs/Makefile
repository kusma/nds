ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

-include $(DEVKITARM)/ds_rules

all:
	@echo -n Compiling load_crt0...
	@$(CC)  -x assembler-with-cpp -marm -mthumb-interwork -c load_crt0.s -o load_crt0.o
	@echo done.
	
	@echo -n Rewriting specs file...
	@echo '*startfile:' > load.specs
	@echo -n $(shell pwd -W) >> load.specs
	@echo '/../specs/load_crt0%O%s crti%O%s crtbegin%O%s' >> load.specs
	@echo done.
	
clean:
	@echo Clean...
	rm -f load.specs load_crt0.o
	@echo done.