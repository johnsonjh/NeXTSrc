# This makefile may be used to make either the VAX or the 68020 assembler

# This makefile must be changed to to compile the 68020 assembler on
# the HPUX machines.  See the instructions around line 23.  (They should
# be clearly marked with HPUX.)

# To make the 68020 assembler compile as the default, un-comment the next
# four lines, and comment out the four lines below the next (text) comment
all: as.NEW

BINDIR=/bin
IFLAGS = -s

install: ${DSTROOT}${BINDIR} all
	install ${IFLAGS} -m 555 as.NEW ${DSTROOT}${BINDIR}/as

${DSTROOT}${BINDIR}:; mkdirs $@

local: LCLROOT ${LCLROOT}${BINDIR} all
	install ${IFLAGS} -m 555 as.NEW ${LCLROOT}${BINDIR}/as

${LCLROOT}${BINDIR}:; mkdirs $@

a:	as.NEW 
	@rm -f a
	@ln as.NEW a

.depend: 
	-ln -s m-next.h m68k.h
	touch .depend

lint:	lint_68k
# To make the VAX assembler compile as the default, un-comment the next four
# lines, and comment out the four lines above this comment
#a:	avax
#	@rm -f a
#	@ln avax a
#lint:	lint_vax

# NOTE:  m68k.h should be a symlink to either m-sun3.h , m-hpux.h or m-generic.h
# depending on which machine you want to compile the 68020 assembler for.

# To compile for HPUX, symlink m-hpux.h to m68k.h , un-comment the
# next two lines, and comment out the CFLAGS line seven lines (or so) down.
# CFLAGS = -g -I../hp-include -I.
# LOADLIBES = -lGNU

# to include the mc68851 mmu coprocessor instructions un-comment
# the next line, and comment out the following line.  (68020 only)
# CFLAGS = -g -I. -Dm68851
CFLAGS= -bsd -g -O -I../include -Dm68851 -Derror=as_fatal -I. -DCROSS -DMach_O
LDFLAGS = $(CFLAGS)

x =\
as.o		xrealloc.o	xmalloc.o	hash.o		hex-value.o\
atof-generic.o	append.o	messages.o	expr.o		app.o\
frags.o		input-file.o	input-scrub.o	output-file.o	read.o\
subsegs.o	symbols.o	write.o		version.o\
flonum-const.o	flonum-copy.o	flonum-mult.o	strstr.o	bignum-copy.o \
gdb.o		gdb-file.o	gdb-symbols.o	gdb-blocks.o	obstack.o\
gdb-lines.o	atom.o		Mach-O.o



# files that aren't used       flonum-normal.o	flonum-print.o   cpystr.o
y =\
m68k.o		atof-m68k.o	atof-m68k-assist.o
z = \
vax.o		atof-vax.o	

as.NEW: .depend $x $y 
	$(CC) -o as.NEW $(CFLAGS) $x m68k.o $(LOADLIBES)

avax:	$x $z
	$(CC) -o avax $(LDFLAGS) $x $z $(LOADLIBES)


m68k.o:		a.out.h as.h expr.h flonum.h frags.h hash.h
m68k.o:		m68k-opcode.h m68k.h md.h obstack.h struc-symbol.h pmmu.h

app.o:		as.h

as.o:		a.out.h as.h read.h struc-symbol.h write.h
atof-generic.o:	flonum.h
atof-m68k.o:	flonum.h
atof-vax.o:	as.h flonum.h read.h
bignum-copy.o:	bignum.h
expr.o:		a.out.h as.h expr.h flonum.h obstack.h read.h struc-symbol.h
expr.o:		 symbols.h
flonum-const.o:	flonum.h
flonum-copy.o:	flonum.h
flonum-mult.o:	flonum.h
flonum-normal.o:flonum.h
flonum-print.o:	flonum.h
frags.o:	a.out.h as.h frags.h obstack.h struc-symbol.h subsegs.h
gdb.o:		as.h
gdb-blocks.o:	as.h
gdb-lines.o:	as.h frags.h obstack.h
gdb-symbols.o:	a.out.h as.h struc-symbol.h
hash.o:		hash.h
input-file.o:	input-file.h
input-scrub.o:	as.h input-file.h read.h
messages.o:	as.h
obstack.o:	obstack.h
read.o:		a.out.h as.h expr.h flonum.h frags.h hash.h md.h obstack.h
read.o:		read.h struc-symbol.h symbols.h
subsegs.o:	a.out.h as.h frags.h obstack.h struc-symbol.h subsegs.h write.h
symbols.o:	a.out.h as.h frags.h hash.h obstack.h struc-symbol.h symbols.h
vax.o:		a.out.h as.h expr.h flonum.h frags.h md.h obstack.h read.h
vax.o:		struc-symbol.h symbols.h vax-inst.h vax-opcode.h
write.o:	a.out.h as.h md.h obstack.h struc-symbol.h subsegs.h
write.o:	symbols.h write.h

flonum.h:					bignum.h

# app is no longer a separate process.
# app:	app.c
#	$(CC) -o app app.c
clean:
	rm -f avax as.NEW $x $y $z a core gmon.out
	rm -f .depend

oprint:	oprint.o pp.o
	$(CC) $(CFLAGS) -o oprint oprint.o pp.o



A =\
as.c		xrealloc.c	xmalloc.c	hash.c		hex-value.c\
atof-generic.c	append.c	messages.c	expr.c		bignum-copy.c\
frags.c		input-file.c	input-scrub.c	output-file.c	read.c\
subsegs.c	symbols.c	write.c		strstr.c\
flonum-const.c	flonum-copy.c	flonum-mult.c	app.c		version.c\
gdb.c		gdb-file.c	gdb-symbols.c	gdb-blocks.c	obstack.c\
gdb-lines.c

B =\
m68k.c		atof-m68k.c	atof-m68k-assist.s

C =\
vax.c		atof-vax.c

H = \
a.out.h		as.h		bignum.h	expr.h		flonum.h \
frags.h		hash.h		input-file.h	m68k-opcode.h	md.h	 \
obstack.h	read.h		struc-symbol.h	subsegs.h		 \
symbols.h	vax-inst.h	vax-opcode.h	write.h			 \
m-sun3.h	m-generic.h	pmmu.h		m-hpux.h

# JF unused	flonum-normal.c	flonum-print.c cpystr.c

lint_68k:
	lint -I. $A $B

lint_vax:
	lint -I. $A $C

gas-dist.tar: COPYING README $A $B $C $H Makefile
	tar cvf gas-dist.tar COPYING README $A $B $C $H Makefile

gas-dist.tar.Z:	gas-dist.tar
	compress < gas-dist.tar > gas-dist.tar.Z

dist:	gas-dist.tar gas-dist.tar.Z
