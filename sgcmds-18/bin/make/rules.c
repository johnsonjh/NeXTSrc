/*
 * DEFAULT RULES FOR UNIX
 *
 * These are the internal rules that "make" trucks around with it at
 * all times. One could completely delete this entire list and just
 * conventionally define a global "include" makefile which had these
 * rules in it. That would make the rules dynamically changeable
 * without recompiling make. This file may be modified to local
 * needs.
 *
 */

#ifdef CMUCS
#include <sys/features.h>
#endif
#include "defs.h"

char *builtin[] = {
#ifdef pwb
	".SUFFIXES : .L .out .o .c .f .e .r .y .yr .ye .l .s .z .x .t .h .cl",
#else
#ifdef	NeXT_MOD
    /* added .a for libraries, .m for Objective-C, .psw for pswrap, .pswm
       for pswraps with ObjC mixed in the file, .ym for yacc files using
       ObjC, and .lm for lex files using ObjC.
    */
	".SUFFIXES : .out .a .o .m .s .c .psw .pswm .F .f .e .r .y .yr .ye .ym .l .lm .p .sh .csh .h",
#else	NeXT_MOD
	".SUFFIXES : .out .o .s .c .F .f .e .r .y .yr .ye .l .p .sh .csh .h",
#endif	NeXT_MOD
#endif

	/*
	 * PRESET VARIABLES
	 */
#ifndef NeXT_MOD
	"MAKE=make",
#endif NeXT_MOD
	"AR=ar",
#ifdef	NeXT_MOD
	"ARFLAGS=rv",
#else	NeXT_MOD
	"ARFLAGS=",
#endif	NeXT_MOD
	"RANLIB=ranlib",
	"LD=ld",
	"LDFLAGS=",
	"LINT=lint",
	"LINTFLAGS=",
#ifdef CMUCS
	"CO=rcsco",
#else
	"CO=co",
#endif
	"COFLAGS=-q",
	"CP=cp",
	"CPFLAGS=",
	"MV=mv",
	"MVFLAGS=",
	"RM=rm",
	"RMFLAGS=-f",
	"YACC=yacc",
	"YACCR=yacc -r",
	"YACCE=yacc -e",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"CC=cc",
	"CFLAGS=",
#ifdef	NeXT_MOD
    /* switches for the ObjC preprocessor (uses $(CC) for its command) */
	"OBJCFLAGS=",
    /* commands and switches for pswrap */
	"PSWRAP=pswrap",
	"PSWFLAGS=",
#endif	NeXT_MOD
	"AS=as",
	"ASFLAGS=",
	"PC=pc",
	"PFLAGS=",
	"RC=f77",
	"RFLAGS=",
	"EC=efl",
	"EFLAGS=",
	"FC=f77",
	"FFLAGS=",
	"LOADLIBES=",
#ifdef pwb
	"SCOMP=scomp",
	"SCFLAGS=",
	"CMDICT=cmdict",
	"CMFLAGS=",
#endif
#ifdef pdp11
	"CPU=pdp11",
	"MACHINE=PDP11",
	"machine=pdp11",
	"CPUTYPE=PDP11",
	"cputype=pdp11",
#endif
#ifdef vax
	"CPU=vax",
	"MACHINE=VAX",
	"machine=vax",
	"CPUTYPE=VAX",
	"cputype=vax",
#endif
#ifdef sun
#ifdef sparc
	"CPU=sparc",
	"MACHINE=SUN",
	"machine=sun",
	"CPUTYPE=SPARC",
	"cputype=sparc",
#endif
#ifdef mc68020
	"CPU=mc68020",
	"MACHINE=SUN",
	"machine=sun",
	"CPUTYPE=MC68020",
	"cputype=mc68020",
#else
	"CPU=mc68000",
	"MACHINE=SUN",
	"machine=sun",
	"CPUTYPE=MC68000",
	"cputype=mc68000",
#endif
#endif
#ifdef	NeXT
	"CPU=next",
	"MACHINE=NEXT",
	"machine=next",
	"CPUTYPE=NEXT",
	"cputype=next",
#endif	NeXT
#ifdef ibm032
	"CPU=ibm032",
	"MACHINE=IBMRT",
	"machine=ibmrt",
	"CPU=ibm032",
	"MACHINE=IBM032",
	"machine=ibm032",
	"CPUTYPE=IBM032",
	"cputype=ibm032",
#endif
#ifdef ibm370
	"CPU=ibm370",
	"MACHINE=IBM",
	"machine=ibm",
	"CPUTYPE=IBM370",
	"cputype=ibm370",
#endif
#ifdef ns32000
#ifdef balance
	"CPU=ns32032",
	"MACHINE=BALANCE",
	"machine=balance",
	"CPUTYPE=NS32032",
	"cputype=ns32032",
#endif
#ifdef MULTIMAX
	"CPU=ns32032",
	"MACHINE=MMAX",
	"machine=mmax",
	"CPUTYPE=NS32032",
	"cputype=ns32032",
#endif
#endif

	/*
	 * SINGLE SUFFIX RULES
	 */
	".s :",
	"\t$(AS) $(ASFLAGS) -o $@ $<",

	".c :",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $< $(LOADLIBES) -o $@",

	".F .f :",
	"\t$(FC) $(LDFLAGS) $(FFLAGS) $< $(LOADLIBES) -o $@",

	".e :",
	"\t$(EC) $(LDFLAGS) $(EFLAGS) $< $(LOADLIBES) -o $@",

	".r :",
	"\t$(RC) $(LDFLAGS) $(RFLAGS) $< $(LOADLIBES) -o $@",

	".p :",
	"\t$(PC) $(LDFLAGS) $(PFLAGS) $< $(LOADLIBES) -o $@",

	".y :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@",
	"\t$(RM) $(RMFLAGS) y.tab.c",

#ifdef	NeXT_MOD
    /* rule for ObjC files */
	".m.o :",
	"\t$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m",

    /* rules for pswrap files */
	".psw.c :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $*.c $*.psw",

	".psw.o :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $*.c $*.psw",
	"\t$(CC) $(CFLAGS) -c $*.c",

	".pswm.m :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $*.m $*.pswm",

	".pswm.o :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $*.m $*.pswm",
	"\t$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m",
#endif
	".l :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@",
	"\t$(RM) $(RMFLAGS) lex.yy.c",

	".sh :",
	"\t$(CP) $(CPFLAGS) $< $@",
	"\tchmod +x $@",

	".csh :",
	"\t$(CP) $(CPFLAGS) $< $@",
	"\tchmod +x $@",

	".CO :",
	"\t$(CO) $(COFLAGS) $< $@",

	".CLEANUP :",
	"\t$(RM) $(RMFLAGS) $?",

	/*
	 * DOUBLE SUFFIX RULES
	 */
	".s.o :",
	"\t$(AS) -o $@ $<",

	".c.o :",
	"\t$(CC) $(CFLAGS) -c $<",

#ifdef	NeXT_MOD
    /* rule for yacc files using ObjC */
	".ym.m :",
	"\t$(YACC) $(YFLAGS) $*.ym",
	"\t$(MV) $(MVFLAGS) y.tab.c $*.m",
#endif	NeXT_MOD

	".F.o .f.o :",
	"\t$(FC) $(FFLAGS) -c $<",

	".e.o :",
	"\t$(EC) $(EFLAGS) -c $<",

#ifdef	NeXT_MOD
    /* rule for lex files using ObjC */
	".lm.m :",
	"\t$(LEX) $(LFLAGS) $*.lm",
	"\t$(MV) $(MVFLAGS) lex.yy.c $*.m",
#endif	NeXT_MOD

	".r.o :",
	"\t$(RC) $(RFLAGS) -c $<",

	".y.o :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c y.tab.c",
	"\t$(RM) $(RMFLAGS) y.tab.c",
	"\t$(MV) $(MVFLAGS) y.tab.o $@",

	".yr.o:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\t$(RC) $(RFLAGS) -c y.tab.r",
	"\t$(RM) $(RMFLAGS) y.tab.r",
	"\t$(MV) $(MVFLAGS) y.tab.o $@",

	".ye.o :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\t$(EC) $(EFLAGS) -c y.tab.e",
	"\t$(RM) $(RMFLAGS) y.tab.e",
	"\t$(MV) $(MVFLAGS) y.tab.o $@",

#ifdef	NeXT_MOD
    /* rule for yacc files using ObjC */
	".ym.o :",
	"\t$(YACC) $(YFLAGS) $*.ym",
	"\t$(MV) $(MVFLAGS) y.tab.c $*.m",
	"\t$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m",
	"\t$(RM) $(RMFLAGS) $*.m",
#endif	NeXT_MOD

	".l.o :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c lex.yy.c",
	"\t$(RM) $(RMFLAGS) lex.yy.c",
	"\t$(MV) $(MVFLAGS) lex.yy.o $@",

#ifdef	NeXT_MOD
    /* rule for lex files using ObjC */
	".lm.o :",
	"\t$(LEX) $(LFLAGS) $*.lm",
	"\t$(MV) $(MVFLAGS) lex.yy.c $*.m",
	"\t$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m",
	"\t$(RM) $(RMFLAGS) $*.m",
#endif	NeXT_MOD

	".p.o :",
	"\t$(PC) $(PFLAGS) -c $<",

#ifdef pwb
	".cl.o :",
	"\tclass -c $<",
#endif

	".y.c :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(MV) $(MVFLAGS) y.tab.c $@",

	".yr.r:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\t$(MV) $(MVFLAGS) y.tab.r $@",

	".ye.e :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\t$(MV) $(MVFLAGS) y.tab.e $@",

	".l.c :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(MV) $(MVFLAGS) lex.yy.c $@",

#ifdef pwb
	".o.L .c.L .t.L:",
	"\t$(SCOMP) $(SCFLAGS) $<",

	".t.o:",
	"\t$(SCOMP) $(SCFLAGS) -c $<",

	".t.c:",
	"\t$(SCOMP) $(SCFLAGS) -t $<",

	".h.z .t.z:",
	"\t$(CMDICT) $(CMFLAGS) $<",

	".h.x .t.x:",
	"\t$(CMDICT) $(CMFLAGS) -c $<",
#endif

	".o.out .s.out .c.out :",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $< $(LOADLIBES) -o $@",

	".F.out .f.out :",
	"\t$(FC) $(LDFLAGS) $(FFLAGS) $< $(LOADLIBES) -o $@",

	".e.out :",
	"\t$(EC) $(LDFLAGS) $(EFLAGS) $< $(LOADLIBES) -o $@",

	".r.out :",
	"\t$(RC) $(LDFLAGS) $(RFLAGS) $< $(LOADLIBES) -o $@",

	".y.out :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@",
	"\t$(RM) $(RMFLAGS) y.tab.c",

	".l.out :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@",
	"\t$(RM) $(RMFLAGS) lex.yy.c",

	".p.out :",
	"\t$(PC) $(LDFLAGS) $(PFLAGS) $< $(LOADLIBES) -o $@",


#ifdef notdef
	"%0/%1 :",
	"\tcd %0 && $(MAKE) %1",
#endif

#ifdef	NeXT_MOD
    /* rule for single file ObjC programs */
	".m :",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $(OBJCFLAGS) $*.m $(LOADLIBES) -o $*",

    /* rules for single file pswrap programs */
	".psw :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $*.c $*.psw",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $*.c $(LOADLIBES) -o $@",

	".pswm :",
	"\t$(PSWRAP) $(PSWFLAGS) -o $.m $*.pswm",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $(OBJCFLAGS) $*.m $(LOADLIBES) -o $*",

    /* rule for single file yacc progs using ObjC */
	".ym :",
	"\t$(YACC) $(YFLAGS) $*.ym",
	"\t$(MV) $(MVFLAGS) y.tab.c $*.m",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $(OBJCFLAGS) $*.m $(LOADLIBES) -ly -o $*",
	"\t$(RM) $(RMFLAGS) $*.m",

    /* rule for single file lex progs using ObjC */
	".lm :",
	"\t$(LEX) $(LFLAGS) $*.lm",
	"\t$(MV) $(MVFLAGS) lex.yy.c $*.m",
	"\t$(CC) $(LDFLAGS) $(CFLAGS) $(OBJCFLAGS) $*.m $(LOADLIBES) -ll -o $*",
	"\t$(RM) $(RMFLAGS) $*.m",

#endif	NeXT_MOD

	0 };
