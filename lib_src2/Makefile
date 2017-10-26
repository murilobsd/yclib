top_dir = ../

include $(top_dir)/Make.rules

sub_dirs=screen

$(EVERYTHING):
	@for subdir in $(sub_dirs); \
	do \
	    if test -d $$subdir ; \
	    then \
		echo "making $@ in $$subdir"; \
		( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	    fi; \
	done
