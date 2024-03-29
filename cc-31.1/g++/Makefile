
# Configuration parameters:

# Machine to target compiler for:
MACHINE = NeXT

# Avoid using ./cc:
CC = /bin/cc

DBG_CFLAGS = -g
OPT_CFLAGS = -g -O
PROF_CFLAGS = -pg -O
# Default value only; often overridden by recursive make:
CFLAGS = $(OPT_CFLAGS)

INCLUDE_DIRS = -I. -Iconfig -I$(INSN_DIR) -I../include

LIBS = -lsys_s
PROF_LIBS = -lsys_p

BISON = ../bison/bison.NEW 
BISONFLAGS = -v

MD = md
MDFLAGS = -d

IFLAGS = -c

BINDIR = /bin
LIBDIR = /lib
USRLIBDIR = /usr/lib
USRUCBDIR = /usr/ucb

# Default value only; often overridden by recursive make:
OFILE_DIR = obj
# Directory for source files generated from md:
INSN_DIR = insn
# Directory for source files generation tools:
GEN_DIR = ./gen

VPATH = $(OFILE_DIR):$(INSN_DIR):$(GEN_DIR)

GARBAGE = *.o *.s *.d *.output *_vers.c *.greg *.lreg *.combine *.flow \
	*.cse *.jump *.rtl *.tree *.loop *.dbr *.jump2 .places .nfs*

SHARED_CFILES = version.c obstack.c
SHARED_HFILES = obstack.h

CC_CFILES = gcc.c
CC_OFILES = $(CC_CFILES:.c=.o) $(SHARED_CFILES:.c=.o) cc++_vers.o

CC1_YFILES = cplus-parse.y
CC1_CFILES = cplus-decl.c cplus-decl2.c cplus-typeck.c cplus-type2.c \
	cplus-tree.c cplus-lex.c cplus-class.c cplus-init.c cplus-except.c \
	cplus-method.c cplus-cvt.c cplus-search.c cplus-ptree.c cplus-expr.c \
	toplev.c tree.c print-tree.c stor-layout.c fold-const.c \
	rtl.c rtlanal.c expr.c stmt.c expmed.c explow.c optabs.c varasm.c \
	symout.c dbxout.c sdbout.c emit-rtl.c \
	integrate.c jump.c cse.c loop.c flow.c stupid.c combine.c \
	regclass.c local-alloc.c global-alloc.c reload.c reload1.c caller-save.c \
	final.c recog.c case.c lastfile.c
CC1_HFILES = basic-block.h cplus-decl.h cplus-parse.h cplus-tree.h \
	conditions.h expr.h flags.h \
	gdbfiles.h hard-reg-set.h input.h objc-actions.h output.h real.h \
	recog.h regs.h reload.h rtl.h stab.h symseg.h tree.h typeclass.h

INSN_CFILES = insn-emit.c insn-extract.c insn-output.c insn-peep.c insn-recog.c

CC1_OFILES = $(INSN_CFILES:.c=.o) cplus-tab.o $(CC1_CFILES:.c=.o) \
	$(SHARED_CFILES:.c=.o) cc1++_vers.o

GEN_CFILES = genemit.c genextract.c genoutput.c genpeep.c \
	genrecog.c gencodes.c genconfig.c genflags.c
GEN_OFILES = $(GEN_CFILES:.c=.o)
GEN = $(GEN_OFILES:.o=)

INSN_C = emit extract output peep recog
INSN_H = codes config flags

CFILES = $(SHARED_CFILES) $(CC_CFILES) $(CC1_CFILES) $(GEN_CFILES)
HFILES = $(SHARED_HFILES) $(CC1_HFILES)
YFILES = $(CC1_YFILES)

DEF_FILES = machmode.def rtl.def stab.def tree.def

SOURCEFILES = $(CFILES) $(HFILES) $(YFILES) $(DEF_FILES) md config \
	config.gcc move-if-change Makefile

# Dependencies:

# The executables:

default:: help

cc++ cc1++ cc++.debug cc1++.debug cc++.profile cc1++.profile make-gen collect g++filt libc++.a:: $(OFILE_DIR)

cc++ cc++.debug cc++.profile:: config.h $(CC_OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CC_OFILES) $(LIBS)

cc1++ cc1++.debug cc1++.profile:: config.h tm.h stamp-md $(CC1_OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CC1_OFILES) $(LIBS)

collect:: config.h collect.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ collect.o $(LIBS) -object

g++filt:: g++filt.o cplus-dem.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ g++filt.o cplus-dem.o $(LIBS) -object

libc++.a:: gnulib3.o
	$(AR) rc libc++.a gnulib3.o
	ranlib -s libc++.a

make-gen:: $(GEN)


# Other .c files which require special compilation flags:

gcc.o: gcc.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -MMD -c gcc.c -o $(OFILE_DIR)/gcc.o \
	  -DSTANDARD_STARTFILE_PREFIX=\"$(LIBDIR)/\" \
	  -DSTANDARD_EXEC_PREFIX=\"$(LIBDIR)/\"
	  -DSTANDARD_SYS_PREFIX=\"$(BINDIR)/\"
	@ $(MD) $(MDFLAGS) -m Makefile.depend gcc.d


# .y files:

