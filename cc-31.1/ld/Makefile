TARGET_PAGESIZE = 8192
CFLAGS = -g -O -Wall -DTARGET_PAGESIZE=$(TARGET_PAGESIZE) -I.. -I../../include
IFLAGS = -s
BINDIR = /bin

HFILES = ld.h pass1.h objects.h sections.h cstring_literals.h symbols.h \
	 fvmlibs.h layout.h specs.h pass2.h generic_reloc.h  hash_string.h \
	 sets.h 4byte_literals.h 8byte_literals.h literal_pointers.h
CFILES = ld.c pass1.c objects.c sections.c cstring_literals.c symbols.c \
	 fvmlibs.c layout.c specs.c pass2.c generic_reloc.c rld.c sets.c \
	 ld_vers.c 4byte_literals.c 8byte_literals.c literal_pointers.c

SRCS = $(HFILES) $(CFILES)
OBJS = $(CFILES:.c=.o)

GARBAGE = tags make.out

all:	ld_build profile_ofiles

ld_build:	ld_dir
	cd ld_dir;      $(MAKE) -f ../Makefile VPATH=.. DIR=ld_dir ld \
	COPTS="-DDEBUG -fomit-frame-pointer -finline-functions"

ld_g_build:	ld_g_dir
	cd ld_g_dir;    $(MAKE) -f ../Makefile VPATH=.. DIR=ld_g_dir ld \
	COPTS="-g -DDEBUG"

ld_p_build:	ld_p_dir
	cd ld_p_dir;    $(MAKE) -f ../Makefile VPATH=.. DIR=ld_p_dir ld \
	COPTS="-pg -DDEBUG"

shlib_ofiles:	shlib_obj
	cd shlib_obj;   $(MAKE) -f ../Makefile VPATH=.. DIR=shlib_obj ofiles \
	COPTS="-DRLD -DSHLIB -I$(SHLIBIDIR)"

profile_ofiles:	profile_obj
	cd profile_obj; $(MAKE) -f ../Makefile VPATH=.. DIR=profile_obj ofiles \
	COPTS="-DRLD -pg"

ld:	$(OBJS)
	$(CC) $(COPTS) $(CFLAGS) -o $@ $(OBJS) -lsys_s

ofiles:	$(OBJS)

ld_vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c ld > $@

.c.o:
	$(CC) -MD $(COPTS) $(CFLAGS) -c -o $@ $<
	md -d -u Makedep $*.d

clean:	ALWAYS
	-rm -r -f ld_dir ld_g_dir ld_p_dir shlib_obj profile_obj
	-rm -r -f rld_obj librld_s.a librld_s.Z.shlib
	-rm -f $(GARBAGE)

install: $(DSTROOT)$(BINDIR) all
	install $(IFLAGS) -m 555 ld_dir/ld $(DSTROOT)$(BINDIR)/ld

librld: rld_ofiles shlib

rld_ofiles:	rld_obj
	cd rld_obj;   $(MAKE) -f ../Makefile VPATH=.. DIR=rld_obj ofiles \
	pointers.o alias.o COPTS="-DRLD -DSHLIB"

shlib:
	cd rld_obj; mkshlib $(MKSHLIBFLAGS) -s ../spec_rld \
	-t ../librld_s.Z.shlib -h ../librld_s.a \
	-minor_version `vers_string -n` -seglinkedit \
	-segprot __LINKEDIT rwx r

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTROOT)$(BINDIR) ld_dir ld_g_dir ld_p_dir shlib_obj profile_obj rld_obj:
	mkdirs $@

tags:	ALWAYS
	ctags $(SRCS)

ALWAYS:

-include $(DIR)/Makedep
