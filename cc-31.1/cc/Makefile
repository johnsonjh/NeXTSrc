
# Configuration parameters:

# Machine to target compiler for:
MACHINE = NeXT

# Avoid using ./cc:
CC = /bin/cc

DBG_CFLAGS = -g -DNeXT_STATISTICS
OPT_CFLAGS = -g -O -fstrength-reduce
PROF_CFLAGS = -pg -O -fstrength-reduce
# Default value only; often overridden by recursive make:
CFLAGS = $(OPT_CFLAGS)

INCLUDE_DIRS = -I. -Iconfig -I$(INSN_DIR)

LIBS = -lsys_s
PROF_LIBS = -lsys_p

BISON = ../bison/bison.NEW 
BISONFLAGS = -v

MD = md
MDFLAGS = -d

IFLAGS = -c -s

BINDIR = /bin
LIBDIR = /lib

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
CC_OFILES = $(CC_CFILES:.c=.o) $(SHARED_CFILES:.c=.o) cc_vers.o

CPP_YFILES = cexp.y
CPP_CFILES = cccp.c
CPP_OFILES = $(CPP_YFILES:.y=.o) $(CPP_CFILES:.c=.o) $(SHARED_CFILES:.c=.o) \
	cpp_vers.o

CC1_YFILES = objc-parse.y
CC1_CFILES = objc-actions.c c-decl.c c-typeck.c c-convert.c \
	toplev.c tree.c print-tree.c stor-layout.c fold-const.c \
	rtl.c rtlanal.c expr.c stmt.c expmed.c explow.c optabs.c varasm.c \
	symout.c dbxout.c sdbout.c emit-rtl.c \
	integrate.c jump.c cse.c loop.c flow.c stupid.c combine.c \
	regclass.c local-alloc.c global-alloc.c reload.c reload1.c caller-save.c \
	final.c recog.c
CC1_HFILES = basic-block.h c-parse.h c-tree.h conditions.h expr.h flags.h \
	gdbfiles.h hard-reg-set.h input.h objc-actions.h output.h real.h \
	recog.h regs.h reload.h rtl.h stab.h symseg.h tree.h typeclass.h

INSN_CFILES = insn-emit.c insn-extract.c insn-output.c insn-peep.c insn-recog.c

CC1_OFILES = $(INSN_CFILES:.c=.o) $(CC1_YFILES:.y=.o) $(CC1_CFILES:.c=.o) \
	$(SHARED_CFILES:.c=.o) cc1_vers.o

GEN_CFILES = genemit.c genextract.c genoutput.c genpeep.c \
	genrecog.c gencodes.c genconfig.c genflags.c
GEN_OFILES = $(GEN_CFILES:.c=.o)
GEN = $(GEN_OFILES:.o=)

INSN_C = emit extract output peep recog
INSN_H = codes config flags

CFILES = $(SHARED_CFILES) $(CC_CFILES) $(CPP_CFILES) $(CC1_CFILES) $(GEN_CFILES)
HFILES = $(SHARED_HFILES) $(CC1_HFILES)
YFILES = $(CPP_YFILES) $(CC1_YFILES)

DEF_FILES = machmode.def rtl.def stab.def tree.def

SOURCEFILES = $(CFILES) $(HFILES) $(YFILES) $(DEF_FILES) md config \
	config.gcc move-if-change Makefile

# Dependencies:

# The executables:

default:: help

cc cpp cc1 cc.debug cpp.debug cc1.debug cc.profile cpp.profile cc1.profile make-gen:: $(OFILE_DIR)

cc cc.debug cc.profile:: config.h $(CC_OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CC_OFILES) $(LIBS)

cpp cpp.debug cpp.profile:: config.h $(CPP_OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CPP_OFILES) $(LIBS)

cc1 cc1.debug cc1.profile:: config.h tm.h stamp-md $(CC1_OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CC1_OFILES) $(LIBS)

make-gen:: $(GEN)


# The compiler configuration files:

config.h tm.h md aux-output.c:
	@echo "Configuring compiler for use with a $(MACHINE):"
	@./config.gcc $(MACHINE)


# The source files that are generated from the machine description.
# stamp-md records the last time source files were generated.  Each time
# the machine description changes, all the source files are re-generated,
# but the files are only touched if their contents have actually changed.

