top_srcdir = ../


sub_dirs := toolkit trs foss logserver

all clean: 
	@for subdir in $(sub_dirs); \
	do \
	    if test -d $$subdir ; \
	    then \
		echo "making $@ in $$subdir"; \
		( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	    fi; \
	done
