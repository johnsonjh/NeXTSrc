/* tm-next.h:  Definitions for Next as target machine for GNU C compiler.  */

#include "tm-m68k.h"

/* See tm-m68k.h.  0207 means 68040 (or 68030 with 68882).  */

#define TARGET_DEFAULT 0207

/* Support The Objective-C language (as defined by Stepstone, Inc. v4.0) */

#define NeXT_OBJC

/* Put quotes around method name symbols, and make internal Objective-C symbols
   local to the assembler. */

#undef ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(FILE,NAME)	\
  do { if (NAME[0] == '+' || NAME[0] == '-') fprintf (FILE, "\"%s\"", NAME);  \
       else if (!strncmp (NAME, "_OBJC_", 6)) fprintf (FILE, "L%s", NAME);     \
       else fprintf (FILE, "_%s", NAME); } while (0)

#undef STACK_BOUNDARY
/* Boundary (in *bits*) on which stack pointer should be aligned.  */
#define STACK_BOUNDARY 32

/* These compiler options take an argument.  */

#define WORD_SWITCH_TAKES_ARG(STR)			\
  (!strcmp (STR, "Ttext") || !strcmp (STR, "Tdata") ||	\
   !strcmp (STR, "segalign") || !strcmp (STR, "seg1addr"))

#define WORD_SWITCH_TAKES_TWO_ARGS(STR)	\
  (!strcmp (STR, "segaddr") || !strcmp (STR, "sectobjectsymbols"))

#define WORD_SWITCH_TAKES_THREE_ARGS(STR)			\
  (!strcmp (STR, "segprot") || !strcmp (STR, "sectcreate") ||	\
   !strcmp (STR, "sectalign") || !strcmp (STR, "segcreate") ||	\
   !strcmp (STR, "sectorder"))

/* Names to predefine in the preprocessor for this target machine.  */

#define CPP_PREDEFINES "-Dmc68000 -DNeXT -Dunix -D__MACH__"

/* Machine dependent ccp options.  */

#define CPP_SPEC "%{bsd:-D__STRICT_BSD__} %{MD:-MD %b.d} %{MMD:-MMD %b.d}"

/* Machine dependent ld options.  */

#define LINK_SPEC "%{Z} %{M} \
%{execute*} %{object*} %{preload*} %{fvmlib*} \
%{segalign*} %{seg1addr*} %{segaddr*} %{segprot*} \
%{seglinkedit*} %{noseglinkedit*} \
%{sectcreate*} %{sectalign*} %{sectobjectsymbols}\
%{segcreate*} %{Mach*} %{whyload} %{w} \
%{sectorder*} %{whatsloaded}"

/* Machine dependent libraries.  */

#define LIB_SPEC "%{!p:%{!pg:-lsys_s}} %{pg:-lsys_p}"
 
/* We specify crt0.o as -lcrt0.o so that ld will search the library path. */
#define STARTFILE_SPEC  \
  "%{pg:-lgcrt0.o}%{!pg: \
     %{p:%e-p profiling is no longer supported.  Use -pg instead.} \
     %{!p:-lcrt0.o}}"

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

#define SECTION_FUNCTION(FUNCTION, SECTION, DIRECTIVE, WAS_TEXT)	\
void									\
FUNCTION ()								\
{									\
  extern void text_section ();						\
  									\
  if (WAS_TEXT && flag_no_mach_text_sections)				\
    text_section ();							\
  else if (in_section != SECTION)					\
    {									\
      fprintf (asm_out_file, DIRECTIVE);				\
      in_section = SECTION;						\
    }									\
}									\

#define EXTRA_SECTIONS							\
  in_const, in_cstring, in_literal4, in_literal8, in_static_data,	\
  in_objc_class, in_objc_meta_class, in_objc_category,			\
  in_objc_class_vars, in_objc_instance_vars,				\
  in_objc_cls_meth, in_objc_inst_meth,					\
  in_objc_cat_cls_meth, in_objc_cat_inst_meth,				\
  in_objc_selector_strs, in_objc_selector_refs,				\
  in_objc_symbols, in_objc_module_info

#define EXTRA_SECTION_FUNCTIONS							\
SECTION_FUNCTION (const_section,	in_const,	".const\n", 1)		\
SECTION_FUNCTION (cstring_section,	in_cstring,	".cstring\n", 1)	\
SECTION_FUNCTION (literal4_section,	in_literal4,	".literal4\n", 1)	\
SECTION_FUNCTION (literal8_section,	in_literal8,	".literal8\n", 1)	\
SECTION_FUNCTION (static_data_section,	in_static_data,	".static_data\n", 0)	\
SECTION_FUNCTION (objc_class_section,		\
		  in_objc_class,		\
		  ".objc_class\n", 0)		\
SECTION_FUNCTION (objc_meta_class_section,	\
		  in_objc_meta_class,		\
		  ".objc_meta_class\n", 0)	\
SECTION_FUNCTION (objc_category_section,	\
		  in_objc_category,		\
		".objc_category\n", 0)		\
SECTION_FUNCTION (objc_class_vars_section,	\
		  in_objc_class_vars,		\
		  ".objc_class_vars\n", 0)	\
SECTION_FUNCTION (objc_instance_vars_section,	\
		  in_objc_instance_vars,	\
		  ".objc_instance_vars\n", 0)	\
SECTION_FUNCTION (objc_cls_meth_section,	\
		  in_objc_cls_meth,		\
		  ".objc_cls_meth\n", 0)	\
SECTION_FUNCTION (objc_inst_meth_section,	\
		  in_objc_inst_meth,		\
		  ".objc_inst_meth\n", 0)	\
