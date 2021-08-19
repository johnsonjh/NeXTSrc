/* tm-next.h:  Definitions for Next as target machine for GNU C compiler.  */

#include "tm-m68k.h"

/* See tm-m68k.h.  7 means 68020/030 with 68881/882.  */

#define TARGET_DEFAULT 7

/* Support The Objective-C language (as defined by Stepstone, Inc. v4.0) */

#define NeXT_OBJC

/* These compiler options take an argument.  */

#define WORD_SWITCH_TAKES_ARG(STR)			\
  (!strcmp (STR, "Ttext") || !strcmp (STR, "Tdata") ||	\
   !strcmp (STR, "segalign") || !strcmp (STR, "seg1addr"))

#define WORD_SWITCH_TAKES_TWO_ARGS(STR)	\
  (!strcmp (STR, "segaddr"))

#define WORD_SWITCH_TAKES_THREE_ARGS(STR)			\
  (!strcmp (STR, "segprot") || !strcmp (STR, "sectcreate") ||	\
   !strcmp (STR, "sectalign") || !strcmp (STR, "segcreate"))

/* Names to predefine in the preprocessor for this target machine.  */

#define CPP_PREDEFINES "-Dmc68000 -DNeXT -Dunix -D__MACH__"

/* Machine dependent ccp options.  */

#define CPP_SPEC "%{bsd:-D__STRICT_BSD__} %{MD:-MD %b.d} %{MMD:-MMD %b.d}"

/* Machine dependent ld options.  */

#define LINK_SPEC "%{Z} %{M} \
%{execute*} %{object*} %{preload*} %{fvmlib*} \
%{segalign*} %{seg1addr*} %{segaddr*} %{segprot*} %{seglinkedit*} \
%{sectcreate*} %{sectalign*} \
%{segcreate*} %{Mach*}"

/* Machine dependent libraries.  */

#define LIB_SPEC "%{!p:%{!pg:-lc}}%{p:-lsys_p}%{pg:-lsys_p}"
 
/* We specify crt0.o as -lcrt0.o so that ld will search the library path. */
#define STARTFILE_SPEC  \
  "%{pg:-lgcrt0.o}%{!pg:%{p:-lmcrt0.o}%{!p:-lcrt0.o}}"

/* Every structure or union's size must be a multiple of 2 bytes.  */

#define STRUCTURE_SIZE_BOUNDARY 16

/* We want C++ style comments to be supported for Objective-C */

#define CPLUSPLUS

/* Why not? */

#define DOLLARS_IN_IDENTIFIERS 1

/* Allow #sscs (but don't do anything). */

#define SCCS_DIRECTIVE

/* GDB uses DBX symbol format.
   Don't use stab continuation since it wastes space. */

#define DBX_DEBUGGING_INFO

#define DBX_CONTIN_LENGTH 0

/* Don't use .gcc_compiled symbols to communicate with GDB;
   They interfere with numerically sorted symbol lists. */

#define ASM_IDENTIFY_GCC(asm_out_file)

/* Support -MD and -MMD flags */

#define MACH_MAKE_DEPEND

#undef OVERRIDE_OPTIONS		/* defined in tm-m68k.h */
#define OVERRIDE_OPTIONS		\
{					\
  if (TARGET_FPA) target_flags &= ~2;	\
  if (write_symbols == GDB_DEBUG)	\
    {					\
      warning ("`-gg' symbols obsolete (using `-g' format)");	\
      write_symbols = DBX_DEBUG;	\
      use_gdb_dbx_extensions = 1;	\
    }					\
}

/* This is how to output an assembler line defining a `double' constant.  */

#undef ASM_OUTPUT_DOUBLE
#define ASM_OUTPUT_DOUBLE(FILE,VALUE)					\
  (isinf ((VALUE))							\
   ? fprintf (FILE, "\t.double 0r%s99e999\n", ((VALUE) > 0 ? "" : "-")) \
   : fprintf (FILE, "\t.double 0r%.20e\n", (VALUE)))

/* This is how to output an assembler line defining a `float' constant.  */

#undef ASM_OUTPUT_FLOAT
#define ASM_OUTPUT_FLOAT(FILE,VALUE)					\
  (isinf ((VALUE))							\
   ? fprintf (FILE, "\t.single 0r%s99e999\n", ((VALUE) > 0 ? "" : "-")) \
   : fprintf (FILE, "\t.single 0r%.20e\n", (VALUE)))

#undef ASM_OUTPUT_FLOAT_OPERAND
#define ASM_OUTPUT_FLOAT_OPERAND(FILE,VALUE)				\
  (isinf ((VALUE))							\
   ? fprintf (FILE, "#0r%s99e999", ((VALUE) > 0 ? "" : "-")) \
   : fprintf (FILE, "#0r%.9g", (VALUE)))

#undef ASM_OUTPUT_DOUBLE_OPERAND
#define ASM_OUTPUT_DOUBLE_OPERAND(FILE,VALUE)				\
  (isinf ((VALUE))							\
   ? fprintf (FILE, "#0r%s99e999", ((VALUE) > 0 ? "" : "-")) \
   : fprintf (FILE, "#0r%.20g", (VALUE)))
