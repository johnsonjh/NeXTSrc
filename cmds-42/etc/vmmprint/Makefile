#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
CDEFS=
CFLAGS=	-O -g $(CDEFS)
LDFLAGS= -object
IFLAGS= -s -g kmem -m 2755
LIBS= -lsys_s

#
# The name of this program as installed
#
PRODUCT= vmmprint

#
# Where it gets installed
#
BINDIR=	${DSTROOT}/usr/etc

#
# The list of files that make-up this command.  Used both
# for compilation and tags.
#
HFILES=
YFILES=
CFILES= ${PRODUCT}.c
SFILES=

#
# SRCS is used by "tags" command
# OBJS is used by ld
#
SRCS= ${HFILES} ${YFILES} ${CFILES} ${SFILES}
OBJS= ${CFILES:.c=.o} ${YFILES:.y=.o} ${SFILES:.s=.o}

#
# DERIVED is the list of files that must be made before a make depend
# is done.  Typically the C file corresponding to yacc source goes here.
#
DERIVED=
GARBAGE= TAGS tags vers.c

#
# Standard targets
#
all:	${PRODUCT}

clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCT} ${DERIVED} ${GARBAGE}
	-rm -f Makedep

install: DSTROOT ${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${BINDIR}/${PRODUCT}

#
# Building PRODUCT
#
${PRODUCT}.NEW:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

${PRODUCT}:	$$@.NEW

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${BINDIR}:
	mkdirs $@

depend:

tags:	ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
	md -u Makedep -d $*.d

-include Makedep