stamp-md:: $(INSN_DIR)
stamp-md:: stamp-gen md aux-output.c
	@echo "Generating source files from machine description:"
	@for i in $(INSN_C); do\
	  echo "Generating $(INSN_DIR)/insn-$$i.c";\
	  $(GEN_DIR)/gen$$i md > $(INSN_DIR)/tmp-insn-$$i.c;\
	  ./move-if-change $(INSN_DIR)/tmp-insn-$$i.c $(INSN_DIR)/insn-$$i.c;\
	done
	@for i in $(INSN_H); do\
	  echo "Generating $(INSN_DIR)/insn-$$i.h";\
	  $(GEN_DIR)/gen$$i md > $(INSN_DIR)/tmp-insn-$$i.h;\
	  ./move-if-change $(INSN_DIR)/tmp-insn-$$i.h $(INSN_DIR)/insn-$$i.h;\
	done
	@rm -f $(INSN_DIR)/stamp-md
	@touch $(INSN_DIR)/stamp-md
	@echo ""


# The source file generation tools.
# stamp-gen records the last time the tools were built.

stamp-gen:: $(GEN_DIR)
stamp-gen:: $(GEN_CFILES) rtl.c obstack.c
	@echo ""
	@echo "Building source generation tools:"
	@$(MAKE) make-gen OFILE_DIR=gen_obj CFLAGS="$(OPT_CFLAGS)"
	@rm -f $(GEN_DIR)/stamp-gen
	@touch $(GEN_DIR)/stamp-gen
	@echo ""

COMMON = rtl.o obstack.o

gencodes: gencodes.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ gencodes.o $(COMMON) $(LIBS)
genconfig: genconfig.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genconfig.o $(COMMON) $(LIBS)
genflags: genflags.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genflags.o $(COMMON) $(LIBS)
genemit: genemit.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genemit.o $(COMMON) $(LIBS)
genextract: genextract.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genextract.o $(COMMON) $(LIBS)
genoutput: genoutput.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genoutput.o $(COMMON) $(LIBS)
genpeep: genpeep.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genpeep.o $(COMMON) $(LIBS)
genrecog: genrecog.o $(COMMON)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(GEN_DIR)/$@ genrecog.o $(COMMON) $(LIBS)


# Other .c files which require special compilation flags:

cccp.o: cccp.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -MMD -c cccp.c -o $(OFILE_DIR)/cccp.o \
          -DGCC_INCLUDE_DIR=\"\" \
          -DGPLUSPLUS_INCLUDE_DIR=\"\"
	@ $(MD) $(MDFLAGS) -m Makefile.depend cccp.d

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
	@rm -f objc-parse.output objc-parse.c
	$(BISON) $(BISONFLAGS) -o $@ objc-parse.y


# SGS version files generated by vers_string:

cc_vers.c:
	@rm -f cc_vers.c
	vers_string -c cc > $@

cpp_vers.c:
	@rm -f cpp_vers.c
	vers_string -c cpp > $@

cc1_vers.c:
	@rm -f cc1_vers.c
	vers_string -c cc1 > $@


# Suffix rules:

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -MMD -c $*.c -o $(OFILE_DIR)/$*.o
	@ $(MD) $(MDFLAGS) -m Makefile.depend $*.d


# Any necessary directories:

$(OFILE_DIR) $(INSN_DIR) $(GEN_DIR) $(SRCROOT) $(DSTROOT)$(BINDIR) $(DSTROOT)$(LIBDIR):
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

all: cc cpp cc1

debug:
	@$(MAKE) cc.debug cpp.debug cc1.debug \
	OFILE_DIR=debug_obj CFLAGS="$(DBG_CFLAGS)"

profile:
	@$(MAKE) cc.profile cpp.profile cc1.profile \
	OFILE_DIR=profile_obj \
	CFLAGS="$(PROF_CFLAGS)" LIBS="$(PROF_LIBS)"

clean:
	@rm -rf obj debug_obj profile_obj gen_obj gen insn
	@rm -f tm.h md aux-output.c config.h
	@rm -f cc cpp cc1
	@rm -f cc.debug cpp.debug cc1.debug
	@rm -f cc.profile cpp.profile cc1.profile
	@rm -f $(YFILES:.y=.c) $(YFILES:.y=.tab.c) $(YFILES:.y=.output)
	@rm -f Makefile.depend
	@touch Makefile.depend
	@rm -f config.status
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

install: all $(DSTROOT)$(BINDIR) $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) -m 555 cc $(DSTROOT)$(BINDIR)
	install $(IFLAGS) -m 555 cpp $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) -m 555 cc1 $(DSTROOT)$(LIBDIR)

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



