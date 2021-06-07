all: examples

DIRS=\
	hello_world\
	example.001\
	example.002\
	example.003\
	example.004\
	example.005\
	example.006\
	example.007

examples:
	@for dir in $(DIRS) ; do \
		(cd $$dir; make $(MAKECMDGOALS)); \
	done

clean:
	@for dir in $(DIRS) ; do \
		(cd $$dir; make $(MAKECMDGOALS)); \
	done

distclean:
	@find -name "*.o" -exec rm '{}' \;
	@find -name "*~" -exec rm '{}' \;
	@find -name "*.xmc.c" -exec rm '{}' \;
	@find -name "*.bin" -exec rm '{}' \;
	@find -name "partition?" -exec rm '{}' \;
	@find -name "xm_cf" -exec rm '{}' \;
	@find -name "resident_sw" -exec rm '{}' \;
	@find -name "dep.mk" -exec rm '{}' \;