SECTION_FUNCTION (objc_cat_cls_meth_section,	\
		  in_objc_cat_cls_meth,		\
		  ".objc_cat_cls_meth\n", 0)	\
SECTION_FUNCTION (objc_cat_inst_meth_section,	\
		  in_objc_cat_inst_meth,	\
		  ".objc_cat_inst_meth\n", 0)	\
SECTION_FUNCTION (objc_selector_strs_section,	\
		  in_objc_selector_strs,	\
		  ".objc_selector_strs\n", 0)	\
SECTION_FUNCTION (objc_selector_refs_section,	\
		  in_objc_selector_refs,	\
		  ".objc_selector_refs\n", 0)	\
SECTION_FUNCTION (objc_symbols_section,		\
		  in_objc_symbols,		\
		  ".objc_symbols\n", 0)		\
SECTION_FUNCTION (objc_module_info_section,	\
		  in_objc_module_info,		\
		  ".objc_module_info\n", 0)

#define SELECT_SECTION(exp)					\
  do								\
    {								\
      if (TREE_CODE (exp) == STRING_CST)			\
	{							\
	  if (flag_writable_strings)				\
	    data_section ();					\
	  else if (TREE_STRING_LENGTH (exp) !=			\
		   strlen (TREE_STRING_POINTER (exp)) + 1)	\
	    const_section ();					\
	  else							\
	    cstring_section ();					\
	}							\
      else if (TREE_CODE (exp) == INTEGER_CST			\
	       || TREE_CODE (exp) == REAL_CST)			\
        {							\
	  tree size = TYPE_SIZE (TREE_TYPE (exp));		\
	  							\
	  if (TREE_CODE (size) == INTEGER_CST &&		\
	      TREE_INT_CST_LOW (size) == 4 &&			\
	      TREE_INT_CST_HIGH (size) == 0)			\
	    literal4_section ();				\
	  else if (TREE_CODE (size) == INTEGER_CST &&		\
	      TREE_INT_CST_LOW (size) == 8 &&			\
	      TREE_INT_CST_HIGH (size) == 0)			\
	    literal8_section ();				\
	  else							\
	    const_section ();					\
	}							\
      else if (TREE_READONLY (exp) && !TREE_VOLATILE (exp))	\
	const_section ();					\
      else if (TREE_CODE (exp) == VAR_DECL &&					\
	       DECL_NAME (exp) &&						\
	       TREE_CODE (DECL_NAME (exp)) == IDENTIFIER_NODE &&		\
	       IDENTIFIER_POINTER (DECL_NAME (exp)) &&				\
	       !strncmp (IDENTIFIER_POINTER (DECL_NAME (exp)), "_OBJC_", 6))	\
	{									\
	  const char *name = IDENTIFIER_POINTER (DECL_NAME (exp));		\
	  									\
	  if (!strncmp (name, "_OBJC_CLASS_METHODS_", 20))			\
	    objc_cls_meth_section ();						\
	  else if (!strncmp (name, "_OBJC_INSTANCE_METHODS_", 23))		\
	    objc_inst_meth_section ();						\
	  else if (!strncmp (name, "_OBJC_CATEGORY_CLASS_METHODS_", 20))	\
	    objc_cat_cls_meth_section ();					\
	  else if (!strncmp (name, "_OBJC_CATEGORY_INSTANCE_METHODS_", 23))	\
	    objc_cat_inst_meth_section ();					\
	  else if (!strncmp (name, "_OBJC_CLASS_VARIABLES_", 22))		\
	    objc_class_vars_section ();						\
	  else if (!strncmp (name, "_OBJC_INSTANCE_VARIABLES_", 25))		\
	    objc_instance_vars_section ();					\
	  else if (!strncmp (name, "_OBJC_CLASS_", 12))				\
	    objc_class_section ();						\
	  else if (!strncmp (name, "_OBJC_METACLASS_", 16))			\
	    objc_meta_class_section ();						\
	  else if (!strncmp (name, "_OBJC_CATEGORY_", 15))			\
	    objc_category_section ();						\
	  else if (!strncmp (name, "_OBJC_STRINGS", 13))			\
	    objc_selector_strs_section ();					\
	  else if (!strncmp (name, "_OBJC_SELECTOR_REFERENCES", 25))		\
	    objc_selector_refs_section ();					\
	  else if (!strncmp (name, "_OBJC_SYMBOLS", 13))			\
	    objc_symbols_section ();						\
	  else if (!strncmp (name, "_OBJC_MODULES", 13))			\
	    objc_module_info_section ();					\
	  else									\
	    data_section ();							\
	}									\
      else									\
        data_section ();							\
    }										\
  while (0)

#define SELECT_RTX_SECTION(mode, rtx) const_section ()

/* Make sure jump tables go in the const section. */

#undef GO_IF_INDEXABLE_BASE(X, ADDR)
#define GO_IF_INDEXABLE_BASE(X, ADDR)	\
{ if (GET_CODE (X) == REG && REG_OK_FOR_BASE_P (X)) goto ADDR; }

#undef CASE_VECTOR_MODE
#define CASE_VECTOR_MODE SImode

#undef CASE_VECTOR_PC_RELATIVE

/* This is how to output an internal numbered label which
   labels a jump table.  */

#define ASM_OUTPUT_CASE_LABEL(FILE,PREFIX,NUM,JUMPTABLE)	\
  do { const_section ();					\
       ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM); } while (0)

/* Output at the end of a jump table.  */

#define ASM_OUTPUT_CASE_END(FILE,NUM,INSN)	\
  text_section ();
