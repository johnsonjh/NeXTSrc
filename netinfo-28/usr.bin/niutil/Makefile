PRODUCT= niutil
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

-include ../../lib/Makefile.optimize

WARNINGS= -W -Wall
CPPFLAGS= -I../../lib

CFLAGS= $(OPTIMIZE) $(WARNINGS)  $(CPPFLAGS)
LDFLAGS= $(OPTIMIZE)
LIBS= ../../lib/libni.a $(LIBC)
IFLAGS= -s

GARBAGE= *.out *~ #*#

SRCS=	niutil.c

OBJS= $(SRCS:.c=.o)

all: $(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

installsrc: $(SRCS) Makefile
	cp $? $(SRCROOT)

$(PRODUCT).NEW: $(OBJS) vers.o
	$(CC) -o $@ $(LDFLAGS) $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

$(PRODUCT):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
		$(GARBAGE)

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

depend Makedep: $(DERIVED)
	rm -f Makedep
	$(CC) -M $(CFLAGS) $(SRCS) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
