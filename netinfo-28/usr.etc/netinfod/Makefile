PRODUCT= netinfod
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

-include ../../lib/Makefile.optimize

LIBCHECK= libcheck
WARNINGS= -W -Wall
CPPFLAGS= -I../../lib
CPPDEFS= 
CFLAGS= $(OPTIMIZE) $(WARNINGS)  $(CPPFLAGS) $(CPPDEFS)
LDFLAGS= $(OPTIMIZE) $(SCATTER_LOAD)
LIBS=  $(LIBCHECK).a $(LIBC) ../../lib/libni.a
IFLAGS= -s

DERIVED= ni_prot_svc.c

GARBAGE= *.out *~

HDRS=	checksum.h \
	event.h \
	getstuff.h \
	ni_dir.h \
	ni_file.h \
	ni_globals.h \
	ni_impl.h \
	ni_serial.h \
	ni_server.h \
	notify.h \
	safe_stdio.h \
	alert.h \
	strstore.h \
	index.h \
	index_manager.h  \
	ranstrcmp.h \
	listops.h

REAL_SRCS= \
	svc_tcp.c \
	ni_prot_proc.c \
	ni.c \
	ni_impl.c \
	ni_file.c \
	ni_serial.c \
	ni_dir.c \
	ni_globals.c \
	ni_lookupprop.c \
	ni_main.c \
	svc_run.c \
	readall.c \
	notify.c \
	getstuff.c \
	checksum.c \
	event.c \
	safe_stdio.c \
	alert.c \
	mcast_args.c \
	index.c \
	index_manager.c \
	strstore.c \
	ranstrcmp.c \
	listops.c


SRCS=	$(REAL_SRCS) $(DERIVED) 

OBJS= $(SRCS:.c=.o)

all: $(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

installsrc: $(REAL_SRCS) $(HDRS) $(LIBCHECK).c Makefile rpcgenfix.sed gmon.order
	cp $? $(SRCROOT)

$(LIBCHECK).a: $(LIBCHECK).o
	ar cru $@ $?
	ranlib $@

$(PRODUCT).NEW: $(LIBCHECK).a $(OBJS) vers.o
	$(CC) -o $@ $(LDFLAGS) $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

ni_prot_svc.c-orig: /usr/include/netinfo/ni_prot.x
	rpcgen -m $? -o $@

ni_prot_svc.c: ni_prot_svc.c-orig rpcgenfix.sed
	sed  -f rpcgenfix.sed ni_prot_svc.c-orig > $@

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

#
# No warnings: it can't handle it + add NeXT_MOD
#
svc_tcp.o: svc_tcp.c
	$(CC) -DNeXT_MOD $(OPTIMIZE) $(CPPFLAGS) -c $*.c -o $@

-include Makedep
