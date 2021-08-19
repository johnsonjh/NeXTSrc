#
# Generic top-level makefile for multi-file commands
# (c) NeXT, Inc.  1987
#
# makefile for hashed database utilities
#
ROOT = root
GROUP = wheel

DSTDIR = 
BINDIR = $(DSTDIR)/usr/lib/database
LIBDIR = $(DSTDIR)/usr/lib

CDEFS = -DNeXT_MOD -DNeXT_NFS
CINCS = -I..
CFLAGS = -O -g $(CDEFS) ${CINCS}
IFLAGS = -o ${ROOT} -g ${GROUP} -m 711
LIBS = ${DSTROOT}${LIBDIR}/libdb.a

DSTDIRS = ${DSTROOT}${BINDIR} ${DSTROOT}${LIBDIR}

HFILES = dbUtil.h
LFILES = _dbBaseName.c _dbCatenate.c _dbCompare.c _dbCopy.c _dbPrintD.c \
	_dbReport.c
PFILES = dbCatenate.c dbCompare.c dbCompress.c dbCopy.c dbDescribe.c dbExpand.c
CFILES = ${LFILES} ${PFILES}
DFILES = ${CFILES:.c=.d}
SFILES = 
YFILES = 
SRCS = 	${HFILES} ${YFILES} ${CFILES} ${SFILES}

PRODUCTS = ${PFILES:.c=}

MAKEDEP = Makefile.depends
FILES = ${SRCS} Makefile ${MAKEDEP}
INTERMEDIATES = ${CFILES:.c=.o}
DERIVED = 
GARBAGE = TAGS tags *_vers.c *\~ ${MAKEDEP} .depend_done

all: ${PRODUCTS}


${PRODUCTS}:: .depend_done


dbCatenate_OFILES = dbCatenate.o _dbCatenate.o dbCatenate_vers.o _dbBaseName.o
dbCatenate:: ${dbCatenate_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbCatenate_OFILES} ${LIBS}

dbCatenate_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbCatenate > $@


dbCompare_OFILES = dbCompare.o _dbCompare.o _dbCatenate.o dbCompare_vers.o \
	_dbBaseName.o
dbCompare:: ${dbCompare_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbCompare_OFILES} ${LIBS}

dbCompare_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbCompare > $@


dbCompress_OFILES = dbCompress.o dbCompress_vers.o _dbBaseName.o
dbCompress:: ${dbCompress_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbCompress_OFILES} ${LIBS}

dbCompress_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbCompress > $@


dbCopy_OFILES = dbCopy.o _dbCopy.o dbCopy_vers.o _dbBaseName.o
dbCopy:: ${dbCopy_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbCopy_OFILES} ${LIBS}

dbCopy_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbCopy > $@


dbDescribe_OFILES = dbDescribe.o _dbReport.o _dbPrintD.o dbDescribe_vers.o \
	_dbBaseName.o
dbDescribe:: ${dbDescribe_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbDescribe_OFILES} ${LIBS}

dbDescribe_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbDescribe > $@


dbExpand_OFILES = dbExpand.o dbExpand_vers.o _dbBaseName.o
dbExpand:: ${dbExpand_OFILES}
	$(CC) $(CFLAGS) -o $@ ${dbExpand_OFILES} ${LIBS}

dbExpand_vers.c: ${DOVERS}
	@rm -f $@
	vers_string -c dbExpand > $@

#
# Standard targets
#

clean: ALWAYS
	-rm -f ${PRODUCTS} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	touch ${MAKEDEP}

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} dbCatenate ${DSTROOT}${BINDIR}/dbCatenate
	install ${IFLAGS} dbCompare ${DSTROOT}${BINDIR}/dbCompare
	install ${IFLAGS} dbCompress ${DSTROOT}${BINDIR}/dbCompress
	install ${IFLAGS} dbCopy ${DSTROOT}${BINDIR}/dbCopy
	install ${IFLAGS} dbDescribe ${DSTROOT}${BINDIR}/dbDescribe
	install ${IFLAGS} dbExpand ${DSTROOT}${BINDIR}/dbExpand

installsrc: SRCROOT
	-rm -rf ${SRCROOT}
	mkdirs ${SRCROOT}
	cp ${FILES} ${SRCROOT}
	
DSTROOT:
#	@if [ -n "${$@}" ]; \
#	then \
#		exit 0; \
#	else \
#		echo Must define $@; \
#		exit 1; \
#	fi

SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS}:
	mkdirs $@

depend .depend_done: ${DERIVED}
	-rm -f ${MAKEDEP}
	-touch ${MAKEDEP}
	${CC} -MD -E ${CINCS} ${CDEFS} ${CFILES} ${SFILES} ${DERIVED} \
		> /dev/null
	@md -m ${MAKEDEP} -d ${DFILES}
	-touch .depend_done

tags: ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

-include ${MAKEDEP}
