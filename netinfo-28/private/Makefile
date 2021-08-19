PRODUCT= local.nidb
SOURCE=  local-source.nidb
BINDIR=	$(DSTROOT)/private/etc/netinfo/
DSTDIRS= $(BINDIR)

all:
	@echo -n ""

clean:
	@echo -n ""

install: DSTROOT $(DSTDIRS)
	-mkdirs $(BINDIR)/$(PRODUCT)
	cd $(SOURCE); for i in *; do \
		install -m 644 $$i $(BINDIR)/$(PRODUCT); \
	done 

installsrc:
	cp Makefile $(SRCROOT)
	cp -r $(SOURCE) $(SRCROOT)
	
DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS):
	mkdirs $@