cexp.c: cexp.y
	$(BISON) $(BISONFLAGS) -o $@ cexp.y

c-parse.c: c-parse.y
	$(BISON) $(BISONFLAGS) -o $@ c-parse.y

objc-parse.c: objc-parse.y
	$(BISON) $(BISONFLAGS) -o $@ objc-parse.y

cplus-tab.c cplus-tab.h: cplus-parse.y
	$(BISON) $(BISONFLAGS) -d -o cplus-tab.c cplus-parse.y


# SGS version files generated by vers_string:

cc++_vers.c:
	@rm -f cc++_vers.c
	vers_string -c cc++ > $@

cc1++_vers.c:
	@rm -f cc1++_vers.c
	vers_string -c cc1++ > $@


# Suffix rules:

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -MMD -c $*.c -o $(OFILE_DIR)/$*.o
	@ $(MD) $(MDFLAGS) -m Makefile.depend $*.d


# Any necessary directories:

$(OFILE_DIR) $(INSN_DIR) $(GEN_DIR) $(SRCROOT) $(DSTROOT)$(BINDIR) $(DSTROOT)$(LIBDIR) $(DSTROOT)$(USRUCBDIR) $(DSTROOT)$(USRLIBDIR):
	mkdirs $@


# Library members defined in gnulib2.c:

LIB2FUNCS = _adddi3 _subdi3 _muldi3 _divdi3 _moddi3 _udivdi3 _umoddi3 _negdi2 \
    _anddi3 _iordi3 _xordi3 _lshrdi3 _lshldi3 _ashldi3 _ashrdi3 _one_cmpldi2 \
    _bdiv _cmpdi2 _ucmpdi2 _fixunsdfdi _fixdfdi _floatdidf _fixunsdfsi \
    _eprintf _builtin_new _builtin_New _builtin_del

gnulib2: gnulib2.c config.h
	for name in $(LIB2FUNCS); \
	do \
	  echo $${name}; \
	  $(CC) $(OPT_CFLAGS) $(INCLUDE_DIRS) -c -DL$${name} gnulib2.c -o $${name}.o; \
	  $(AR) rc gnulib2 $${name}.o; \
	  rm -f $${name}.o; \
	done
	ranlib gnulib2


# For all dependencies of .c files on .h files from -MMD option:

-include Makefile.depend


# Targets:

all: cc++ cc1++ collect g++filt libc++.a

debug:
	@$(MAKE) cc++.debug cc1++.debug \
	OFILE_DIR=debug_obj CFLAGS="$(DBG_CFLAGS)"

profile:
	@$(MAKE) cc++.profile cc1++.profile \
	OFILE_DIR=profile_obj \
	CFLAGS="$(PROF_CFLAGS)" LIBS="$(PROF_LIBS)"

clean:
	@rm -rf obj debug_obj profile_obj gen_obj
	@rm -f cc++ cc1++ collect g++filt libc++.a
	@rm -f cc++.debug cc1++.debug
	@rm -f cc++.profile cc1++.profile
	@rm -f $(YFILES:.y=.c) $(YFILES:.y=.tab.c) $(YFILES:.y=.output)
	@rm -f Makefile.depend
	@touch Makefile.depend
	@rm -f $(GARBAGE)

etags: config.h tm.h aux-output.c
	etags config.h tm.h aux-output.c $(CC1_CFILES) $(CC1_HFILES) $(CC1_YFILES)

ctags: config.h tm.h aux-output.c
	ctags config.h tm.h aux-output.c $(CC1_CFILES) $(CC1_HFILES) $(CC1_YFILES)

diff: $(SRCROOT)
	@for f in $(SOURCEFILES); \
	    do (echo $$f; \
	    csh -f -c "diff -r $(SRCROOT)/$$f $$f; exit 0") \
	done

installsrc: $(SRCROOT)
	-rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	chmod -f 755 $(SRCROOT)
	tar cf - $(SOURCEFILES) | (cd $(SRCROOT); tar xfp -)
	chmod 444 $(CFILES) $(HFILES) $(YFILES) $(DEF_FILES) md config/* Makefile
	chmod 755 config
	chmod 555 config.gcc move-if-change

install: all $(DSTROOT)$(BINDIR) $(DSTROOT)$(LIBDIR) $(DSTROOT)$(USRUCBDIR) $(DSTROOT)$(USRLIBDIR)
	install $(IFLAGS) -s -m 555 cc++ $(DSTROOT)$(BINDIR)
	install $(IFLAGS) -s -m 555 cc1++ $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) -s -m 555 collect $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) -s -m 555 g++filt $(DSTROOT)$(BINDIR)
	install $(IFLAGS) -m 555 nm++ $(DSTROOT)$(BINDIR)
	install $(IFLAGS) -m 555 gprof++ $(DSTROOT)$(USRUCBDIR)
	install $(IFLAGS) -m 444 libc++.a $(DSTROOT)$(USRLIBDIR)

help::
	@echo '  all'; \
	echo '  debug'; \
	echo '  profile'; \
	echo '  clean'; \
	echo '  etags'; \
	echo '  ctags'; \
	echo '  diff SRCROOT=somepath'; \
	echo '  installsrc SRCROOT=somepath'; \
	echo '  install [DSTROOT=somepath]'



