top_srcdir = ../../


sub_dirs := align188 cmp_file h2x truncate_file x2h

all clean: 
	@for subdir in $(sub_dirs); \
	do \
	    if test -d $$subdir ; \
	    then \
		echo "making $@ in $$subdir"; \
		( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	    fi; \
	done
