PRODUCT= niload
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

-include ../../lib/Makefile.optimize

WARNINGS= -W -Wall
CPPFLAGS= -I../../lib

CFLAGS= $(OPTIMIZE) $(WARNINGS)  $(CPPFLAGS)
LDFLAGS= $(OPTIMIZE)
LIBS= $(LIBC) ../../lib/libni.a

IFLAGS= -s

GARBAGE= *.out *~

SRCS=	load_aliases.c \
	load_bootp.c \
	load_bootparams.c \
	load_groups.c \
	load_machines.c \
	load_mounts.c \
	load_networks.c \
	load_printers.c \
	load_protocols.c \
	load_rpcs.c \
	load_services.c \
	load_unix.c \
	load_users.c

OBJS= $(SRCS:.c=.o)

all: $(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

installsrc: $(SRCS) Makefile
	cp $? $(SRCROOT)

$(PRODUCT).NEW: $(OBJS) vers.o
	$(CC) -o $@ $(CFLAGS) $(OBJS) vers.o $(LIBS)

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
