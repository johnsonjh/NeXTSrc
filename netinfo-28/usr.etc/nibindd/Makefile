PRODUCT= nibindd
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

-include ../../lib/Makefile.optimize

CPPFLAGS= -I../../lib
WARNINGS= -W -Wall
CFLAGS= $(OPTIMIZE) $(WARNINGS) $(CPPFLAGS)
LDFLAGS= $(OPTIMIZE) $(SCATTER_LOAD)
LIBS= ../../lib/libni.a $(LIBC)
IFLAGS= -s

DERIVED= nibind_prot_svc.c

GARBAGE= *.out *~

REAL_SRCS= \
	nibind_main.c \
	nibind_prot_proc.c 

SRCS= $(REAL_SRCS) $(DERIVED)

OBJS= $(SRCS:.c=.o)

all: $(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

installsrc: $(REAL_SRCS) Makefile gmon.order
	cp $? $(SRCROOT)

$(PRODUCT).NEW: $(OBJS) vers.o
	$(CC) -o $@ $(LDFLAGS) $(OBJS) vers.o $(LIBS)

check: $(OBJS) vers.o
	unref $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

nibind_prot_svc.c: /usr/include/netinfo/nibind_prot.x
	rpcgen -m $? -o $@

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

