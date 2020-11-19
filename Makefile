SUBDIR := src

all: $(SUBDIR)
$(SUBDIR):
	$(MAKE) -C $@

clean:
	$(MAKE) -C $(SUBDIR) clean
	
.PHONY: all $(SUBDIR)
