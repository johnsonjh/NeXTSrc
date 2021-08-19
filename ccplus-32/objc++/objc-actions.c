/*
 *	objc-actions.c
 *	Copyright 1988, NeXT, Inc.
 *	Author: s.naroff
 *
 *      Purpose: This module implements the Objective-C 4.0 language.
 *
 *	GNU files that were modified:
 *
 *	compiler "config": 	tm-NeXT.h
 *	compiler "driver": 	gcc.c
 *	compiler "proper": 	c-parse.y, toplev.c, tree.[c,h,def]
 *                              c-typeck.c
 *
 *      All modifications are #ifdef'd by `NeXT_OBJC' which is enabled
 *      in the Makefile for the compiler...the only exceptions are the
 *      grammar changes (which don't get piped through cpp).
 *
 *      compatibility issues (with the Stepstone translator):
 *
 *	- does not recognize the following 3.3 constructs.
 *	  @requires, @classes, @messages, = (...)
 *	- methods with variable arguments must conform to ANSI standard.
 *	- tagged structure definitions that appear in BOTH the interface
 *	  and implementation are not allowed. 
 *      - public/private: all instance variables are public within the 
 *        context of the implementation...I consider this to be a bug in
 *        the translator.
 *      - statically allocated objects are not supported. the user will
 *        receive an error if this service is requested.
 *
 *      code generation `options':
 *
 *      - NeXT_SELS_R_CHAR_PTRS,   NeXT_SELS_R_INTS,	NeXT_SELS_R_STRUCTS,
 *        NeXT_SELS_R_STRUCT_PTRS
 */
#include <stdio.h>
#include "config.h"
#include "tree.h"
#ifdef NeXT_CPLUS
#include "cplus-parse.h"
#else
#include "c-parse.h"
#endif
#include "objc-actions.h"
#include "flags.h"

/* for encode_method_def */
#include "rtl.h"

#define OBJC_VERSION	2

#define NULLT	(tree) 0

#define NeXT_SELS_R_STRUCT_PTRS
#define NeXT_ENCODE_INLINE_DEFS 	0
#define NeXT_ENCODE_DONT_INLINE_DEFS	1

/*** Private Interface (procedures) ***/

/* code generation */

static void synth_module_prologue();
static void build_module_descriptor();
static tree   init_module_descriptor();
static void build_module_entry();
static void build_message_selector_pool();
static void build_selector_translation_table();
static tree build_ivar_chain(tree interface);

static tree build_ivar_template();
static tree build_method_template();
static tree build_private_template();
static void build_class_template();
static void build_category_template();
static tree build_super_template();

static void synth_forward_declarations();
static void generate_ivar_lists();
static void generate_dispatch_tables();
static void generate_shared_structures();

static tree build_msg_pool_reference();
static tree init_selector();
static tree build_keword_selector();
static tree synth_id_with_class_suffix(char *preamble);

/* misc. bookkeeping */

typedef struct hashedEntry 	*hash;
typedef struct hashedAttribute  *attr;

struct hashedAttribute {
        attr    next;
        tree    value;
};
struct hashedEntry {
        attr    list; 
	hash	next;
	tree 	key;
};
static void hash_init();
static void hash_enter(hash *hashList, tree method);
static hash hash_lookup(hash *hashList, tree sel_name);
static void hash_add_attr(hash entry, tree value);
static tree lookup_method();
static tree lookup_instance_method_static(tree interface, tree ident);
static tree lookup_class_method_static(tree interface, tree ident);
static tree add_class();
static int  add_selector_reference(tree anIdentifier);
static void add_class_reference(tree anIdentifier);
static int  add_objc_string(tree anIdentifier);

/* type encoding */

static void encode_aggregate(tree type, char *typeStr, int format);
static void encode_bitfield(int width, char *typeStr, int format);
static void encode_type(tree type, char *typeStr, int format);
static void encode_field_decl(tree field_decl, char *typeStr, int format);

static void reallyStartMethod(tree method, tree parmlist);
static int  comp_method_with_proto(tree method, tree proto);
static int  comp_proto_with_proto(tree proto1, tree proto2);
static tree getArgTypeList(tree meth, int context, int superflag);
static tree expr_last();

/* utilities for debugging and error diagnostics: */

static void warn_with_method(char *message, char mtype, tree method);
static void error_with_method(char *message, char mtype, tree method);
static void error_with_ivar(char *message, tree decl, tree rawdecl);
static char *genMethodDecl();
static char *genDeclaration();
static char *genDeclarator();
static int     isComplexDecl();
static void    adornDecl();
static void dump_interfaces();

/*** Private Interface (data) ***/

/* reserved tag definitions: */

#define TYPE_ID			"id"
#define TAG_OBJECT		"objc_object"
#define TAG_CLASS		"objc_class"
#define TAG_SUPER		"objc_super"
#define TAG_SELECTOR		"objc_selector"

#define _TAG_CLASS		"_objc_class"
#define _TAG_IVAR		"_objc_ivar"
#define _TAG_IVAR_LIST		"_objc_ivar_list"
#define _TAG_METHOD		"_objc_method"
#define _TAG_METHOD_LIST	"_objc_method_list"
#define _TAG_CATEGORY		"_objc_category"
#define _TAG_MODULE		"_objc_module"
#define _TAG_SYMTAB		"_objc_symtab"
#define _TAG_SUPER		"_objc_super"

/* set by `continue_class()' and checked by `is_public()' */

#define TREE_STATIC_TEMPLATE(record_type) (TREE_PUBLIC(record_type))
#define TYPED_OBJECT(type) \
       (TREE_CODE(type) == RECORD_TYPE && TREE_STATIC_TEMPLATE(type))

/* some commonly used instances of "identifier_node". */

static tree self_id, _cmd_id, _msg_id, _msgSuper_id; 
static tree objc_getClass_id, objc_getMetaClass_id;

static tree self_decl, _msg_decl, _msgSuper_decl;
static tree objc_getClass_decl, objc_getMetaClass_decl;

#ifdef NeXT_CPLUS
static tree super_type, _selector_type, id_type, objc_class_type;	
#else
static tree super_type, _selector_type, id_type, class_type;	
#endif

static tree instance_type;

static tree interface_chain = NULLT;

/* chains to manage selectors that are referenced and defined in the module */

static tree cls_ref_chain = NULLT;	/* classes referenced */
static tree sel_ref_chain = NULLT;	/* selectors referenced */
static tree sel_refdef_chain = NULLT;	/* selectors references & defined */
static int  max_selector_index;		/* total # of selector referenced */

/* hash tables to manage the global pool of method prototypes */

static hash *nst_method_hash_list = 0;
static hash *cls_method_hash_list = 0;

/* the following are used when compiling a class implementation.
 *  
 * implementation_template will normally be an anInterface, however if 
 * none exists this will be equal to implementation_context...it is
 * set in start_class().
 */

/* backend data declarations */

static tree _OBJC_SYMBOLS_decl;
static tree 	_OBJC_INSTANCE_VARIABLES_decl, _OBJC_CLASS_VARIABLES_decl;
static tree 	_OBJC_INSTANCE_METHODS_decl, _OBJC_CLASS_METHODS_decl;
static tree 	_OBJC_CLASS_decl, _OBJC_METACLASS_decl;
static tree _OBJC_MODULES_decl;
static tree _OBJC_STRINGS_decl;

static tree implementation_context = NULLT, 
	    implementation_template = NULLT;

struct imp_entry {
	struct imp_entry *next;
	tree imp_context;
	tree imp_template;
	tree class_decl; 	/* _OBJC_CLASS_<my_name>; */
	tree meta_decl;  	/* _OBJC_METACLASS_<my_name>; */
};
static struct imp_entry *imp_list = 0;
static int imp_count = 0;	/* `@implementation' */
static int cat_count = 0;	/* `@category' */

static tree objc_class_template, objc_category_template, _PRIVATE_record;
static tree _clsSuper_ref, __clsSuper_ref;

static tree objc_method_template, objc_ivar_template;
static tree objc_symtab_template, objc_module_template;
static tree objc_super_template, objc_object_reference;
#ifdef NeXT_SELS_R_STRUCTS
static tree objc_selector_template;
#endif

static tree objc_object_id;
static tree _OBJC_SUPER_decl;

static tree method_context = NULLT;
static int  method_slot = 0;	/* used by start_method_def() */

#define BUFSIZE		512

static char *errbuf;	/* a buffer for error diagnostics */
static char *utlbuf;	/* a buffer for general utility */

/*  
 *  Data Models - currently defined in "tree.def" & "tree.h".
 *
 *  DEFTREECODE(KEYWORD_DECL, "keyword_decl", "o", 0)
 *
 *    struct objc_tree_keyword_decl
 *	{
 *	  char common[sizeof (struct tree_common)];
 *	  union tree_node *arg_name;
 *	  union tree_node *key_name;
 *	}
 *
 *  DEFTREECODE(INSTANCE_METHOD_DECL, "instance_method_decl", "o", 0)
 *  DEFTREECODE(CLASS_METHOD_DECL, "class_method_decl", "o", 0)
 *
 *  sel_name...an identifier_node that represents the full selector name.
 *  sel_args...a chain of keyword_decls (for keyword selectors).
 *  add_args...a tree list node generated by `get_parm_info()'. 
 *  mth_defn...a function_decl, set by start_method_def().
 *
 *    struct objc_tree_method_decl
 *	{
 *	  char common[sizeof (struct tree_common)];
 *        char *filename;
 *        int linenum;
 *	  union tree_node *sel_name;
 *	  union tree_node *sel_args;  
 *	  union tree_node *add_args;
 *	  union tree_node *mth_defn;  
 *	}
 *
 *  DEFTREECODE(INTERFACE_TYPE, "interface_type", "o", 0)
 *  DEFTREECODE(IMPLEMENTATION_TYPE, "implementation_type", "o", 0)
 *
 *    struct objc_tree_class_type
 *	{
 *	  char common[sizeof (struct tree_common)];
 *	  union tree_node *my_name;
 *	  union tree_node *super_name;
 *	  union tree_node *ivar_decls;
 *        union tree_node *raw_ivars;
 *	  union tree_node *nst_method_chain;
 *	  union tree_node *cls_method_chain;
 *        union tree_node *static_template;
 *	}
 *
 *    union tree_node
 *      {
 *        struct tree_common common;
 *        struct tree_int_cst int_cst;
 *        struct tree_real_cst real_cst;
 *        struct tree_string string;
 *        struct tree_complex complex;
 *        struct tree_identifier identifier;
 *        struct tree_decl decl;
 *        struct tree_type type;
 *        struct tree_list list;
 *        struct tree_exp exp;
 *        struct tree_stmt stmt;
 *        struct tree_if_stmt if_stmt;
 *        struct tree_bind_stmt bind_stmt;
 *        struct tree_case_stmt case_stmt;
 *
 *        Objective-C nodes... 
 *
 *        struct objc_tree_keyword_decl keyword;
 *        struct objc_tree_method_decl method;
 *        struct objc_tree_class_type class;
 *      };
 */

/* services imported from "tree.c" */

extern struct obstack permanent_obstack, *current_obstack,  *rtl_obstack;

extern tree make_node(enum tree_code code);
extern tree copy_node(tree node);
extern tree get_identifier(char *text);
extern tree chainon(tree op1, tree op2);
extern tree tree_cons(tree purpose, tree value, tree chain);
extern tree perm_tree_cons(tree purpose, tree value, tree chain);
extern tree build_tree_list(tree purpose, tree value);
extern tree nreverse(tree t);
extern tree build(/*enum tree_code code, tree type, tree op1, ...*/);
extern tree build_nt(/*enum tree_code code, tree op1, ...*/);
extern tree build_string(/*int len, char *str*/);
extern tree build_index_type(tree maxval);

/* services imported from "c-decl.c" */

extern void finish_decl(tree decl, tree init, tree asmspec);

extern void pushlevel(int tag_transparent);
extern tree poplevel(/*int keep, int reverse, int functionbody*/);
extern tree getdecls();
extern tree pushdecl(tree x);		/* only used by start_method_def() */
extern void store_parm_decls();		/* for "old-style" function defs */

extern tree groktypename(tree list);
extern tree groktypename_inParmContext(tree list); /* NeXT mod to c-decl.c */

/* the basic data types in `C' */
extern tree short_integer_type_node, integer_type_node, 
	    long_integer_type_node, short_unsigned_type_node,	
	    unsigned_type_node, long_unsigned_type_node,
	    unsigned_char_type_node, signed_char_type_node,	
	    char_type_node, float_type_node, double_type_node,
	    long_double_type_node, string_type_node, void_type_node,
	    int_array_type_node, char_array_type_node;

#ifdef NeXT_CPLUS

/* TYPE_NAME(record_type) points to a TYPE_DECL node as oppossed to an
 * IDENTIFIER_NODE. The name can be obtained using the following macro:
 *
 * 	IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(record_type)))
 *
 * ONLY if NOT exclosed in a `extern "C"' linkage directive... 
 */
#include "cplus-tree.h"

#define CPLUS_TYPE_NAME(type) \
	( (TREE_CODE(TYPE_NAME(type)) == TYPE_DECL) \
		? DECL_NAME(TYPE_NAME(type)) \
		: TYPE_NAME(type))

/* define in cplus-class.c */
extern tree lang_name_c; 
extern void push_lang_context(tree), pop_lang_context();

extern tree class_type_node, record_type_node, union_type_node, enum_type_node;

extern tree make_anon_name();
extern tree xref_tag(tree code_type_node, tree name, tree basetype_info);
extern tree grokfield (tree declarator, tree declspecs, tree width, tree init, tree asmspec);
extern tree finish_struct (tree t, tree list_of_fieldlists, int empty, int warn_anon);
extern int start_function (tree declspecs, tree declarator, tree raises, 
				int pre_parsed_p);
extern int comptypes(tree type1, tree type2, int strict); 

extern tree start_decl(tree declarator, tree declspecs, int initialized,
			tree raises);

extern tree build_component_ref(tree datum, tree component, 
			tree basetype_path, int protect);

extern void finish_function(int lineno, int call_poplevel);

static tree objc_xref_tag(enum tree_code code, tree name)
{
	return xref_tag(record_type_node, name, 0);	
}

static tree objc_grokfield(char *filename, int line, 
                      tree declarator, tree declspecs, tree width)
{
	return grokfield(declarator, declspecs, width, 0, 0);
}

/* this code is intended to mimic the `structsp' production in the parser */

static int cplus_struct_hack;

static tree objc_finish_struct(tree t, tree fieldlist)
{
	tree s = finish_struct(t,  
			build_tree_list((tree)visibility_default, fieldlist), 
			1, 0 /* dont warn for untagged struct templates */);
	
	if (cplus_struct_hack & 1)
		    resume_temporary_allocation ();
	if (cplus_struct_hack & 2)
		    resume_momentary (1);
		    
	return s;
}

tree start_struct(enum tree_code code, tree name) 
{ 
	tree s = xref_tag(record_type_node, name ? name : make_anon_name(), 0);
	
	/* simulate `LC' production */
	int temp = allocation_temporary_p ();
	int momentary = suspend_momentary ();
	
	if (temp)
		end_temporary_allocation ();
	cplus_struct_hack = (momentary << 1) | temp;
	pushclass (s, 0); 
	
	return s;	
}

tree push_parm_decl(tree parmlist, tree parm) 
{
	if (parmlist)
		return chainon(parmlist, build_tree_list(0 /* no init */, parm));
	else
		return build_tree_list(0 /* no init */, parm);
}

tree get_parm_info(int void_at_end, tree parmlist) 
{		      
    TREE_PARMLIST(parmlist) = 1;

    if (void_at_end)
#ifdef NeXT_CPLUS
       chainon (parmlist, void_list_node);
#else
       chainon (parmlist, build_tree_list (NULL_TREE, void_type_node));
#endif
       
    return parmlist;
}

#else

#include "c-parse.h"

#define objc_xref_tag 	xref_tag

extern tree xref_tag(enum tree_code code, tree name);
extern void push_parm_decl(tree parm);	/* for function prototypes */
extern tree get_parm_info(int void_at_end);
extern tree start_struct(enum tree_code code, tree name);
extern tree grokfield(char *filename, int line, 
                      tree delclarator, tree declspecs, tree width);
extern tree finish_struct(tree t, tree fieldlist);
extern int  start_function(tree declspecs, tree declarator);
extern int  comptypes(tree type1, tree type2); 
extern void finish_function(int lineno);

extern tree start_decl(tree declarator, tree declspecs, int initialized);
extern tree build_component_ref(tree datum, tree component);
#endif

/* handle to the function currently being compiled */
extern tree current_function_decl;

/* services imported from "c-typeck.c" */

extern tree build_function_call(tree function, tree params);
extern tree build_array_ref(tree array, tree index);
extern tree build_indirect_ref(tree ptr, char *errorstring);
extern tree build_unary_op(enum tree_code code, tree xarg, int convert);
extern tree build_modify_expr(tree lhs, enum tree_code code, tree rhs);
extern tree build_compound_expr(tree list);
extern void c_expand_return(tree retval);

/* services imported from "varasm.c" */

extern void assemble_asm(tree string);

/* services imported from "final.c" */

extern void app_disable();

/* services imported from "toplev.c" */

extern int extra_warnings;
extern void warning();
extern void error(), error_with_decl();

/* data imported from "c-parse.y" */

extern char *input_filename;	
extern int lineno;	
extern tree ridpointers[];

extern char *strcpy(),*strcat();

static tree define_decl(tree declarator, tree declspecs)
{
#ifdef NeXT_CPLUS
  tree decl = start_decl(declarator, declspecs, 0, NULLT);
#else
  tree decl = start_decl(declarator, declspecs, 0);
#endif
  finish_decl(decl, NULLT, NULLT);
  return decl;
}

/* 
 * rules for statically typed objects...called from `c-typeck.comptypes()'.
 *
 * an assignment of the form `a' = `b' is permitted if:
 *
 *   - `a' is of type "id".
 *   - `a' and `b' are the same class type.
 *   - `a' and `b' are of class types A and B such that B is a descendant
 *     of A.
 */

int objc_comptypes(tree lhs, tree rhs)
{
  /* `id' = `<class> *', `<class> *' = `id' */

#ifdef NeXT_CPLUS
  if (((TYPE_NAME(lhs) && DECL_NAME(TYPE_NAME(lhs)) == objc_object_id) && 
			TYPED_OBJECT(rhs)) ||
      (TYPED_OBJECT(lhs) && 
	(TYPE_NAME(rhs) && DECL_NAME(TYPE_NAME(rhs)) == objc_object_id)))
#else
  if ((TYPE_NAME(lhs) == objc_object_id && TYPED_OBJECT(rhs)) ||
      (TYPED_OBJECT(lhs) && TYPE_NAME(rhs) == objc_object_id))
#endif
    return 1;

  /* `<class> *' = `<class> *' */

  else if (TYPED_OBJECT(lhs) && TYPED_OBJECT(rhs))
    {
    tree lname = TYPE_NAME(lhs), rname = TYPE_NAME(rhs);

    if (lname == rname)
      return 1;
    else
      {
      /* if the left hand side is a super class of the right hand side,
         allow it... 
       */
      tree rinter = lookup_interface(rname);

      while (rinter)
        {
        if (lname == rinter->class.super_name)
          return 1;

        rinter = lookup_interface(rinter->class.super_name);
        }

      return 0;
      }
    }
  else
    return -1;	/* not an Objective-C type */
}

/* called from `toplev.rest_of_decl_compilation()' */

void objc_check_decl(tree aDecl)
{
  tree type = TREE_TYPE(aDecl);
  static int alreadyWarned = 0;

  if (TREE_CODE(type) == RECORD_TYPE && TREE_STATIC_TEMPLATE(type))
    {
    if (!alreadyWarned)
      {
      error("NeXT compiler does not support statically allocated objects");
      alreadyWarned = 1;
      }
    error_with_decl(aDecl,"`%s' cannot be statically allocated");
    }
}

/* implement static typing. at this point, we know we have an interface... */

tree get_static_reference(tree interface)
{
  return objc_xref_tag(RECORD_TYPE, interface->class.my_name);
}

#ifdef NeXT_SELS_R_STRUCTS
/*
 *	struct objc_selector {
 *		unsigned int SEL;
 *	};
 */		
static void build_objc_selector_template()
{
  tree decl_specs, field_decl, field_decl_chain;

  objc_selector_template = start_struct(RECORD_TYPE,
					get_identifier(TAG_SELECTOR));

  /* unsigned int SEL; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_UNSIGNED]);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  field_decl = get_identifier("SEL");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  finish_struct(objc_selector_template, field_decl_chain);
}
#endif

/*
 *	purpose: "play" parser, creating/installing representations
 *		 of the declarations that are required by Objective-C.
 *
 *	model:
 *
 *		type_spec--------->sc_spec
 *		(tree_list)        (tree_list)
 *		    |                  |
 *		    |                  |
 *		identifier_node    identifier_node
 */
static void synth_module_prologue()
{
  tree sc_spec, type_spec, decl_specs, expr_decl, parms, record;
#ifdef NeXT_CPLUS
  tree parmlist;
#endif

  /* defined in `objc.h' */
  objc_object_id = get_identifier(TAG_OBJECT);

  objc_object_reference = objc_xref_tag(RECORD_TYPE, objc_object_id);

  id_type = groktypename(build_tree_list(
				build_tree_list(NULLT, objc_object_reference), 
				build(INDIRECT_REF,NULLT,NULLT)));

#ifdef NeXT_CPLUS
  objc_class_type = groktypename(build_tree_list(build_tree_list(NULLT, 
			objc_xref_tag(RECORD_TYPE, get_identifier(TAG_CLASS))),
			build(INDIRECT_REF,NULLT,NULLT)));
#else
  class_type = groktypename(build_tree_list(build_tree_list(NULLT, 
			objc_xref_tag(RECORD_TYPE, get_identifier(TAG_CLASS))),
			build(INDIRECT_REF,NULLT,NULLT)));
#endif

#ifdef NeXT_SELS_R_STRUCTS
  build_objc_selector_template ();
#endif

/* Declare SEL type before prototypes for objc_msgSend(), or else those
   struct tags are considered local to the prototype and won't match the one
   in <objc/objc-runtime.h>. */
#ifdef NeXT_SELS_R_INDIRECT
  /* `char **' */
  _selector_type = groktypename(build_tree_list(
		build_tree_list(NULLT,ridpointers[RID_CHAR]),
		build(INDIRECT_REF,NULLT, build(INDIRECT_REF,NULLT,NULLT))));
#else

#ifdef NeXT_SELS_R_INTS
  /* `unsigned int' */
  _selector_type = unsigned_type_node;
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  /* `struct objc_selector *' */
  _selector_type = groktypename(build_tree_list(
		build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
						get_identifier(TAG_SELECTOR))),
		build(INDIRECT_REF,NULLT,NULLT)));
#endif

#endif

  /* forward declare type...or else the prototype for `super' will bitch */
  groktypename(build_tree_list(build_tree_list(NULLT,
			objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SUPER))),
				build(INDIRECT_REF,NULLT,NULLT)));

#ifdef NeXT_CPLUS
  push_lang_context(lang_name_c);
#endif

  _msg_id = get_identifier("objc_msgSend");
  _msgSuper_id = get_identifier("objc_msgSendSuper");
  objc_getClass_id = get_identifier("objc_getClass");
  objc_getMetaClass_id = get_identifier("objc_getMetaClass");

  /* struct objc_object *objc_msgSend(id, SEL, ...); */
#ifndef NeXT_CPLUS
  pushlevel(0);
#endif
 
  decl_specs = build_tree_list(NULLT, objc_object_reference);

#ifdef NeXT_CPLUS
  parmlist = push_parm_decl(0, build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
#else
  push_parm_decl(build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
#endif

#ifdef NeXT_SELS_R_INTS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_UNSIGNED]);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  expr_decl = NULLT;
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  expr_decl = build(INDIRECT_REF, NULLT, NULLT);
#endif

#ifdef NeXT_SELS_R_STRUCTS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  expr_decl = NULLT;
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  expr_decl = build(INDIRECT_REF, NULLT, NULLT);
#endif

#ifdef NeXT_CPLUS
  parmlist = push_parm_decl(parmlist, build_tree_list(decl_specs, NULLT));
  parms = get_parm_info(0, parmlist);
#else
  push_parm_decl(build_tree_list(decl_specs, NULLT));
  parms = get_parm_info(0);
  poplevel(0,0,0);
#endif

  decl_specs = build_tree_list(NULLT, objc_object_reference);
  expr_decl = build_nt(CALL_EXPR, _msg_id, parms, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);

  _msg_decl = define_decl(expr_decl, decl_specs);

  /* struct objc_object *objc_msgSendSuper(struct objc_super *, SEL, ...); */
#ifndef NeXT_CPLUS
  pushlevel(0);
#endif
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
				get_identifier(TAG_SUPER)));
#ifdef NeXT_CPLUS
  parmlist = push_parm_decl(0, build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
#else
  push_parm_decl(build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
#endif

#ifdef NeXT_SELS_R_INTS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_UNSIGNED]);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  expr_decl = NULLT;
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  expr_decl = build(INDIRECT_REF, NULLT, NULLT);
#endif

#ifdef NeXT_SELS_R_STRUCTS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  expr_decl = NULLT;
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  expr_decl = build(INDIRECT_REF, NULLT, NULLT);
#endif

#ifdef NeXT_CPLUS
  parmlist = push_parm_decl(parmlist, build_tree_list(decl_specs, NULLT));
  parms = get_parm_info(0, parmlist);	
#else
  push_parm_decl(build_tree_list(decl_specs, NULLT));
  parms = get_parm_info(0);	
  poplevel(0,0,0);
#endif

  expr_decl = build_nt(CALL_EXPR, _msgSuper_id, parms, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);

  _msgSuper_decl = define_decl(expr_decl, decl_specs);

  /* id objc_getClass(); */
#ifdef NeXT_CPLUS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  parmlist = push_parm_decl(0, build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
  parms = get_parm_info(1/*not variable length*/, parmlist);	
#else
  parms = build_tree_list(NULLT, NULLT);
#endif

  decl_specs = build_tree_list(NULLT, objc_object_reference);
  expr_decl = build_nt(CALL_EXPR, objc_getClass_id, parms, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);

  objc_getClass_decl = define_decl(expr_decl, decl_specs);

  /* id objc_getMetaClass(); */
#ifdef NeXT_CPLUS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  parmlist = push_parm_decl(0, build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,NULLT)));
  parms = get_parm_info(1/*not variable length*/, parmlist);	
#else
  parms = build_tree_list(NULLT, NULLT);
#endif
  expr_decl = build_nt(CALL_EXPR, objc_getMetaClass_id, parms, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);

  decl_specs = build_tree_list(NULLT, objc_object_reference);
  objc_getMetaClass_decl = define_decl(expr_decl, decl_specs);

#ifdef NeXT_CPLUS
  pop_lang_context();
#endif

#if 0
  _OBJC_SELECTOR_REFERENCES_id = get_identifier("_OBJC_SELECTOR_REFERENCES");

#ifdef NeXT_SELS_R_INTS

  /* extern unsigned int selTransTbl[]; */
  sc_spec = tree_cons(NULLT, ridpointers[RID_EXTERN], NULLT);
  decl_specs = tree_cons(NULLT, ridpointers[RID_UNSIGNED], sc_spec);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  expr_decl = build_nt(ARRAY_REF, _OBJC_SELECTOR_REFERENCES_id, NULLT);
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS

  /* extern char *selTransTbl[]; */
  sc_spec = tree_cons(NULLT, ridpointers[RID_EXTERN], NULLT);
  decl_specs = tree_cons(NULLT, ridpointers[RID_CHAR], sc_spec);

  expr_decl = build_nt(ARRAY_REF, _OBJC_SELECTOR_REFERENCES_id, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);
#endif

#ifdef NeXT_SELS_R_STRUCTS

  /* extern struct objc_selector selTransTbl[]; */
  sc_spec = tree_cons(NULLT, ridpointers[RID_EXTERN], NULLT);
  decl_specs = tree_cons(NULLT,
			 objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SELECTOR)),
			 sc_spec);

  expr_decl = build_nt(ARRAY_REF, _OBJC_SELECTOR_REFERENCES_id, NULLT);
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS

  /* extern struct objc_selector *selTransTbl[]; */
  sc_spec = tree_cons(NULLT, ridpointers[RID_EXTERN], NULLT);
  decl_specs = tree_cons(NULLT,
			 objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SELECTOR)),
			 sc_spec);

  expr_decl = build_nt(ARRAY_REF, _OBJC_SELECTOR_REFERENCES_id, NULLT);
  expr_decl = build(INDIRECT_REF, NULLT, expr_decl);
#endif

  _OBJC_SELECTOR_REFERENCES_decl = define_decl(expr_decl, decl_specs);

  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_SELECTOR_REFERENCES_decl) = 1;
#endif
}

/*
 * custom "build_string()" which sets TREE_TYPE!
 */
static tree my_build_string(int len, char *str)
{
  int wide_flag = 0;
  tree aString = build_string(len,str);
  /*
   *  some code from "combine_strings()", which is local to c-parse.y.
   */
  if (TREE_TYPE(aString) == int_array_type_node)
    wide_flag = 1;
  
  TREE_TYPE(aString) = 
      build_array_type (wide_flag ? integer_type_node : char_type_node,
		        build_index_type(build_int_2(len - 1, 0)));

  TREE_LITERAL(aString) = 1;	/* puts string in the ".text" segment */
  TREE_STATIC(aString) = 1;

  return aString;
}

/*
 *	struct objc_symtab {
 *		long sel_ref_cnt;
 *		char *refs;
 *		long cls_def_cnt;
 *		long cat_def_cnt;
 *		void *defs[cls_def_cnt+cat_def_cnt];
 *	};
 */		
static void build_objc_symtab_template()
{
  tree decl_specs, field_decl, field_decl_chain;

  objc_symtab_template = start_struct(RECORD_TYPE, get_identifier(_TAG_SYMTAB));

  /* long sel_ref_cnt; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("sel_ref_cnt");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

#ifdef NeXT_SELS_R_INTS
  /* unsigned int *sel_ref; */
  decl_specs = build_tree_list(NULLT, ridpointers[RID_UNSIGNED]);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("refs"));
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS
  /* char  **sel_ref; */
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("refs"));
  field_decl = build(INDIRECT_REF, NULLT, field_decl);
#endif

#ifdef NeXT_SELS_R_STRUCTS
  /* struct objc_selector **sel_ref; */
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("refs"));
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  /* struct objc_selector **sel_ref; */
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("refs"));
  field_decl = build(INDIRECT_REF, NULLT, field_decl);
#endif

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* short cls_def_cnt; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_SHORT]);
  field_decl = get_identifier("cls_def_cnt");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* short cat_def_cnt; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_SHORT]);
  field_decl = get_identifier("cat_def_cnt");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* void *defs[cls_def_cnt+cat_def_cnt]; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_VOID]);
  field_decl = build_nt(ARRAY_REF, get_identifier("defs"),
			build_int_2(imp_count+cat_count,0));
  field_decl = build(INDIRECT_REF, NULLT, field_decl);
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_symtab_template, field_decl_chain);
}

static tree init_def_list()
{
  tree expr, initlist = NULLT;
  struct imp_entry *impent;

  if (imp_count)
    for (impent = imp_list; impent; impent = impent->next)
      {
      if (TREE_CODE(impent->imp_context) == IMPLEMENTATION_TYPE) 
        {
        expr = build_unary_op(ADDR_EXPR,impent->class_decl,0);
        initlist = tree_cons(NULLT, expr, initlist);
        }
      }

  if (cat_count)
    for (impent = imp_list; impent; impent = impent->next)
      {
      if (TREE_CODE(impent->imp_context) == CATEGORY_TYPE) 
        {
        expr = build_unary_op(ADDR_EXPR,impent->class_decl,0);
        initlist = tree_cons(NULLT, expr, initlist);
        }
      }
  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

/*
 *	struct objc_symtab {
 *		long sel_ref_cnt;
 *		char *refs;
 *		long cls_def_cnt;
 *		long cat_def_cnt;
 *		void *defs[cls_def_cnt+cat_def_cnt];
 *	};
 */		
static tree init_objc_symtab()
{
  tree initlist;

  /* sel_ref_cnt = { ..., 5, ... } */

  if (sel_ref_chain)
    initlist = build_tree_list(NULLT,build_int_2(max_selector_index,0));
  else
    initlist = build_tree_list(NULLT,build_int_2(0,0));

  /* THIS FIELD IS VESTIGIAL! refs = { ..., _OBJC_SELECTOR_REFERENCES, ... } */

  initlist = tree_cons(NULLT,build_int_2(0,0),initlist);

  /* cls_def_cnt = { ..., 5, ... } */
  
  initlist = tree_cons(NULLT,build_int_2(imp_count,0),initlist);

  /* cat_def_cnt = { ..., 5, ... } */
  
  initlist = tree_cons(NULLT,build_int_2(cat_count,0),initlist);

  /* cls_def = { ..., { &Foo, &Bar, ...}, ... } */

  if (imp_count || cat_count)
    initlist = tree_cons(NULLT, init_def_list(),initlist);

  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

static void forward_declare_categories()
{
  struct imp_entry *impent;
  tree sav = implementation_context;
  for (impent = imp_list; impent; impent = impent->next)
    {
    if (TREE_CODE(impent->imp_context) == CATEGORY_TYPE) 
      {
      tree sc_spec, decl_specs, decl;

      sc_spec = build_tree_list(NULLT, ridpointers[RID_EXTERN]);
      decl_specs = tree_cons(NULLT, objc_category_template, sc_spec);

      implementation_context = impent->imp_context;
      impent->class_decl = define_decl(
				synth_id_with_class_suffix("_OBJC_CATEGORY"),
				decl_specs);
      }
    }
  implementation_context = sav;
}

static void generate_objc_symtab_decl()
{
  tree sc_spec;

  if (!objc_category_template)
    build_category_template();

  /* forward declare categories */
  if (cat_count)
    forward_declare_categories();

  if (!objc_symtab_template)
    build_objc_symtab_template();

  sc_spec = build_tree_list(NULLT, ridpointers[RID_STATIC]);

#ifdef NeXT_CPLUS
  _OBJC_SYMBOLS_decl = start_decl(get_identifier("_OBJC_SYMBOLS"), 
			 tree_cons(NULLT, objc_symtab_template, sc_spec), 1, NULLT);
#else
  _OBJC_SYMBOLS_decl = start_decl(get_identifier("_OBJC_SYMBOLS"), 
			 tree_cons(NULLT, objc_symtab_template, sc_spec), 1);
#endif

  finish_decl(_OBJC_SYMBOLS_decl, init_objc_symtab(), NULLT);
  
  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_SYMBOLS_decl) = 1;
}

/*
 *	tree_node------->tree_node----->...
 *          |                |
 *          | (value)        | (value)
 *          |                |
 *	  expr             expr
 */
static tree init_module_descriptor()
{
  tree initlist, expr;

  /* version = { 1, ... } */

  expr = build_int_2(OBJC_VERSION,0);
  initlist = build_tree_list(NULLT, expr);

  /* size = { ..., sizeof(struct objc_module), ... } */

  expr = build_int_2(TREE_INT_CST_LOW(TYPE_SIZE(objc_module_template)),0);
  initlist = tree_cons(NULLT,expr,initlist);

  /* name = { ..., "foo.m", ... } */

  expr = build_msg_pool_reference(
		add_objc_string(get_identifier(input_filename)));
  initlist = tree_cons(NULLT,expr,initlist);

  /* symtab = { ..., _OBJC_SYMBOLS, ... } */

  if (_OBJC_SYMBOLS_decl)
    expr = build_unary_op(ADDR_EXPR,_OBJC_SYMBOLS_decl,0);
  else
    expr = build_int_2(0,0);
  initlist = tree_cons(NULLT,expr,initlist);

  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

/*
 *  struct objc_module { ... } _OBJC_MODULE = { ... };
 */
static void build_module_descriptor()
{
  tree decl_specs, field_decl, field_decl_chain;

  objc_module_template = start_struct(RECORD_TYPE, get_identifier(_TAG_MODULE));

  /* long version; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("version");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* long  size; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("size");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* char  *name; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("name"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_symtab *symtab; */

  decl_specs = get_identifier(_TAG_SYMTAB);
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, decl_specs));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("symtab"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_module_template, field_decl_chain);

  /* create an instance of "objc_module" */

  decl_specs = tree_cons(NULLT,objc_module_template,
  			build_tree_list(NULLT, ridpointers[RID_STATIC]));

#ifdef NeXT_CPLUS
  _OBJC_MODULES_decl = start_decl(get_identifier("_OBJC_MODULES"), 
			decl_specs, 1, NULLT);
#else
  _OBJC_MODULES_decl = start_decl(get_identifier("_OBJC_MODULES"), 
			decl_specs, 1);
#endif

  finish_decl(_OBJC_MODULES_decl, init_module_descriptor(), NULLT);
  
  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_MODULES_decl) = 1;
}

/* extern const char _OBJC_STRINGS[]; */

static void generate_forward_declaration_to_string_table()
{
    tree sc_spec, decl_specs, expr_decl;

    sc_spec = tree_cons(NULLT, ridpointers[RID_EXTERN], NULLT);
    decl_specs = tree_cons(NULLT, ridpointers[RID_CHAR], sc_spec);

    expr_decl = build_nt(ARRAY_REF, get_identifier("_OBJC_STRINGS"), NULLT);

    _OBJC_STRINGS_decl = define_decl(expr_decl, decl_specs);

  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_STRINGS_decl) = 1;
}

/* static char _OBJC_STRINGS[] = "..."; */

static void build_message_selector_pool()
{
  tree sc_spec, decl_specs, expr_decl;
  tree chain, string_expr;
  int goolengthtmp = 0, msg_pool_size = 0;
  char *string_goo;

  sc_spec = tree_cons(NULLT, ridpointers[RID_STATIC], NULLT);
  decl_specs = tree_cons(NULLT, ridpointers[RID_CHAR], sc_spec);

  expr_decl = build_nt(ARRAY_REF, get_identifier("_OBJC_STRINGS"), NULLT);

#ifdef NeXT_CPLUS
  _OBJC_STRINGS_decl = start_decl(expr_decl, decl_specs, 1, NULLT);
#else
  _OBJC_STRINGS_decl = start_decl(expr_decl, decl_specs, 1);
#endif

  for (chain = sel_refdef_chain; chain; chain = TREE_CHAIN(chain))
      msg_pool_size += IDENTIFIER_LENGTH(TREE_VALUE(chain)) + 1;

  msg_pool_size++;

  string_goo = (char *)malloc(msg_pool_size);
  bzero(string_goo, msg_pool_size);

  for (chain = sel_refdef_chain; chain; chain = TREE_CHAIN(chain))
    {
      strcpy(string_goo+goolengthtmp,IDENTIFIER_POINTER(TREE_VALUE(chain)));
      goolengthtmp += IDENTIFIER_LENGTH(TREE_VALUE(chain)) + 1;
    }

  string_expr = my_build_string(msg_pool_size, string_goo);

  finish_decl(_OBJC_STRINGS_decl, string_expr, NULLT);
}

/*
 * synthesize the following expr: (char *)&_OBJC_STRINGS[<offset>]
 *
 * the cast stops the compiler from issuing the following message:
 *
 * grok.m: warning: initialization of non-const * pointer from const *
 * grok.m: warning: initialization between incompatible pointer types
 */
static tree build_msg_pool_reference(int offset)
{
    tree expr = build_int_2(offset,0);
    tree cast;

    expr = build_array_ref(_OBJC_STRINGS_decl, expr);
    expr = build_unary_op(ADDR_EXPR,expr,0);

    cast = build_tree_list(build_tree_list(NULLT,ridpointers[RID_CHAR]),
		           build(INDIRECT_REF,NULLT,NULLT));
    TREE_TYPE(expr) = groktypename(cast);
    return expr;
}

static tree build_selector_reference(int idx)
{
  tree ref, decl, name, ident;
  char buf[256];
  struct obstack *save_current_obstack = current_obstack;
  struct obstack *save_rtl_obstack = rtl_obstack;

  sprintf(buf,"_OBJC_SELECTOR_REFERENCES_%d",idx);

  /* new stuff */
  rtl_obstack = current_obstack = &permanent_obstack;
  ident = get_identifier(buf);

  if (IDENTIFIER_GLOBAL_VALUE(ident))
    decl = IDENTIFIER_GLOBAL_VALUE(ident); /* set by pushdecl() */
  else 
    {
    decl = build_decl(VAR_DECL, ident, _selector_type);
    TREE_EXTERNAL(decl) = 1;
    TREE_PUBLIC(decl) = 1;
    TREE_USED(decl) = 1;

    make_decl_rtl(decl, 0, 1); /* usually called from `rest_of_decl_compilation' */
    pushdecl_top_level(decl);  /* our `extended/custom' pushdecl in c-decl.c */
    }
  current_obstack = save_current_obstack;
  rtl_obstack = save_rtl_obstack;

  return decl;
}

static tree init_selector(int offset)
{
    tree expr = build_msg_pool_reference(offset);
    TREE_TYPE(expr) = _selector_type; /* cast */
    return expr;
}

static void build_selector_translation_table()
{

  tree sc_spec, decl_specs, decl, var_decl;
  tree chain;
  int offset = 0, idx = 0;
  char buf[256];

  for (chain = sel_ref_chain; chain; chain = TREE_CHAIN(chain))
    {
    tree expr;

    sc_spec = build_tree_list(NULLT, ridpointers[RID_STATIC]);

    decl_specs =  tree_cons(NULLT, 
		    objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SELECTOR)),
		    sc_spec);

    sprintf(buf,"_OBJC_SELECTOR_REFERENCES_%d",idx);
    var_decl = build(INDIRECT_REF, NULLT, get_identifier(buf));

    /* the `decl' that is returned from start_decl is the one that we
     * forwarded declared in `build_selector_reference()'
     */
#ifdef NeXT_CPLUS
    decl = start_decl(var_decl, decl_specs, 1, NULLT); 
#else
    decl = start_decl(var_decl, decl_specs, 1); 
#endif

    expr = init_selector(offset);

    /* add one for the '\0' character */
    offset += IDENTIFIER_LENGTH(TREE_VALUE(chain)) + 1;
    idx++;

    finish_decl(decl, expr, NULLT);

    }
}

static void 
add_class_reference(tree anIdentifier)
{
  tree chain;

  if (chain = cls_ref_chain)
    {
      tree tail;
      do
        {
        if (anIdentifier == TREE_VALUE(chain))
	  return;

        tail = chain;
        chain = TREE_CHAIN(chain);
        }
      while (chain);

      /* append to the end of the list */
      TREE_CHAIN(tail) = perm_tree_cons(NULLT, anIdentifier, NULLT);
    }
  else
    cls_ref_chain = perm_tree_cons(NULLT, anIdentifier, NULLT);
}

/*
 * sel_ref_chain is a list whose "value" fields will be instances of
 * identifier_node that represent the selector.
 */
static int 
add_selector_reference(tree anIdentifier)
{
  tree chain;
  int index = 0;

  /* this adds it to sel_refdef_chain, the global pool of selectors */
  add_objc_string(anIdentifier);

  if (chain = sel_ref_chain)
    {
      tree tail;
      do
        {
        if (anIdentifier == TREE_VALUE(chain))
	  return index;

        index++;
        tail = chain;
        chain = TREE_CHAIN(chain);
        }
      while (chain);

      /* append to the end of the list */
      TREE_CHAIN(tail) = perm_tree_cons(NULLT, anIdentifier, NULLT);
    }
  else
    sel_ref_chain = perm_tree_cons(NULLT, anIdentifier, NULLT);

  max_selector_index++;
  return index;
}

/*
 * sel_refdef_chain is a list whose "value" fields will be instances of
 * identifier_node that represent the selector. It returns the offset of
 * the selector from the beginning of the _OBJC_STRINGS pool. This offset
 * is typically used by "init_selector()" during code generation.
 */
static int 
add_objc_string(tree anIdentifier)
{
  tree chain;
  int offset = 0;

  if (chain = sel_refdef_chain)
    {
      tree tail;
      do
        {
        if (anIdentifier == TREE_VALUE(chain))
	  return offset;

        /* add one for the '\0' character */
        offset += IDENTIFIER_LENGTH(TREE_VALUE(chain)) + 1;
        tail = chain;
        chain = TREE_CHAIN(chain);
        }
      while (chain);

      /* append to the end of the list */
      TREE_CHAIN(tail) = perm_tree_cons(NULLT, anIdentifier, NULLT);
    }
  else
    sel_refdef_chain = perm_tree_cons(NULLT, anIdentifier, NULLT);

  return offset;
}

tree lookup_interface(tree anIdentifier)
{
  tree chain;

#ifdef NeXT_CPLUS
  if (anIdentifier && TREE_CODE(anIdentifier) != IDENTIFIER_NODE &&
	TREE_CODE(anIdentifier) == TYPE_DECL)
    anIdentifier = DECL_NAME(anIdentifier);
#endif

  for (chain = interface_chain; chain; chain = TREE_CHAIN(chain))
    {
      if (anIdentifier == chain->class.my_name)
	return chain;
    }
  return NULLT;
}

static tree objc_copy_list(tree list, tree *head)
{
  tree newlist = NULL_TREE, tail = NULL_TREE;

  while (list)
    {
      tail = copy_node(list);

      /* the following statement fixes a bug when inheriting instance 
	 variables that are declared to be bitfields. finish_struct() expects
	 to find the width of the bitfield in DECL_INITIAL(), which it
	 nulls out after processing the decl of the super class...rather
	 than change the way finish_struct() works (which is risky), 
	 I create the situation it expects...s.naroff (7/23/89).
       */
      if (TREE_PACKED(tail) && DECL_SIZE_UNIT(tail) && DECL_INITIAL(tail) == 0)
	DECL_INITIAL(tail) = build_int_2(DECL_SIZE_UNIT(tail),0);

      newlist = chainon(newlist, tail);
      list = TREE_CHAIN(list);
    }
  *head = newlist;
  return tail;
}

/* used by:
 * build_private_template(), get_class_ivars(), and get_static_reference().
 */
static tree build_ivar_chain(tree interface)
{
  tree my_name, super_name, ivar_chain;

  my_name = interface->class.my_name;
  super_name = interface->class.super_name;

  /* "leaf" ivars never get copied...there is no reason to. */
  ivar_chain = interface->class.ivar_decls;

  while (super_name)
    {
      tree op1;
      tree super_interface = lookup_interface (super_name);

      if (!super_interface)
        {
	/* fatal did not work with 2 args...should fix */
        error("Cannot find interface declaration for `%s', superclass of `%s'",
             IDENTIFIER_POINTER(super_name), IDENTIFIER_POINTER(my_name));
        exit(34);
        }
      if (super_interface == interface)
        {
        fatal ("Circular inheritance in interface declaration for `%s'",
             IDENTIFIER_POINTER(super_name));
        }
      interface = super_interface;
      my_name = interface->class.my_name;
      super_name = interface->class.super_name;

      if (op1 = interface->class.ivar_decls)
        {
        tree head, tail = objc_copy_list(op1, &head);

	/* prepend super class ivars...make a copy of the list, we
	 * do not want to alter the original.
	 */
        TREE_CHAIN(tail) = ivar_chain;
	ivar_chain = head;
        }
    }
  return ivar_chain;
}

/*
 *  struct <classname> { 
 *    struct objc_class *isa; 
 *    ... 
 *  };
 */
static tree build_private_template(tree class)
{
  tree ivar_context;

  if (class->class.static_template)
    {
    _PRIVATE_record = class->class.static_template;
    ivar_context = TYPE_FIELDS(class->class.static_template);
    }
  else
    {
    _PRIVATE_record = start_struct(RECORD_TYPE, class->class.my_name);

    ivar_context = build_ivar_chain(class);

    objc_finish_struct(_PRIVATE_record, ivar_context);

    class->class.static_template = _PRIVATE_record;

    /* mark this record as class template - for class type checking */
    TREE_STATIC_TEMPLATE(_PRIVATE_record) = 1;
    }
  instance_type = groktypename(
		      build_tree_list(build_tree_list(NULLT, _PRIVATE_record),
		           build(INDIRECT_REF,NULLT,NULLT)));
  return ivar_context;
}

/*
 *  struct objc_category {
 *    char *category_name;
 *    char *class_name;
 *    struct objc_method_list *instance_methods;
 *    struct objc_method_list *class_methods;
 *  };
 */
static void build_category_template()
{
  tree decl_specs, field_decl, field_decl_chain;

  objc_category_template = start_struct(RECORD_TYPE, 
					get_identifier(_TAG_CATEGORY));
  /* char *category_name; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("category_name"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* char *class_name; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("class_name"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_method_list *instance_methods; */

  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
		get_identifier(_TAG_METHOD_LIST)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("instance_methods"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_method_list *class_methods; */

  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
		get_identifier(_TAG_METHOD_LIST)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("class_methods"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_category_template, field_decl_chain);
}

/*
 *  struct objc_class {
 *    struct objc_class *isa;
 *    struct objc_class *super_class;
 *    char *name;
 *    long version;
 *    long info;
 *    long instance_size;
 *    struct objc_ivar_list *ivars;
 *    struct objc_method_list *methods;
 *    struct objc_cache *cache;
 *  };
 */
static void build_class_template()
{
  tree decl_specs, field_decl, field_decl_chain;

  objc_class_template = start_struct(RECORD_TYPE, get_identifier(_TAG_CLASS));

  /* struct objc_class *isa; */

  decl_specs = build_tree_list(NULLT, objc_class_template);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("isa"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* struct objc_class *super_class; */

  decl_specs = build_tree_list(NULLT, objc_class_template);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("super_class"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* char *name; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("name"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* long version; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("version");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* long info; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("info");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* long instance_size; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_LONG]);
  field_decl = get_identifier("instance_size");
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_ivar_list *ivars; */

  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
		get_identifier(_TAG_IVAR_LIST)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("ivars"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_method_list *methods; */

  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
		get_identifier(_TAG_METHOD_LIST)));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("methods"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_cache *cache; */

  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
		get_identifier("objc_cache")));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("cache"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_class_template, field_decl_chain);
}

/* 
 * generate appropriate forward declarations for an implementation
 */
static
void synth_forward_declarations()
{
  tree sc_spec, decl_specs, factory_id, anId;

  /* extern struct objc_class _OBJC_CLASS_<my_name>; */

  anId = synth_id_with_class_suffix("_OBJC_CLASS");

  sc_spec = build_tree_list(NULLT, ridpointers[RID_EXTERN]);
  decl_specs = tree_cons(NULLT, objc_class_template, sc_spec);
  _OBJC_CLASS_decl = define_decl(anId, decl_specs);

  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_CLASS_decl) = 1;

  /* extern struct objc_class _OBJC_METACLASS_<my_name>; */

  anId = synth_id_with_class_suffix("_OBJC_METACLASS");

  _OBJC_METACLASS_decl = define_decl(anId, decl_specs);

  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(_OBJC_METACLASS_decl) = 1;

  /* pre-build the following entities - for speed/convenience. */

  anId = get_identifier("super_class");
#ifdef NeXT_CPLUS
  _clsSuper_ref = build_component_ref(_OBJC_CLASS_decl, anId, NULLT, 1);
  __clsSuper_ref = build_component_ref(_OBJC_METACLASS_decl, anId, NULLT, 1);
#else
  _clsSuper_ref = build_component_ref(_OBJC_CLASS_decl, anId);
  __clsSuper_ref = build_component_ref(_OBJC_METACLASS_decl, anId);
#endif
}

static void error_with_ivar(char *message, tree decl, tree rawdecl)
{
  extern int errorcount;

  fprintf(stderr,"%s:%d: ",DECL_SOURCE_FILE(decl),DECL_SOURCE_LINE(decl));
  bzero(errbuf,BUFSIZE);
  fprintf(stderr,"%s `%s'\n", message, genDeclaration(rawdecl,errbuf));
  errorcount++;
}

#define USERTYPE(t)	(TREE_CODE(t) == RECORD_TYPE || \
			 TREE_CODE(t) == UNION_TYPE ||  \
			 TREE_CODE(t) == ENUMERAL_TYPE)

static 
void check_ivars(tree inter, tree imp)
{
  tree intdecls = inter->class.ivar_decls;
  tree impdecls = imp->class.ivar_decls;
  tree rawintdecls = inter->class.raw_ivars;
  tree rawimpdecls = imp->class.raw_ivars;

  while (1)
    {
      tree t1, t2;

      if (intdecls == 0 && impdecls == 0)
	break;
      if (intdecls == 0 || impdecls == 0)
	{
	error("inconsistent instance variable specification");
	break;
	}
      t1 = TREE_TYPE(intdecls); t2 = TREE_TYPE(impdecls);

#ifdef NeXT_CPLUS
      if (!comptypes(t1, t2, 0))
#else
      if (!comptypes(t1, t2))
#endif
	{
	if (DECL_NAME(intdecls) == DECL_NAME(impdecls))
	  {
	  error_with_ivar("conflicting instance variable type",
                          impdecls, rawimpdecls);
	  error_with_ivar("previous declaration of",
                          intdecls, rawintdecls);
	  }
        else /* both the type and the name don't match */
	  {
	  error("inconsistent instance variable specification");
	  break;
	  }
	}
      else if (DECL_NAME(intdecls) != DECL_NAME(impdecls))
	  {
	  error_with_ivar("conflicting instance variable name",
                          impdecls, rawimpdecls);
	  error_with_ivar("previous declaration of",
                          intdecls, rawintdecls);
	  }
      intdecls = TREE_CHAIN(intdecls);
      impdecls = TREE_CHAIN(impdecls);
      rawintdecls = TREE_CHAIN(rawintdecls);
      rawimpdecls = TREE_CHAIN(rawimpdecls);
    }
}

/*  
 * 	struct objc_super { 
 *		id self;
 *		struct objc_class *class;
 *	};
 */
static tree build_super_template()
{
  tree record, decl_specs, field_decl, field_decl_chain;

  record = start_struct(RECORD_TYPE, get_identifier(_TAG_SUPER));

  /* struct objc_object *self; */

  decl_specs = build_tree_list(NULLT, objc_object_reference);
  field_decl = get_identifier("self");
  field_decl = build(INDIRECT_REF, NULLT, field_decl);
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* struct objc_class *class; */

  decl_specs = get_identifier(_TAG_CLASS);
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, decl_specs));
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("class"));

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(record, field_decl_chain);

  /* `struct objc_super *' */
  super_type = groktypename(build_tree_list(build_tree_list(NULLT, record),
				build(INDIRECT_REF,NULLT,NULLT)));
  return record;
}

/*  
 * 	struct objc_ivar { 
 *		char *ivar_name;
 *		char *ivar_type;
 *		int ivar_offset;
 *	};
 */
static tree build_ivar_template()
{
  tree objc_ivar_id, objc_ivar_record;
  tree decl_specs, field_decl, field_decl_chain;

  objc_ivar_id = get_identifier(_TAG_IVAR);
  objc_ivar_record = start_struct(RECORD_TYPE, objc_ivar_id);

  /* char *ivar_name; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("ivar_name"));

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* char *ivar_type; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("ivar_type"));

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* int ivar_offset; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_INT]);
  field_decl = get_identifier("ivar_offset");

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_ivar_record, field_decl_chain);

  return objc_ivar_record;
}

/*
 * 	struct { 
 *		int ivar_count;
 *		struct objc_ivar ivar_list[ivar_count];
 *	};
 */
static tree build_ivar_list_template(tree list_type, int size)
{
  tree objc_ivar_list_id, objc_ivar_list_record;
  tree decl_specs, field_decl, field_decl_chain;

  objc_ivar_list_record = start_struct(RECORD_TYPE, NULLT);

  /* int ivar_count; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_INT]);
  field_decl = get_identifier("ivar_count");

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* struct objc_ivar ivar_list[]; */

  decl_specs = build_tree_list(NULLT, list_type); 
  field_decl = build_nt(ARRAY_REF, get_identifier("ivar_list"),
			build_int_2(size,0));

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_ivar_list_record, field_decl_chain);

  return objc_ivar_list_record;
}

/*
 * 	struct { 
 *		int method_next;
 *		int method_count;
 *		struct objc_method method_list[method_count];
 *	};
 */
static tree build_method_list_template(tree list_type, int size)
{
  tree objc_ivar_list_id, objc_ivar_list_record;
  tree decl_specs, field_decl, field_decl_chain;

  objc_ivar_list_record = start_struct(RECORD_TYPE, NULLT);

  /* int method_next; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_INT]);
  field_decl = get_identifier("method_next");

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  /* int method_count; */

  decl_specs = build_tree_list(NULLT, ridpointers[RID_INT]);
  field_decl = get_identifier("method_count");

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* struct objc_method method_list[]; */

  decl_specs = build_tree_list(NULLT, list_type); 
  field_decl = build_nt(ARRAY_REF, get_identifier("method_list"),
			build_int_2(size,0));

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(objc_ivar_list_record, field_decl_chain);

  return objc_ivar_list_record;
}

static tree build_ivar_list_initializer(tree field_decl, int *size)
{
  tree initlist = NULLT;

  do
    {
    int offset;

    /* set name */
    if (DECL_NAME(field_decl))
      {
        offset = add_objc_string(DECL_NAME(field_decl));
        initlist = tree_cons(NULLT, build_msg_pool_reference(offset), initlist);
      }
    else
      {
        /* unnamed bit-field ivar (yuck). */
        initlist = tree_cons(NULLT, build_int_2(0), initlist);
      }

    /* set type */
    bzero(utlbuf,BUFSIZE);
    encode_field_decl(field_decl, utlbuf, NeXT_ENCODE_DONT_INLINE_DEFS);

    offset = add_objc_string(get_identifier(utlbuf));
    initlist = tree_cons(NULLT, build_msg_pool_reference(offset), initlist);

    /* set offset */
    initlist = tree_cons(NULLT, build_int_2(DECL_OFFSET(field_decl)/
				DECL_SIZE_UNIT(field_decl), 0), initlist);
    (*size)++;
    field_decl = TREE_CHAIN(field_decl);
    }
  while (field_decl);

  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

static tree generate_ivars_list(tree type, char *name, int size, tree list)
{
  tree sc_spec, decl_specs, decl, initlist;

  sc_spec = tree_cons(NULLT, ridpointers[RID_STATIC], NULLT);
  decl_specs = tree_cons(NULLT, type, sc_spec);

#ifdef NeXT_CPLUS
  decl = start_decl(synth_id_with_class_suffix(name), decl_specs, 1, NULLT);
#else
  decl = start_decl(synth_id_with_class_suffix(name), decl_specs, 1);
#endif

  initlist = build_tree_list(NULLT, build_int_2(size,0));
  initlist = tree_cons(NULLT, list, initlist);

  finish_decl(decl, build_nt(CONSTRUCTOR, NULLT, nreverse(initlist)), NULLT);

  return decl;
}

static void generate_ivar_lists()
{
  tree initlist, ivar_list_template, chain;
  tree cast, variable_length_type;
  int size;

  if (!objc_ivar_template)
    objc_ivar_template = build_ivar_template();

  cast = build_tree_list(build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
			 get_identifier(_TAG_IVAR_LIST))), NULLT);
  variable_length_type = groktypename(cast);

  /* only generate class variables for the root of the inheritance
     hierarchy since these will be the same for every class */

  if (implementation_template->class.super_name == NULLT &&
	(chain = TYPE_FIELDS(objc_class_template)))
    {
    size = 0;
    initlist = build_ivar_list_initializer(chain, &size);

    ivar_list_template = build_ivar_list_template(objc_ivar_template, size);

    _OBJC_CLASS_VARIABLES_decl = 
	generate_ivars_list(ivar_list_template, "_OBJC_CLASS_VARIABLES", 
			    size, initlist);
    /* cast! */
    TREE_TYPE(_OBJC_CLASS_VARIABLES_decl) = variable_length_type;
    
    /* Mark the decl as used to avoid "defined but not used" warning. */
    TREE_USED(_OBJC_CLASS_VARIABLES_decl) = 1;
    }
  else
    _OBJC_CLASS_VARIABLES_decl = 0;

  if (chain = implementation_template->class.ivar_decls)
    {
    size = 0;
    initlist = build_ivar_list_initializer(chain, &size);

    ivar_list_template = build_ivar_list_template(objc_ivar_template, size);

    _OBJC_INSTANCE_VARIABLES_decl = 
	generate_ivars_list(ivar_list_template, "_OBJC_INSTANCE_VARIABLES", 
			    size, initlist);
    /* cast! */
    TREE_TYPE(_OBJC_INSTANCE_VARIABLES_decl) = variable_length_type; 
    
    /* Mark the decl as used to avoid "defined but not used" warning. */
    TREE_USED(_OBJC_INSTANCE_VARIABLES_decl) = 1;
    }
  else
    _OBJC_INSTANCE_VARIABLES_decl = 0;
}

static tree build_dispatch_table_initializer(tree entries, int *size)
{
  tree initlist = NULLT;

  do
    {
    int offset = add_objc_string(entries->method.sel_name);

    initlist = tree_cons(NULLT, init_selector(offset), initlist);

    offset = add_objc_string(entries->method.encode_types);
    initlist = tree_cons(NULLT, build_msg_pool_reference(offset), initlist);

    initlist = tree_cons(NULLT, entries->method.mth_defn, initlist);

    (*size)++;
    entries = TREE_CHAIN(entries);
    }
  while (entries);

  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

/*  
 * To accomplish method prototyping without generating all kinds of
 * inane warnings, the definition of the dispatch table entries were
 * changed from:
 *
 * 	struct objc_method { SEL _cmd; id (*_imp)(); };
 * to:
 * 	struct objc_method { SEL _cmd; void *_imp; };
 */
static tree build_method_template()
{
  tree _SLT_record;
  tree decl_specs, field_decl, field_decl_chain, parms; 

  _SLT_record = start_struct(RECORD_TYPE, get_identifier(_TAG_METHOD));

#ifdef NeXT_SELS_R_INTS
  /* unsigned int _cmd; */
  decl_specs = tree_cons(NULLT, ridpointers[RID_UNSIGNED], NULLT);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  field_decl = get_identifier("_cmd");
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS
  /* char *_cmd; */
  decl_specs = tree_cons(NULLT, ridpointers[RID_CHAR], NULLT);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("_cmd"));
#endif

#ifdef NeXT_SELS_R_STRUCTS
  /* struct objc_selector _cmd; */
  decl_specs = tree_cons(NULLT,
			 objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SELECTOR)),
			 NULLT);
  field_decl = get_identifier("_cmd");
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  /* struct objc_selector _cmd; */
  decl_specs = tree_cons(NULLT,
			 objc_xref_tag(RECORD_TYPE, get_identifier(TAG_SELECTOR)),
			 NULLT);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("_cmd"));
#endif

  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  field_decl_chain = field_decl;

  decl_specs = tree_cons(NULLT, ridpointers[RID_CHAR], NULLT);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("method_types"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  /* void *_imp; */

  decl_specs = tree_cons(NULLT, ridpointers[RID_VOID], NULLT);
  field_decl = build(INDIRECT_REF, NULLT, get_identifier("_imp"));
  field_decl = objc_grokfield(input_filename, lineno, field_decl, decl_specs, NULLT);
  chainon(field_decl_chain, field_decl);

  objc_finish_struct(_SLT_record, field_decl_chain);

  return _SLT_record;
}


static tree generate_dispatch_table(tree type, char *name, int size, tree list)
{
  tree sc_spec, decl_specs, decl, initlist;

  sc_spec = tree_cons(NULLT, ridpointers[RID_STATIC], NULLT);
  decl_specs = tree_cons(NULLT, type, sc_spec);

#ifdef NeXT_CPLUS
  decl = start_decl(synth_id_with_class_suffix(name), decl_specs, 1, NULLT);
#else
  decl = start_decl(synth_id_with_class_suffix(name), decl_specs, 1);
#endif

  initlist = build_tree_list(NULLT, build_int_2(0,0));
  initlist = tree_cons(NULLT, build_int_2(size,0), initlist);
  initlist = tree_cons(NULLT, list, initlist);

  finish_decl(decl, build_nt(CONSTRUCTOR, NULLT, nreverse(initlist)), NULLT);

  return decl;
}

static void generate_dispatch_tables()
{
  tree initlist, chain, method_list_template;
  tree cast, variable_length_type;
  int size;

  if (!objc_method_template)
    objc_method_template = build_method_template();

  cast = build_tree_list(build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE, 
			 get_identifier(_TAG_METHOD_LIST))), NULLT);
  variable_length_type = groktypename(cast);

  if (chain = implementation_context->class.cls_method_chain)
    {
    size = 0;
    initlist = build_dispatch_table_initializer(chain, &size);

    method_list_template = build_method_list_template(objc_method_template, 
					size);
#if 1
    if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
      _OBJC_CLASS_METHODS_decl = 
	  generate_dispatch_table(method_list_template, "_OBJC_CLASS_METHODS", 
				  size, initlist);
    else
      /* we have a category */
      _OBJC_CLASS_METHODS_decl = 
	  generate_dispatch_table(method_list_template,
				  "_OBJC_CATEGORY_CLASS_METHODS", 
				  size, initlist);
#else
    _OBJC_CLASS_METHODS_decl = 
       generate_dispatch_table(method_list_template, "_OBJC_CLASS_METHODS", 
				size, initlist);
#endif
    /* cast! */
    TREE_TYPE(_OBJC_CLASS_METHODS_decl) = variable_length_type;

    /* Mark the decl as used to avoid "defined but not used" warning. */
    TREE_USED(_OBJC_CLASS_METHODS_decl) = 1;    
    }
  else
    _OBJC_CLASS_METHODS_decl = 0;

  if (chain = implementation_context->class.nst_method_chain)
    {
    size = 0;
    initlist = build_dispatch_table_initializer(chain, &size);

    method_list_template = build_method_list_template(objc_method_template, 
					size);
#if 1
    if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
      _OBJC_INSTANCE_METHODS_decl = 
	  generate_dispatch_table(method_list_template, "_OBJC_INSTANCE_METHODS", 
				  size, initlist);
    else
      /* we have a category */
      _OBJC_INSTANCE_METHODS_decl = 
	  generate_dispatch_table(method_list_template,
				  "_OBJC_CATEGORY_INSTANCE_METHODS", 
				  size, initlist);
#else
    _OBJC_INSTANCE_METHODS_decl = 
       generate_dispatch_table(method_list_template, "_OBJC_INSTANCE_METHODS", 
				size, initlist);
#endif
    /* cast! */
    TREE_TYPE(_OBJC_INSTANCE_METHODS_decl) = variable_length_type; 

    /* Mark the decl as used to avoid "defined but not used" warning. */
    TREE_USED(_OBJC_INSTANCE_METHODS_decl) = 1;    
    }
  else
    _OBJC_INSTANCE_METHODS_decl = 0;
}

static tree build_category_initializer(tree cat_name, tree class_name,
			       tree instance_methods, tree class_methods)
{
  tree initlist = NULLT, expr;

  initlist = tree_cons(NULLT, cat_name, initlist);
  initlist = tree_cons(NULLT, class_name, initlist);

  if (!instance_methods)
    initlist = tree_cons(NULLT, build_int_2(0,0), initlist);
  else
    {
    expr = build_unary_op(ADDR_EXPR, instance_methods,0);
    initlist = tree_cons(NULLT, expr, initlist);
    }
  if (!class_methods)
    initlist = tree_cons(NULLT, build_int_2(0,0), initlist);
  else
    {
    expr = build_unary_op(ADDR_EXPR, class_methods,0);
    initlist = tree_cons(NULLT, expr, initlist);
    }
  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

/*
 *  struct objc_class {
 *    struct objc_class *isa;
 *    struct objc_class *super_class;
 *    char *name;
 *    long version;
 *    long info;
 *    long instance_size;
 *    struct objc_ivar_list *ivars;
 *    struct objc_method_list *methods;
 *    struct objc_cache *cache;
 *  };
 */
static tree build_shared_structure_initializer(tree isa, 
		tree clsSuper, tree name_expr, tree sizeInst, 
		int status, tree dispatch_table, tree ivar_list)
{
  tree initlist = NULLT, expr;

  /* isa = */
  initlist = tree_cons(NULLT, isa, initlist);

  /* super_class = */
  initlist = tree_cons(NULLT, clsSuper, initlist);

  /* name = */
  initlist = tree_cons(NULLT, name_expr, initlist);

  /* version = */
  initlist = tree_cons(NULLT, build_int_2(0,0), initlist);

  /* info = */
  initlist = tree_cons(NULLT, build_int_2(status), initlist);

  /* instance_size = */
  initlist = tree_cons(NULLT, sizeInst, initlist);

  /* objc_ivar_list = */
  if (!ivar_list)
    initlist = tree_cons(NULLT, build_int_2(0,0), initlist);
  else
    {
    expr = build_unary_op(ADDR_EXPR, ivar_list,0);
    initlist = tree_cons(NULLT, expr, initlist);
    }

  /* objc_method_list = */
  if (!dispatch_table)
    initlist = tree_cons(NULLT, build_int_2(0,0), initlist);
  else
    {
    expr = build_unary_op(ADDR_EXPR, dispatch_table,0);
    initlist = tree_cons(NULLT, expr, initlist);
    }

  /* method_cache = */
  initlist = tree_cons(NULLT, build_int_2(0,0), initlist);

  return build_nt(CONSTRUCTOR, NULLT, nreverse(initlist));
}

/* 
 * static struct objc_category _OBJC_CATEGORY_<name> = { ... };
 */
static void generate_category(tree cat)
{
  tree sc_spec, decl_specs, decl;
  tree initlist, cat_name_expr, class_name_expr;
  int offset;

  sc_spec = tree_cons(NULLT, ridpointers[RID_STATIC], NULLT);
  decl_specs = tree_cons(NULLT, objc_category_template, sc_spec);

#ifdef NeXT_CPLUS
  decl = start_decl(synth_id_with_class_suffix("_OBJC_CATEGORY"),
			 decl_specs, 1, NULLT);
#else
  decl = start_decl(synth_id_with_class_suffix("_OBJC_CATEGORY"),
			 decl_specs, 1);
#endif

  offset = add_objc_string(cat->class.super_name);
  cat_name_expr = build_msg_pool_reference(offset);

  offset = add_objc_string(cat->class.my_name);
  class_name_expr = build_msg_pool_reference(offset);

  initlist = build_category_initializer(
		cat_name_expr, class_name_expr,
  		_OBJC_INSTANCE_METHODS_decl, _OBJC_CLASS_METHODS_decl);

  finish_decl(decl, initlist, NULLT);
  
  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(decl) = 1;
}

/* 
 * static struct objc_class _OBJC_METACLASS_Foo={ ... }; 
 * static struct objc_class _OBJC_CLASS_Foo={ ... }; 
 */
static void generate_shared_structures()
{
  tree sc_spec, decl_specs, expr_decl, decl; 
  tree name_expr, super_expr, root_expr;
  tree my_root_id = NULLT, my_super_id = NULLT;
  tree cast_type, initlist;
  int offset;

  if (my_super_id = implementation_template->class.super_name)
    {
    add_class_reference(my_super_id);

    /* compute "my_root_id" - this is required for code generation.
     * the "isa" for all meta class structures points to the root of
     * the inheritance hierarchy (e.g. "__Object")...
     */
    my_root_id = my_super_id;
    do 
      {
      tree my_root_int = lookup_interface(my_root_id);

      if (my_root_int && my_root_int->class.super_name)
        my_root_id = my_root_int->class.super_name; 
      else 
	break;
      }
    while (1);
    }
  else  /* no super class */
    {
    my_root_id = implementation_template->class.my_name;
    }

  cast_type = groktypename(build_tree_list(build_tree_list(NULLT, 
			objc_class_template), build(INDIRECT_REF,NULLT,NULLT)));

  offset = add_objc_string(implementation_template->class.my_name);
  name_expr = build_msg_pool_reference(offset);

  /* install class `isa' and `super' pointers at runtime */
  if (my_super_id)
    {
    offset = add_objc_string(my_super_id);
    super_expr = build_msg_pool_reference(offset);
    TREE_TYPE(super_expr) = cast_type; /* cast! */
    }
  else
    super_expr = build_int_2(0,0);

  offset = add_objc_string(my_root_id);
  root_expr = build_msg_pool_reference(offset);
  TREE_TYPE(root_expr) = cast_type; /* cast! */

  /* static struct objc_class _OBJC_METACLASS_Foo = { ... }; */

  sc_spec = build_tree_list(NULLT, ridpointers[RID_STATIC]);
  decl_specs = tree_cons(NULLT, objc_class_template, sc_spec);

#ifdef NeXT_CPLUS
  decl = start_decl(DECL_NAME(_OBJC_METACLASS_decl), decl_specs, 1, NULLT); 
#else
  decl = start_decl(DECL_NAME(_OBJC_METACLASS_decl), decl_specs, 1); 
#endif

  initlist = build_shared_structure_initializer(
		root_expr, super_expr, name_expr, 
		TYPE_SIZE(objc_class_template), 
		2/*CLS_META*/,
		_OBJC_CLASS_METHODS_decl, _OBJC_CLASS_VARIABLES_decl);

  finish_decl(decl, initlist, NULLT);

  /* static struct objc_class _OBJC_CLASS_Foo={ ... }; */

#ifdef NeXT_CPLUS
  decl = start_decl(DECL_NAME(_OBJC_CLASS_decl), decl_specs, 1, NULLT);
#else
  decl = start_decl(DECL_NAME(_OBJC_CLASS_decl), decl_specs, 1);
#endif

  initlist = build_shared_structure_initializer(
		build_unary_op(ADDR_EXPR,_OBJC_METACLASS_decl,0), 
		super_expr, name_expr, 
		TYPE_SIZE(implementation_template->class.static_template), 
		1/*CLS_FACTORY*/,
  		_OBJC_INSTANCE_METHODS_decl, _OBJC_INSTANCE_VARIABLES_decl);

  finish_decl(decl, initlist, NULLT);
}

static tree synth_id_with_class_suffix(char *preamble)
{
    if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
      sprintf(utlbuf,"%s_%s", preamble,
	      IDENTIFIER_POINTER(implementation_context->class.my_name));
    else
      /* we have a category */
      sprintf(utlbuf,"%s_%s_%s", preamble,
	      IDENTIFIER_POINTER(implementation_context->class.my_name),
	      IDENTIFIER_POINTER(implementation_context->class.super_name));

    return get_identifier(utlbuf);
}

/*
 *   usage:
 *		keyworddecl:
 *			selector ':' '(' typename ')' identifier
 *
 *   purpose:
 *		transform an Objective-C keyword argument into
 *		the C equivalent parameter declarator.
 *
 *   in:	key_name, an "identifier_node" (optional).
 *		arg_type, a  "tree_list" (optional).
 *		arg_name, an "identifier_node".
 *
 *   note:	it would be really nice to strongly type the preceding
 *		arguments in the function prototype; however, then i 
 *		could not use the "accessor" macros defined in "tree.h". 
 *
 *   out:	an instance of "keyword_decl".
 *
 */

tree 
build_keyword_decl (tree key_name, tree arg_type, tree arg_name)
{
  tree keyword_decl;

  /* if no type is specified, default to "id" */
  if (arg_type == NULLT)
      arg_type = build_tree_list(build_tree_list(NULLT,objc_object_reference), 
		 		 build(INDIRECT_REF,NULLT,NULLT));

  keyword_decl = make_node(KEYWORD_DECL);

  TREE_TYPE(keyword_decl) = arg_type;
  keyword_decl->keyword.arg_name   = arg_name;
  keyword_decl->keyword.key_name   = key_name;

  return keyword_decl;
}

/*
 *  given a chain of keyword_decl's, synthesize the full keyword selector.
 */
static tree
build_keyword_selector (tree selector)
{
  int len = 0;
  tree key_chain, key_name;
  char *buf;

  for (key_chain = selector; key_chain; key_chain = TREE_CHAIN(key_chain))
    {
    if (TREE_CODE(selector) == KEYWORD_DECL)
      key_name = key_chain->keyword.key_name;
    else if (TREE_CODE(selector) == TREE_LIST)
      key_name = TREE_PURPOSE(key_chain);

    if (key_name)
      len += IDENTIFIER_LENGTH(key_name) + 1;
    else /* just a ':' arg */
      len++;
    }
  buf = (char *)alloca(len+1);
  bzero(buf, len+1);

  for (key_chain = selector; key_chain; key_chain = TREE_CHAIN(key_chain))
    {
    if (TREE_CODE(selector) == KEYWORD_DECL)
      key_name = key_chain->keyword.key_name;
    else if (TREE_CODE(selector) == TREE_LIST)
      key_name = TREE_PURPOSE(key_chain);

    if (key_name)
      strcat(buf,IDENTIFIER_POINTER(key_name));
    strcat(buf,":");
    }
  return get_identifier(buf);
}

/* used for declarations and definitions */

tree
build_method_decl (enum tree_code code, tree ret_type, 
		   tree selector, tree add_args)
{
  tree method_decl;

  /* if no type is specified, default to "id" */
  if (ret_type == NULLT)
      ret_type = build_tree_list(build_tree_list(NULLT,objc_object_reference), 
		 		 build(INDIRECT_REF,NULLT,NULLT));

  method_decl = make_node(code);
  TREE_TYPE(method_decl) = ret_type;
  method_decl->method.filename = input_filename;
  method_decl->method.linenum = lineno;
  /*
   *  if we have a keyword selector, create an identifier_node that 
   *  represents the full selector name (`:' included)...
   */
  if (TREE_CODE(selector) == KEYWORD_DECL)
    {
    method_decl->method.sel_name = build_keyword_selector(selector);
    method_decl->method.sel_args = selector;
    method_decl->method.add_args = add_args;
    }
  else
    {
    method_decl->method.sel_name = selector;
    method_decl->method.sel_args = NULLT;
    method_decl->method.add_args = NULLT;
    }	
  
  return method_decl;
}

#define METHOD_DEF 0
#define METHOD_REF 1
/*
 * used by `build_message_expr()' and `comp_method_types()'.
 *
 * add_args is a tree_list node the following info on a parameter list:
 *
 *    The TREE_PURPOSE is a chain of decls of those parms.
 *    The TREE_VALUE is a list of structure, union and enum tags defined.
 *    The TREE_CHAIN is a list of argument types to go in the FUNCTION_TYPE.
 *    This tree_list node is later fed to `grokparms'.
 *
 *    VOID_AT_END nonzero means append `void' to the end of the type-list.
 *    Zero means the parmlist ended with an ellipsis so don't append `void'.
 */
static
tree getArgTypeList(tree meth, int context, int superflag)
{
  tree arglist, akey;

  /* receiver type */
  if (superflag)
    arglist = build_tree_list(NULLT, super_type);
  else
    {
    if (context == METHOD_DEF)
      arglist = build_tree_list(NULLT,TREE_TYPE(self_decl));
    else
      arglist = build_tree_list(NULLT,id_type);
    }

  /* selector type - will eventually change to `int' */
  chainon(arglist, build_tree_list(NULLT,_selector_type));

  /* build a list of argument types */
  for (akey = meth->method.sel_args; akey; akey = TREE_CHAIN(akey))
    {
    tree arg_decl = groktypename_inParmContext(TREE_TYPE(akey));
    chainon(arglist,build_tree_list(NULLT,TREE_TYPE(arg_decl)));
    }

  if (meth->method.add_args == (tree)1)
    /* 
     * we have a `,...' immediately following the selector,
     * finalize the arglist...simulate get_parm_info(0) 
     */
    ;
  else if (meth->method.add_args)
    {
    /* we have a variable length selector */
    tree add_arg_list = TREE_CHAIN(meth->method.add_args);
    chainon(arglist,add_arg_list);
    }
  else /* finalize the arglist...simulate get_parm_info(1) */
#ifdef NeXT_CPLUS
    chainon(arglist, void_list_node);
#else
    chainon(arglist, build_tree_list(NULLT,void_type_node));
#endif

  return arglist;
}

static tree check_duplicates(hash hsh)
{
    tree meth = NULLT;

    if (hsh)
      {
      meth = hsh->key;

      if (hsh->list)
        {
        /* we have two methods with the same name and different types */
        attr loop;
        char type;

        type = (TREE_CODE(meth) == INSTANCE_METHOD_DECL) ? '-' : '+';

        warning("multiple declarations for method `%s'",
                 IDENTIFIER_POINTER(meth->method.sel_name));

        warn_with_method("using", type, meth);
        for (loop = hsh->list; loop; loop = loop->next)
           warn_with_method("also found", type, loop->value);
        }
      }
    return meth;
}

static tree receiver_is_class_object(tree receiver)
{
  /* the receiver is a function call that returns an id...
   * ...check if it is a call to objc_getClass, if so, give it
   * special treatment.
   */
  tree exp = 0;

  if ((exp = TREE_OPERAND(receiver, 0)) && (TREE_CODE(exp) == ADDR_EXPR))
    {
    if ((exp = TREE_OPERAND(exp, 0)) && 
	(TREE_CODE(exp) == FUNCTION_DECL) && exp == objc_getClass_decl)
      {
      /* we have a call to objc_getClass! */
      tree arg = 0;

      if ((arg = TREE_OPERAND(receiver,1)) && (TREE_CODE(arg) == TREE_LIST))
        {
        if ((arg = TREE_VALUE(arg)) && (TREE_CODE(arg) == NOP_EXPR))
          {
          if ((arg = TREE_OPERAND(arg,0)) && (TREE_CODE(arg) == ADDR_EXPR))
            {
            if ((arg = TREE_OPERAND(arg,0)) && (TREE_CODE(arg) == STRING_CST))
              /* finally, we have the class name */
	      return get_identifier(TREE_STRING_POINTER(arg));
            }
          }
        }
      }
    }
  return 0;
}

/*
 *	(*(<abstract_decl>(*)())_msg)(receiver, selTransTbl[n], ...);
 *	(*(<abstract_decl>(*)())_msgSuper)(receiver, selTransTbl[n], ...);
 */
tree
build_message_expr(tree mess)
{
  tree receiver = TREE_PURPOSE(mess), rtype, sel_name;
  tree args = TREE_VALUE(mess);
  tree params = NULLT;
  tree meth = NULLT, message_decl;
  int selTransTbl_index;
  tree retval, class_ident = 0;
  int statically_typed = 0, statically_allocated = 0;

  if (TREE_CODE(receiver) == ERROR_MARK)
    return error_mark_node;

  /* determine receiver type */
  rtype = TREE_TYPE(receiver);
  
  if (rtype == super_type)
    message_decl = _msgSuper_decl;
  else
    {
    message_decl = _msg_decl;

    if (TREE_STATIC_TEMPLATE(rtype))
      statically_allocated = 1;
    else if (TREE_CODE(rtype) == POINTER_TYPE && 
             TREE_STATIC_TEMPLATE(TREE_TYPE(rtype)))
      statically_typed = 1;
      
    /* classfix -smn */
    else if ((TREE_CODE(receiver) == CALL_EXPR) && (rtype == id_type) &&
	     (class_ident = receiver_is_class_object(receiver)))
      ;
#ifdef NeXT_CPLUS
    else if ((rtype != id_type) && (rtype != objc_class_type))
#else
    else if ((rtype != id_type) && (rtype != class_type))
#endif
      {
      bzero(errbuf,BUFSIZE);
      warning("illegal receiver type `%s'", genDeclaration(rtype,errbuf));
      }
    }

  /* obtain the full selector name */
  if (TREE_CODE(args) == IDENTIFIER_NODE)
    /* a unary selector */
    sel_name = args; 
  else if (TREE_CODE(args) == TREE_LIST)
    sel_name = build_keyword_selector(args);

  selTransTbl_index = add_selector_reference(sel_name);

  /* start building argument list - synthesizing the second argument */
  if (statically_allocated)
    params = build_tree_list(NULLT, build_unary_op(ADDR_EXPR,receiver,0));
  else
    params = build_tree_list(NULLT, receiver);

  chainon(params, 
	build_tree_list(NULLT, build_selector_reference(selTransTbl_index)));

  if (TREE_CODE(args) == TREE_LIST)
    {
      tree chain = args, prev = NULLT;

      /* we have a keyword selector - check for comma expressions */
      while (chain)
	{
	tree element = TREE_VALUE(chain);

	/* we have a comma expression, must collapse... */
        if (TREE_CODE(element) == TREE_LIST)
	  {
	  if (prev)
	    TREE_CHAIN(prev) = element;
          else
	    args = element;
	  }
        prev = chain;
        chain = TREE_CHAIN(chain);
        }
      /* attach the rest of the argument list */
      chainon(params, args);
    }

  /* determine return type */

  if (rtype == super_type)
    {
    tree iface = 0;

    if (implementation_template->class.super_name)
      {
        iface = lookup_interface(implementation_template->class.super_name);
      
        if (TREE_CODE(method_context) == INSTANCE_METHOD_DECL)
          meth = lookup_instance_method_static(iface, sel_name);
        else
          meth = lookup_class_method_static(iface, sel_name);

        if (iface && !meth)
          {
            warning("`%s' does not respond to `%s'",
                    IDENTIFIER_POINTER(implementation_template->class.super_name),
                    IDENTIFIER_POINTER(sel_name));
          }
      }
    else
      error("no super class declared in interface for `%s'",
            IDENTIFIER_POINTER( implementation_template->class.my_name));
    }
  else if (statically_allocated)
    {
    tree iface = lookup_interface(TYPE_NAME(rtype));

    if (iface && !(meth = lookup_instance_method_static(iface, sel_name)))
      {
        warning("`%s' does not respond to `%s'",
#ifdef NeXT_CPLUS
                IDENTIFIER_POINTER(CPLUS_TYPE_NAME(rtype)),
#else
                IDENTIFIER_POINTER(TYPE_NAME(rtype)),
#endif
                IDENTIFIER_POINTER(sel_name));
      }
    }
  else if (statically_typed)
    {
    tree ctype = TREE_TYPE(rtype);

    /* `self' is now statically_typed...all methods should be visible 
     * within the context of the implementation...
     */
    if ((implementation_context && 
#ifdef NeXT_CPLUS
         implementation_context->class.my_name == CPLUS_TYPE_NAME(ctype)))
#else
         implementation_context->class.my_name == TYPE_NAME(ctype)))
#endif
      {
      meth = lookup_instance_method_static(implementation_template, sel_name);

      if (!meth && (implementation_template != implementation_context))
        /* the method is not published in the interface...check locally */
        meth = lookup_method(implementation_context->class.nst_method_chain,
			sel_name);
      }
    else
      {
      tree iface;

#ifdef NeXT_CPLUS
      if (iface = lookup_interface(CPLUS_TYPE_NAME(ctype)))
#else
      if (iface = lookup_interface(TYPE_NAME(ctype)))
#endif
        meth = lookup_instance_method_static(iface, sel_name);
      }

    if (!meth)
        warning("`%s' does not respond to `%s'",
#ifdef NeXT_CPLUS
                IDENTIFIER_POINTER(CPLUS_TYPE_NAME(ctype)),
#else
                IDENTIFIER_POINTER(TYPE_NAME(ctype)),
#endif
                IDENTIFIER_POINTER(sel_name));
    }
  else if (class_ident)
    {
    if ((implementation_context && 
         implementation_context->class.my_name == class_ident))
      {
      meth = lookup_class_method_static(implementation_template, sel_name);

      if (!meth && (implementation_template != implementation_context))
        /* the method is not published in the interface...check locally */
        meth = lookup_method(implementation_context->class.cls_method_chain,
			sel_name);
      }
    else
      {
      tree iface;

      if (iface = lookup_interface(class_ident))
        meth = lookup_class_method_static(iface, sel_name);
      }

    if (!meth)
      {
      warning("cannot find class (factory) method.");
      warning("return type for `%s' defaults to id",
               IDENTIFIER_POINTER(sel_name));
      }
    }
  else
    {
    hash hsh;

    /* we think we have an instance...loophole: extern id Object; */
    if (!(hsh = hash_lookup(nst_method_hash_list, sel_name)))
      {
	/* for various loopholes...like sending messages to self in a
	 * factory context...
	 */
        hsh = hash_lookup(cls_method_hash_list, sel_name);
      }
    if (!(meth = check_duplicates(hsh)))
      {
      warning("cannot find method.");
      warning("return type for `%s' defaults to id",
               IDENTIFIER_POINTER(sel_name));
      }
    }

  if (!meth)
    {
    retval = build_function_call(message_decl, params);
    }
  else	/* we have a method prototype */
    {
    tree arglist = NULLT;
    tree savret, savarg;

    arglist = getArgTypeList(meth, METHOD_REF, message_decl == _msgSuper_decl);

    /* install argument types - normally set by "build_function_type()". */
    savarg = TYPE_ARG_TYPES(TREE_TYPE(message_decl));
    TYPE_ARG_TYPES(TREE_TYPE(message_decl)) = arglist;  

    /* install return type - "message_decl" is a function returning ret_type. */
    savret = TREE_TYPE(TREE_TYPE(message_decl));
    TREE_TYPE(TREE_TYPE(message_decl)) = groktypename(TREE_TYPE(meth));

#ifdef NeXT_CPLUS
    DECL_PRINT_NAME(message_decl) = 0;
#endif

    /* check argments types supplied vs. ones in method prototype.
     * build_function_call() calls actualparameterlist() to do the
     * type checking 
     */
    retval = build_function_call(message_decl, params);

    /* restore previous return/argument types */
    TYPE_ARG_TYPES(TREE_TYPE(message_decl)) = savarg;  
    TREE_TYPE(TREE_TYPE(message_decl)) = savret;  
    }

  return retval;
}

tree 
build_selector_expr(tree selnamelist)
{
  tree selname;
  int selTransTbl_index;

  /* obtain the full selector name */
  if (TREE_CODE(selnamelist) == IDENTIFIER_NODE)
    /* a unary selector */
    selname = selnamelist; 
  else if (TREE_CODE(selnamelist) == TREE_LIST)
    selname = build_keyword_selector(selnamelist);

  selTransTbl_index = add_selector_reference(selname);

  return build_selector_reference(selTransTbl_index);
}

tree 
build_encode_expr(tree type)
{
  if (!utlbuf)
    utlbuf = (char *)malloc(BUFSIZE);
  bzero(utlbuf,BUFSIZE);

  encode_type(type, utlbuf, NeXT_ENCODE_INLINE_DEFS);

  /* synthesize a string that represents the encoded struct/union */
  return my_build_string(strlen(utlbuf)+1, utlbuf);
}

tree 
build_ivar_reference(tree anId)
{
  if (TREE_CODE(method_context) == CLASS_METHOD_DECL)
    TREE_TYPE(self_decl) = instance_type; /* cast */

#ifdef NeXT_CPLUS
  return build_component_ref(build_indirect_ref(self_decl,"->"), anId, 
				NULLT, 1);
#else
  return build_component_ref(build_indirect_ref(self_decl,"->"), anId);
#endif
}

#define HASH_ALLOC_LIST_SIZE	170
#define ATTR_ALLOC_LIST_SIZE	170
#define SIZEHASHTABLE 		257
#define HASHFUNCTION(key)	((int)key >> 2) 	/* divide by 4 */

static void hash_init()
{
  nst_method_hash_list = (hash *)malloc(SIZEHASHTABLE * sizeof(hash));
  cls_method_hash_list = (hash *)malloc(SIZEHASHTABLE * sizeof(hash));

  if (!nst_method_hash_list || !cls_method_hash_list)
    perror("unable to allocate space in objc-tree.c");
  else 
    {
      int i;

      for (i = 0; i < SIZEHASHTABLE; i++)
	{
	nst_method_hash_list[i] = 0;
	cls_method_hash_list[i] = 0;
	}
    }
}

static void hash_enter(hash *hashList, tree method)
{
  static hash 	hash_alloc_list = 0;
  static int	hash_alloc_index = 0;
  hash obj;
  int slot = HASHFUNCTION(method->method.sel_name) % SIZEHASHTABLE;

  if (!hash_alloc_list || hash_alloc_index >= HASH_ALLOC_LIST_SIZE) 
    {
    hash_alloc_index = 0;
    hash_alloc_list = (hash)malloc(sizeof(struct hashedEntry) * 
    			HASH_ALLOC_LIST_SIZE);
    if (!hash_alloc_list)
      perror ("unable to allocate in objc-tree.c");
    }
  obj = &hash_alloc_list[hash_alloc_index++];
  obj->list = 0;
  obj->next = hashList[slot];
  obj->key = method;

  hashList[slot] = obj;	/* append to front */
}

static hash hash_lookup(hash *hashList, tree sel_name)
{
  hash target;

  target = hashList[HASHFUNCTION(sel_name) % SIZEHASHTABLE];	

  while (target)
    {
    if (sel_name == target->key->method.sel_name)
      return target;

    target = target->next;
    }
  return 0;
}

static void hash_add_attr(hash entry, tree value)
{
  static attr 	attr_alloc_list = 0;
  static int	attr_alloc_index = 0;
  attr obj;

  if (!attr_alloc_list || attr_alloc_index >= ATTR_ALLOC_LIST_SIZE) 
    {
    attr_alloc_index = 0;
    attr_alloc_list = (attr)malloc(sizeof(struct hashedAttribute) * 
    			ATTR_ALLOC_LIST_SIZE);
    if (!attr_alloc_list)
      perror ("unable to allocate in objc-tree.c");
    }
  obj = &attr_alloc_list[attr_alloc_index++];
  obj->next = entry->list;
  obj->value = value;

  entry->list = obj;	/* append to front */
}

static
tree lookup_method(tree mchain, tree method)
{
  tree key;

  if (TREE_CODE(method) == IDENTIFIER_NODE)
    key = method;
  else 
    key = method->method.sel_name;

  while (mchain)
    {
    if (mchain->method.sel_name == key)
      return mchain;
    mchain = TREE_CHAIN(mchain);
    }
  return NULLT;
}

static
tree lookup_instance_method_static(tree interface, tree ident)
{
  tree inter = interface;
  tree chain = inter->class.nst_method_chain;
  tree meth = NULLT;

  do
    {
    if (meth = lookup_method(chain, ident))
      return meth;

    if (inter->class.category_list)
      {
      tree category = inter->class.category_list;
      chain = category->class.nst_method_chain;

      do 
        {
        if (meth = lookup_method(chain, ident))
          return meth;

        if (category = category->class.category_list)
          chain = category->class.nst_method_chain;
        }
      while (category);
      }

    if (inter = lookup_interface(inter->class.super_name))
      chain = inter->class.nst_method_chain;
    }
  while (inter);

  return meth;
}

static
tree lookup_class_method_static(tree interface, tree ident)
{
  tree inter = interface;
  tree chain = inter->class.cls_method_chain;
  tree meth = NULLT;

  do
    {
    if (meth = lookup_method(chain, ident))
      return meth;

    if (inter->class.category_list)
      {
      tree category = inter->class.category_list;
      chain = category->class.cls_method_chain;

      do 
        {
        if (meth = lookup_method(chain, ident))
          return meth;

        if (category = category->class.category_list)
          chain = category->class.cls_method_chain;
        }
      while (category);
      }

    if (inter = lookup_interface(inter->class.super_name))
      chain = inter->class.cls_method_chain;
    }
  while (inter);

  return meth;
}

tree
add_class_method(tree class, tree method)
{
  tree mth;
  hash hsh;

  if (!(mth = lookup_method(class->class.cls_method_chain, method)))
    {
      /* put method on list in reverse order */
      TREE_CHAIN(method) = class->class.cls_method_chain;
      class->class.cls_method_chain = method;
    }
  else
    {
      if (TREE_CODE(class) == IMPLEMENTATION_TYPE)
         error("duplicate definition of class method `%s'.",
	        IDENTIFIER_POINTER(mth->method.sel_name)); 
      else
        {
        /* check types, if different complain */
        if (!comp_proto_with_proto(method, mth))
           error("duplicate declaration of class method `%s'.",
	          IDENTIFIER_POINTER(mth->method.sel_name)); 
        }
    }

  if (!(hsh = hash_lookup(cls_method_hash_list, method->method.sel_name)))
    {
      /* install on a global chain */
      hash_enter(cls_method_hash_list, method);
    }
  else
    {
      /* check types, if different add to a list */
      if (!comp_proto_with_proto(method, hsh->key))
        hash_add_attr(hsh, method);
    }
  return method;
}

tree
add_instance_method(tree class, tree method)
{
  tree mth;
  hash hsh;

  if (!(mth = lookup_method(class->class.nst_method_chain, method)))
    {
      /* put method on list in reverse order */
      TREE_CHAIN(method) = class->class.nst_method_chain;
      class->class.nst_method_chain = method;
    }
  else
    {
      if (TREE_CODE(class) == IMPLEMENTATION_TYPE)
         error("duplicate definition of instance method `%s'.",
	        IDENTIFIER_POINTER(mth->method.sel_name)); 
      else
        {
        /* check types, if different complain */
        if (!comp_proto_with_proto(method, mth))
           error("duplicate declaration of instance method `%s'.",
	          IDENTIFIER_POINTER(mth->method.sel_name)); 
        }
    }

  if (!(hsh = hash_lookup(nst_method_hash_list, method->method.sel_name)))
    {
      /* install on a global chain */
      hash_enter(nst_method_hash_list, method);
    }
  else
    {
      /* check types, if different add to a list */
      if (!comp_proto_with_proto(method, hsh->key))
        hash_add_attr(hsh, method);
    }
  return method;
}

static 
tree add_class(tree class)
{
  /* put interfaces on list in reverse order */
  TREE_CHAIN(class) = interface_chain;
  interface_chain = class;
  return interface_chain;
}

static 
void add_category(tree class, tree category)
{
  /* put categories on list in reverse order */
  category->class.category_list = class->class.category_list;
  class->class.category_list = category;
}

/* called after parsing each instance variable declaration. Necessary to
 * preserve typedefs and implement public/private...
 */
tree
add_instance_variable(tree class, int isPublic, 
               tree declarator, tree declspecs, tree width)
{
  tree field_decl, raw_decl;

  raw_decl = build_tree_list(declspecs/*purpose*/, declarator/*value*/);

  if (class->class.raw_ivars)
    chainon(class->class.raw_ivars, raw_decl);
  else
    class->class.raw_ivars = raw_decl;

  field_decl = objc_grokfield(input_filename, lineno, 
                         declarator, declspecs, width);

  /* overload the public attribute, it is not used for FIELD_DECL's */
  if (isPublic)
    TREE_PUBLIC(field_decl) = 1;

  if (class->class.ivar_decls)
    chainon(class->class.ivar_decls, field_decl);
  else
    class->class.ivar_decls = field_decl;

  return class;
}

tree is_ivar(tree decl_chain, tree ident)
{
  for ( ; decl_chain; decl_chain = TREE_CHAIN(decl_chain))
    if (DECL_NAME(decl_chain) == ident)
      return decl_chain;
  return NULL_TREE;
}

/* we have an instance variable reference, check to see if it is public...*/

int is_public(tree anExpr, tree anIdentifier)
{
  tree basetype = TREE_TYPE(anExpr);
  enum tree_code code = TREE_CODE(basetype);
  tree decl;

  if (code == RECORD_TYPE)
    {
    if (TREE_STATIC_TEMPLATE(basetype))
      {
      if (decl = is_ivar(TYPE_FIELDS(basetype),anIdentifier))
        {
        /* important diffence between the Stepstone translator:
         
           all instance variables should be public within the context
           of the implementation...
         */
        if (implementation_context)
          {
	  if ((TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE &&
               implementation_context->class.my_name == TYPE_NAME(basetype)) ||
	      (TREE_CODE(implementation_context) == CATEGORY_TYPE &&
               implementation_context->class.my_name == TYPE_NAME(basetype)))
            return 1;
          }

	if (TREE_PUBLIC(decl))
	  return 1;

        error("instance variable `%s' is declared private",
               IDENTIFIER_POINTER(anIdentifier));
        return 0;
        }
      }
    else if (implementation_context && (basetype == objc_object_reference))
      {
      TREE_TYPE(anExpr) = _PRIVATE_record;
      if (extra_warnings) 
	{
        warning("static access to object of type `id'");
        warning("please change to type `%s *'", 
		IDENTIFIER_POINTER(implementation_context->class.my_name));
	}
      }
    }
  return 1;
}

/* implement @defs(<classname>) within struct bodies. */

tree
get_class_ivars(tree interface)
{
  return build_ivar_chain(interface);
}

tree
get_class_reference(tree interface)
{
  tree params;

  add_class_reference(interface->class.my_name);

  params = build_tree_list(NULLT, 
	     my_build_string(IDENTIFIER_LENGTH(interface->class.my_name) + 1,
	     	             IDENTIFIER_POINTER(interface->class.my_name)));

  return build_function_call(objc_getClass_decl, params);
}

/* make sure all entries in "chain" are also in "list" */

static
void check_methods(tree chain, tree list, int mtype)
{
  int first = 1;

  while (chain)
    {
    if (!lookup_method(list, chain))
      {
	if (first)
	  {
          if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
	    warning("incomplete implementation of class `%s'",
		    IDENTIFIER_POINTER(implementation_context->class.my_name));
          else if (TREE_CODE(implementation_context) == CATEGORY_TYPE)
	    warning("incomplete implementation of category `%s'",
	      IDENTIFIER_POINTER(implementation_context->class.super_name));

          first = 0;
	  }
	warning("method definition for `%c%s' not found",
		mtype, IDENTIFIER_POINTER(chain->method.sel_name));
      }
    chain = TREE_CHAIN(chain);
    }
}

tree
start_class(enum tree_code code, tree class_name, tree super_name)
{
  tree class;
  extern int doing_objc_thang;

  if (!doing_objc_thang)
    {
    warning("Objective-C text in `.c' file");
    doing_objc_thang = 1;
    init_objc();
    }

  class = make_node(code);

  class->class.my_name    = class_name;
  class->class.super_name = super_name;

  if (code == IMPLEMENTATION_TYPE)
    {
      /* pre-build the following entities - for speed/convenience. */
      if (!self_id)
        self_id = get_identifier("self");
      if (!_cmd_id)
        _cmd_id = get_identifier("_cmd");

      if (!objc_super_template)
	objc_super_template = build_super_template();

      method_slot = 0;	/* reset for multiple classes per file */

      implementation_context = class;

      /* lookup the interface for this implementation. */

      if (!(implementation_template = lookup_interface(class_name)))
        {
        warning("Cannot find interface declaration for `%s'",
	        IDENTIFIER_POINTER(class_name)); 
        add_class(implementation_template = implementation_context);
        }

      /* if a super class has been specified in the implementation,
         insure it conforms to the one specified in the interface */

      if (super_name &&
          (super_name != implementation_template->class.super_name))
        {
        error("conflicting super class name `%s'", 
          IDENTIFIER_POINTER(super_name));
        error("previous declaration of `%s'",
          IDENTIFIER_POINTER(implementation_template->class.super_name));
        }
    }
  else if (code == INTERFACE_TYPE)
    {
      if (lookup_interface(class_name))
        warning("duplicate interface declaration for class `%s'",
                 IDENTIFIER_POINTER(class_name)); 
      else
        add_class(class);
    }
  else if (code == PROTOCOL_TYPE)
    {
      tree class_category_is_assoc_with;

      /* for a category, class_name is really the name of the class that
         the following set of methods will be associated with...we must
         find the interface so that can derive the objects template */

      if (!(class_category_is_assoc_with = lookup_interface(class_name)))
        {
        error("Cannot find interface declaration for `%s'",
               IDENTIFIER_POINTER(class_name)); 
        exit(1);
        }
      else
        add_category(class_category_is_assoc_with, class);
    } 
  else if (code == CATEGORY_TYPE)
    {
      /* pre-build the following entities - for speed/convenience. */
      if (!self_id)
        self_id = get_identifier("self");
      if (!_cmd_id)
        _cmd_id = get_identifier("_cmd");

      if (!objc_super_template)
	objc_super_template = build_super_template();

      method_slot = 0;	/* reset for multiple classes per file */

      implementation_context = class;

      /* for a category, class_name is really the name of the class that
	 the following set of methods will be associated with...we must
	 find the interface so that can derive the objects template */

      if (!(implementation_template = lookup_interface(class_name)))
        {
        error("Cannot find interface declaration for `%s'",
	        IDENTIFIER_POINTER(class_name)); 
	exit(1);
        }
    }
  return class;
}

tree continue_class(tree aClass)
{
  if (TREE_CODE(aClass) == IMPLEMENTATION_TYPE ||
      TREE_CODE(aClass) == CATEGORY_TYPE)
    {
    struct imp_entry *impEntry;
    tree ivar_context;

    /* check consistency of the instance variables. */

    if (aClass->class.ivar_decls)
      check_ivars(implementation_template, aClass);

    /* code generation */

    ivar_context = build_private_template(implementation_template);

    if (!objc_class_template)
      build_class_template();

    if (!(impEntry = (struct imp_entry *)malloc(sizeof(struct imp_entry))))
      perror ("unable to allocate in objc-tree.c");

    impEntry->next = imp_list;
    impEntry->imp_context = aClass;
    impEntry->imp_template = implementation_template;

    synth_forward_declarations();
    impEntry->class_decl = _OBJC_CLASS_decl;
    impEntry->meta_decl = _OBJC_METACLASS_decl;

    /* append to front and increment count */
    imp_list = impEntry;
    if (TREE_CODE(aClass) == IMPLEMENTATION_TYPE)
      imp_count++;
    else
      cat_count++;

    return ivar_context; 
    }
  else if (TREE_CODE(aClass) == INTERFACE_TYPE)
    {
    tree record = start_struct(RECORD_TYPE, aClass->class.my_name);

    if (!TYPE_FIELDS(record))
      {
      objc_finish_struct(record, build_ivar_chain(aClass));
      aClass->class.static_template = record;

      /* mark this record as a class template - for static typing */
      TREE_STATIC_TEMPLATE(record) = 1;
      }
    return NULLT;
    }
}

/*
 * this is called once we see the "@end" in an interface/implementation.
 */
tree 
finish_class(tree class)
{
  if (TREE_CODE(class) == IMPLEMENTATION_TYPE)
    {
    /* all code generation is done in "finish_objc()" */

    if (implementation_template != implementation_context)
      {
      /* ensure that all method listed in the interface contain bodies! */
      check_methods(implementation_template->class.cls_method_chain,
                    implementation_context->class.cls_method_chain, '+');
      check_methods(implementation_template->class.nst_method_chain,
                    implementation_context->class.nst_method_chain, '-');
      }
    }
  else if (TREE_CODE(class) == CATEGORY_TYPE)
    {
    tree category = implementation_template->class.category_list;

    /* find the category interface from the class it is associated with */
    while (category)
      {
        if (class->class.super_name == category->class.super_name)
          break;
	category = category->class.category_list;
      }

    if (category)
      {
        /* insure that all method listed in the interface contain bodies! */
        check_methods(category->class.cls_method_chain,
                      implementation_context->class.cls_method_chain, '+');
        check_methods(category->class.nst_method_chain,
                      implementation_context->class.nst_method_chain, '-');
      }
    } 
  else if (TREE_CODE(class) == INTERFACE_TYPE)
    {
    tree decl_specs;

    /* extern struct objc_object *_<my_name>; */

    sprintf(utlbuf,"_%s", IDENTIFIER_POINTER(class->class.my_name));

    decl_specs = build_tree_list(NULLT, ridpointers[RID_EXTERN]);
    decl_specs = tree_cons(NULLT, objc_object_reference, decl_specs);
    define_decl(build(INDIRECT_REF, NULLT, get_identifier(utlbuf)), decl_specs);
    }
}

static void encode_pointer(tree type, char *typeStr, int format)
{
  tree pointer_to = TREE_TYPE(type);

  if (TREE_CODE(pointer_to) == RECORD_TYPE)
    {
#ifdef NeXT_CPLUS
    if (DECL_NAME(TYPE_NAME(pointer_to)) &&
           (TREE_CODE(DECL_NAME(TYPE_NAME(pointer_to))) == IDENTIFIER_NODE))
      {
      char *name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(pointer_to)));
#else
    if (TYPE_NAME(pointer_to) &&
	TREE_CODE(TYPE_NAME(pointer_to)) == IDENTIFIER_NODE)
      {
      char *name = IDENTIFIER_POINTER(TYPE_NAME(pointer_to));
#endif

      if ((strcmp(name, TAG_OBJECT) == 0) ||		/* '@' */
	  (TREE_STATIC_TEMPLATE(pointer_to)))
        {
    	strcat(typeStr, "@");
        return;
        }
      else if (strcmp(name, TAG_CLASS) == 0)		/* '#' */
        {
        strcat(typeStr, "#");
        return;
        }
#ifdef NeXT_SELS_R_STRUCT_PTRS
      else if (strcmp(name, TAG_SELECTOR) == 0)		/* ':' */
        {
        strcat(typeStr, ":");
        return;
        }
#endif  /* NeXT_SELS_R_STRUCT_PTRS */
      }
    }
  else if (TREE_CODE(pointer_to) == INTEGER_TYPE && 
	   TYPE_MODE(pointer_to) == QImode)
    {
    strcat(typeStr, "*");
    return;
    }

  /* we have a type that does not get special treatment... */

  /* NeXT extension */
  strcat(typeStr, "^");
  encode_type(pointer_to,typeStr,format); 		
}

static void encode_array(tree type, char *typeStr, int format)
{
  tree anIntCst = TYPE_SIZE(type);
  tree array_of = TREE_TYPE(type);

  sprintf(typeStr+strlen(typeStr),"[%d",
	  TREE_INT_CST_LOW(anIntCst)/TREE_INT_CST_LOW(TYPE_SIZE(array_of)));
  encode_type(array_of,typeStr,format);
  strcat(typeStr,"]");
  return;
}

static void encode_aggregate(tree type, char *typeStr, int format)
{
  enum tree_code code = TREE_CODE(type);

  switch (code)
    {
      case RECORD_TYPE:
        {
          if (typeStr[strlen(typeStr)-1] == '^')
	    {
	    /* we have a reference - this is a NeXT extension */
#ifdef NeXT_CPLUS
            if (TYPE_NAME(type) &&
                (TREE_CODE(TYPE_NAME(type)) == TYPE_DECL) &&
		(TREE_CODE(DECL_NAME(TYPE_NAME(type))) == IDENTIFIER_NODE))
	      sprintf(typeStr+strlen(typeStr),"{%s}",
	  	      IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type))));
#else
            if (TYPE_NAME(type) &&
                (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE))
	      sprintf(typeStr+strlen(typeStr),"{%s}",
	  	      IDENTIFIER_POINTER(TYPE_NAME(type)));
#endif
	    else /* we have an untagged structure or a typedef */
	      sprintf(typeStr+strlen(typeStr),"{?}");
	    }
          else
	    {
	    tree fields = TYPE_FIELDS(type);

            if (format == NeXT_ENCODE_INLINE_DEFS)
              {
	      strcat(typeStr,"{");
	      for ( ; fields; fields = TREE_CHAIN(fields))
                encode_field_decl(fields, typeStr, format);
	      strcat(typeStr,"}");
              }
            else
              {
              if (TYPE_NAME(type) &&
                  (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE))
	        sprintf(typeStr+strlen(typeStr),"{%s}",
	  	        IDENTIFIER_POINTER(TYPE_NAME(type)));
	      else /* we have an untagged structure or a typedef */
	        sprintf(typeStr+strlen(typeStr),"{?}");
              }
	    }
          break;
	}
      case UNION_TYPE:
        {
          if (typeStr[strlen(typeStr)-1] == '^')
	    {
            if (TYPE_NAME(type) &&
                (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE))
	      /* we have a reference - this is a NeXT extension */
	      sprintf(typeStr+strlen(typeStr),"(%s)",
	  	      IDENTIFIER_POINTER(TYPE_NAME(type)));
	    else /* we have an untagged structure */
	      sprintf(typeStr+strlen(typeStr),"(?)");
	    }
          else
	    {
	    tree fields = TYPE_FIELDS(type);

            if (format == NeXT_ENCODE_INLINE_DEFS)
              {
	      strcat(typeStr,"(");
	      for ( ; fields; fields = TREE_CHAIN(fields))
                encode_field_decl(fields, typeStr, format);
	      strcat(typeStr,")");
              }
            else
              {
              if (TYPE_NAME(type) &&
                  (TREE_CODE(TYPE_NAME(type)) == IDENTIFIER_NODE))
	        /* we have a reference - this is a NeXT extension */
	        sprintf(typeStr+strlen(typeStr),"(%s)",
	  	        IDENTIFIER_POINTER(TYPE_NAME(type)));
	      else /* we have an untagged structure */
	        sprintf(typeStr+strlen(typeStr),"(?)");
              }
	    }
          break;
	}
      case ENUMERAL_TYPE:
        strcat(typeStr,"i");
        break;
    }
}

/*
 *  support bitfields, the current version of Objective-C does not support
 *  them. the string will consist of one or more "b:n"'s where n is an 
 *  integer describing the width of the bitfield. Currently, classes in 
 *  the kit implement a method "-(char *)describeBitfieldStruct:" that
 *  simulates this...if they do not implement this method, the archiver
 *  assumes the bitfield is 16 bits wide (padded if necessary) and packed
 *  according to the GNU compiler. After looking at the "kit", it appears
 *  that all classes currently rely on this default behavior, rather than
 *  hand generating this string (which is tedious).
 */
static void encode_bitfield(int width, char *typeStr, int format)
{
  sprintf(typeStr+strlen(typeStr),"b%d", width);
}

/*
 *	format will be:
 *
 *	NeXT_ENCODE_INLINE_DEFS or NeXT_ENCODE_DONT_INLINE_DEFS
 */
static void encode_type(tree type, char *typeStr, int format)
{
  enum tree_code code = TREE_CODE(type);

  if (code == INTEGER_TYPE)
    {
    if (TREE_INT_CST_LOW(TYPE_MIN_VALUE(type)) == 0)
      {
      /* unsigned integer types */

      if (TYPE_MODE(type) == QImode)		/* 'C' */
        strcat(typeStr, "C");
      else if (TYPE_MODE(type) == HImode)	/* 'S' */
        strcat(typeStr, "S");
      else if (TYPE_MODE(type) == SImode)
	{
        if (type == long_unsigned_type_node)
          strcat(typeStr, "L");			/* 'L' */
        else
          strcat(typeStr, "I");			/* 'I' */
        }
      }
    else /* signed integer types */
      {
      if (TYPE_MODE(type) == QImode)		/* 'c' */
        strcat(typeStr, "c");
      else if (TYPE_MODE(type) == HImode)	/* 's' */
        strcat(typeStr, "s");
      else if (TYPE_MODE(type) == SImode)	/* 'i' */
	{
        if (type == long_integer_type_node)
          strcat(typeStr, "l");			/* 'l' */
        else
          strcat(typeStr, "i");			/* 'i' */
        }
      }
    }
  else if (code == REAL_TYPE)
    {
    /* floating point types */

    if (TYPE_MODE(type) == SFmode)		/* 'f' */
      strcat(typeStr, "f");
    else if (TYPE_MODE(type) == DFmode ||
	     TYPE_MODE(type) == TFmode)		/* 'd' */
      strcat(typeStr, "d");
    }

  else if (code == VOID_TYPE)			/* 'v' */
    strcat(typeStr, "v");

  else if (code == ARRAY_TYPE)
    encode_array(type, typeStr, format);
  
  else if (code == POINTER_TYPE)
    encode_pointer(type, typeStr, format);

  else if (code == RECORD_TYPE || code == UNION_TYPE || code == ENUMERAL_TYPE)
    encode_aggregate(type, typeStr, format);

  else if (code == FUNCTION_TYPE) 		/* '?' */
    strcat(typeStr,"?");
}

static
void encode_field_decl(tree field_decl, char *typeStr, int format)
{
  if (TREE_PACKED(field_decl))
    encode_bitfield(DECL_SIZE_UNIT(field_decl), typeStr, format);
  else
    encode_type(TREE_TYPE(field_decl), typeStr, format);
}

static tree expr_last(tree complex_expr)
{
  tree next;

  if (complex_expr)
    while (next = TREE_OPERAND(complex_expr, 0))
      complex_expr = next;
  return complex_expr;
}

static tree cplus_parmlist;
/*
 *  Transform a method definition into a function definition as follows:
 *
 *  - synthesize the first two arguments, "self" and "_cmd".
 */

void
start_method_def(tree method)
{
  tree decl_specs;

  /* required to implement _msgSuper() */
  method_context = method;	
  _OBJC_SUPER_decl = NULLT;

#ifndef NeXT_CPLUS
  pushlevel(0); 		/* must be called BEFORE "start_function()" */
#endif

  /* generate prototype declarations for arguments..."new-style" */

  if (TREE_CODE(method_context) == INSTANCE_METHOD_DECL)
    decl_specs = build_tree_list(NULLT, _PRIVATE_record);
  else
    /* really a `struct objc_class *'...however we allow people to 
       assign to self...which changes its type midstream.
     */
    decl_specs = build_tree_list(NULLT, objc_object_reference);

  cplus_parmlist = push_parm_decl(0, build_tree_list(decl_specs, 
		 		 build(INDIRECT_REF,NULLT,self_id)));

#ifdef NeXT_SELS_R_INTS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_UNSIGNED]);
  decl_specs = tree_cons(NULLT, ridpointers[RID_INT], decl_specs);
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, _cmd_id));
#endif

#ifdef NeXT_SELS_R_CHAR_PTRS
  decl_specs = build_tree_list(NULLT, ridpointers[RID_CHAR]);

#ifdef NeXT_SELS_R_INDIRECT
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, 
  		build(INDIRECT_REF, NULLT, build(INDIRECT_REF,NULLT,_cmd_id))));
#else
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, 
				 build(INDIRECT_REF,NULLT,_cmd_id)));
#endif

#endif

#ifdef NeXT_SELS_R_STRUCTS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, _cmd_id));
#endif

#ifdef NeXT_SELS_R_STRUCT_PTRS
  decl_specs = build_tree_list(NULLT, objc_xref_tag(RECORD_TYPE,
					       get_identifier(TAG_SELECTOR)));

#ifdef NeXT_SELS_R_INDIRECT
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, 
  		build(INDIRECT_REF, NULLT, build(INDIRECT_REF,NULLT,_cmd_id))));
#else
  cplus_parmlist = push_parm_decl(cplus_parmlist, build_tree_list(decl_specs, 
				 build(INDIRECT_REF,NULLT,_cmd_id)));
#endif

#endif

  /* generate argument delclarations if a keyword_decl */
  if (method->method.sel_args)
    {
      tree arglist = method->method.sel_args; 
      do 
	{
        tree arg_spec = TREE_PURPOSE(TREE_TYPE(arglist));
        tree arg_decl = TREE_VALUE(TREE_TYPE(arglist));

        if (arg_decl)
	  {
          tree last_expr = expr_last(arg_decl);

	  /* unite the abstract decl with its name */
	  TREE_OPERAND(last_expr,0) = arglist->keyword.arg_name;
          push_parm_decl(cplus_parmlist, build_tree_list(arg_spec, arg_decl));
#if 0
          /* unhook...restore the abstract declarator */
          TREE_OPERAND(last_expr,0) = NULLT;
#endif
	  }
	else
          push_parm_decl(cplus_parmlist, build_tree_list(arg_spec, arglist->keyword.arg_name));

        arglist = TREE_CHAIN(arglist);
	}
      while (arglist); 
    }

  if (method->method.add_args > (tree)1)
    {
    /* we have a variable length selector - in "prototype" format */
    tree akey = TREE_PURPOSE(method->method.add_args);
    while (akey)
      {
	/* this must be done prior to calling pushdecl(). pushdecl() is
	 * going to change our chain on us...
	 */
	tree nextkey = TREE_CHAIN(akey);
	pushdecl(akey);
	akey = nextkey;
      }
    }
}

static void error_with_method(char *message, char mtype, tree method)
{
  extern int errorcount;

  fprintf(stderr,"%s:%d: ",method->method.filename,method->method.linenum);
  bzero(errbuf,BUFSIZE);
  fprintf(stderr,"%s `%c%s'\n", message, mtype, genMethodDecl(method,errbuf));
  errorcount++;
}

static void warn_with_method(char *message, char mtype, tree method)
{
  fprintf(stderr,"%s:%d: ",method->method.filename,method->method.linenum);
  bzero(errbuf,BUFSIZE);
  fprintf(stderr,"%s `%c%s'\n", message, mtype, genMethodDecl(method,errbuf));
}

/* return 1 if `method' is consistent with `proto' */

static int comp_method_with_proto(tree method, tree proto)
{
  static tree function_type = 0;

  /* create a function_type node once */
  if (!function_type)
    {
    struct obstack *ambient_obstack = current_obstack;
    current_obstack = &permanent_obstack;
    function_type = make_node(FUNCTION_TYPE);
    current_obstack = ambient_obstack;
    }
    
  /* install argument types - normally set by "build_function_type()". */
  TYPE_ARG_TYPES(function_type) = getArgTypeList(proto, METHOD_DEF, 0);

  /* install return type */
  TREE_TYPE(function_type) = groktypename(TREE_TYPE(proto));

#ifdef NeXT_CPLUS
  return comptypes(TREE_TYPE(method->method.mth_defn), function_type, 0);
#else
  return comptypes(TREE_TYPE(method->method.mth_defn), function_type);
#endif
}

/* return 1 if `proto1' is consistent with `proto2' */

static int comp_proto_with_proto(tree proto1, tree proto2)
{
  static tree function_type1 = 0, function_type2 = 0;

  /* create a couple function_type node's once */
  if (!function_type1)
    {
    struct obstack *ambient_obstack = current_obstack;
    current_obstack = &permanent_obstack;
    function_type1 = make_node(FUNCTION_TYPE);
    function_type2 = make_node(FUNCTION_TYPE);
    current_obstack = ambient_obstack;
    }

  /* install argument types - normally set by "build_function_type()". */
  TYPE_ARG_TYPES(function_type1) = getArgTypeList(proto1, METHOD_REF, 0);
  TYPE_ARG_TYPES(function_type2) = getArgTypeList(proto2, METHOD_REF, 0);

  /* install return type */
  TREE_TYPE(function_type1) = groktypename(TREE_TYPE(proto1));
  TREE_TYPE(function_type2) = groktypename(TREE_TYPE(proto2));

#ifdef NeXT_CPLUS
  return comptypes(function_type1, function_type2, 0);
#else
  return comptypes(function_type1, function_type2);
#endif
}

/*
 *  - generate an identifier for the function. the format is "_n_cls",
 *    where 1 <= n <= nMethods, and cls is the name the implementation we
 *    are processing.
 *  - install the return type from the method declaration.
 *  - if we have a prototype, check for type consistency.
 */
static void reallyStartMethod(tree method, tree parmlist)
{
  tree sc_spec, ret_spec, ret_decl, decl_specs;
  tree method_decl, method_id;
  char buf[256];

  /* synth the storage class & assemble the return type */
  sc_spec = tree_cons(NULLT, ridpointers[RID_STATIC], NULLT);
  ret_spec = TREE_PURPOSE(TREE_TYPE(method));
  decl_specs = chainon(sc_spec,ret_spec);

  if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
#if 0
    sprintf(buf,"_%d_%s",++method_slot,
		IDENTIFIER_POINTER(implementation_context->class.my_name));
#else
    sprintf(buf,"%c[%s %s]",
		(TREE_CODE(method) == INSTANCE_METHOD_DECL) ? '-' : '+',
		IDENTIFIER_POINTER(implementation_context->class.my_name),
		IDENTIFIER_POINTER(method->method.sel_name));
#endif
  else /* we have a category */
#if 0
    sprintf(buf,"_%d_%s_%s",++method_slot,
		IDENTIFIER_POINTER(implementation_context->class.my_name),
		IDENTIFIER_POINTER(implementation_context->class.super_name)); 
#else
    sprintf(buf,"%c[%s(%s) %s]",
		(TREE_CODE(method) == INSTANCE_METHOD_DECL) ? '-' : '+',
		IDENTIFIER_POINTER(implementation_context->class.my_name),
		IDENTIFIER_POINTER(implementation_context->class.super_name), 
		IDENTIFIER_POINTER(method->method.sel_name));
#endif

  method_id = get_identifier(buf);

  method_decl = build_nt(CALL_EXPR, method_id, parmlist, NULLT);

  /* check the delclarator portion of the return type for the method */
  if (ret_decl = TREE_VALUE(TREE_TYPE(method)))
    { /* 
       * unite the complex decl (specified in the abstract decl) with the 
       * function decl just synthesized...(int *), (int (*)()), (int (*)[]).
       */
      tree save_expr = expr_last(ret_decl);

      TREE_OPERAND(save_expr,0) = method_decl;
      method_decl = ret_decl;
#ifdef NeXT_CPLUS
      /* fool the parser into thinking it is starting a function */
      start_function(decl_specs, method_decl, NULLT, 0);
      /* must be called AFTER "start_function()" */
      store_parm_decls(); 	
      /*pushlevel(0);
      clear_last_expr();
      push_momentary();
      expand_start_bindings(0);*/
#else
      /* fool the parser into thinking it is starting a function */
      start_function(decl_specs, method_decl);
#endif
      /* unhook...this has the effect of restoring the abstract declarator */
      TREE_OPERAND(save_expr,0) = NULLT;
    }
  else
    {
    TREE_VALUE(TREE_TYPE(method)) = method_decl;
#ifdef NeXT_CPLUS
    /* fool the parser into thinking it is starting a function */
    start_function(decl_specs, method_decl, NULLT, 0);
    /* must be called AFTER "start_function()" */
    store_parm_decls(); 	
#else
    /* fool the parser into thinking it is starting a function */
    start_function(decl_specs, method_decl);
#endif
    /* unhook...this has the effect of restoring the abstract declarator */
    TREE_VALUE(TREE_TYPE(method)) = NULLT;
    }
#ifdef NeXT_CPLUS
  /* set self_decl from the first argument...this global is used by 
   * build_ivar_reference().build_indirect_ref().
   */
  self_decl = DECL_ARGUMENTS(current_function_decl);
#endif

  method->method.mth_defn = current_function_decl;
  
  /* Mark the decl as used to avoid "defined but not used" warning. */
  TREE_USED(method->method.mth_defn) = 1;

  /* check consistency...start_function(), pushdecl(), duplicate_decls(). */

  if (implementation_template != implementation_context)
    {
    tree chain, proto;

    if (TREE_CODE(method) == INSTANCE_METHOD_DECL)
      chain = implementation_template->class.nst_method_chain;
    else
      chain = implementation_template->class.cls_method_chain;

    if (proto = lookup_method(chain, method->method.sel_name))
      {
      if (!comp_method_with_proto(method,proto))
        {
        fprintf(stderr,"%s: In method `%s'\n",input_filename,
		IDENTIFIER_POINTER(method->method.sel_name));
        if (TREE_CODE(method) == INSTANCE_METHOD_DECL)
          {
          error_with_method("conflicting types for", '-', method);
          error_with_method("previous declaration of", '-', proto);
          }
        else
          {
          error_with_method("conflicting types for", '+', method);
          error_with_method("previous declaration of", '+', proto);
          }
        }
      }
    }
}

/*
 * the following routine is always called...this "architecture" is to
 * accommodate "old-style" variable length selectors.
 * 
 *	- a:a b:b // prototype  ; id c; id d; // old-style 
 */
void continue_method_def()
{
  tree parmlist;

  if (method_context->method.add_args == (tree)1)
    /* 
     * we have a `,...' immediately following the selector.
     */
    parmlist = get_parm_info(0, cplus_parmlist);
  else 
    parmlist = get_parm_info(1, cplus_parmlist); /* place a `void_at_end' */
#if 0
  /* set self_decl from the first argument...this global is used by 
   * build_ivar_reference().build_indirect_ref().
   */
  self_decl = TREE_PURPOSE(parmlist);
#endif

#if 0
  poplevel(0,0,0); 	/* must be called BEFORE "start_function()" */
#endif

  reallyStartMethod(method_context, parmlist);

#if 0
  store_parm_decls(); 	/* must be called AFTER "start_function()" */
#endif
}

void
add_objc_decls()
{
  if (!_OBJC_SUPER_decl)
#ifdef NeXT_CPLUS
    _OBJC_SUPER_decl = start_decl(get_identifier(/*_TAG_SUPER*/"XXX$SUP"),
			  build_tree_list(NULLT, objc_super_template), 0, NULLT);
#else
    _OBJC_SUPER_decl = start_decl(get_identifier(/*_TAG_SUPER*/"XXX$SUP"),
			  build_tree_list(NULLT, objc_super_template), 0);
#endif

#ifdef NeXT_CPLUS
  finish_decl(_OBJC_SUPER_decl, NULLT, NULLT);
#endif
  /* this prevents `unused variable' warnings when compiling with `-Wall' */
  TREE_USED(_OBJC_SUPER_decl) = 1;
}

/* 
 *	_n_Method(id self, SEL sel, ...)
 *	{
 *		struct objc_super _S;
 *
 *		_msgSuper((_S.self = self, _S.class = _cls, &_S), ...);
 *	}
 */
tree get_super_receiver()
{
  if (method_context)
    { 
    tree super_expr, super_expr_list;

    /* set receiver to self */
#ifdef NeXT_CPLUS
    super_expr = build_component_ref(_OBJC_SUPER_decl, self_id, NULLT, 1);
#else
    super_expr = build_component_ref(_OBJC_SUPER_decl, self_id);
#endif
    super_expr = build_modify_expr(super_expr, NOP_EXPR, self_decl);
    super_expr_list = build_tree_list (NULLT, super_expr);

    /* set class to begin searching */
#ifdef NeXT_CPLUS
    super_expr = build_component_ref(_OBJC_SUPER_decl, get_identifier("class"),
					NULLT, 1);
#else
    super_expr = build_component_ref(_OBJC_SUPER_decl, get_identifier("class"));
#endif

    if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
      {
      /* [_cls,__cls]Super are "pre-built" in synth_foward_declarations() */

      if (TREE_CODE(method_context) == INSTANCE_METHOD_DECL)
        super_expr = build_modify_expr(super_expr, NOP_EXPR, _clsSuper_ref);
      else
        super_expr = build_modify_expr(super_expr, NOP_EXPR, __clsSuper_ref);
      }
    else /* we have a category... */
      {
      tree params, super_name = implementation_template->class.super_name;
      tree funcCall;

      if (!super_name)	/* Barf if super used in a category of Object */
        {
          error("no super class declared in interface for `%s'",
                IDENTIFIER_POINTER( implementation_template->class.my_name));
	  return error_mark_node;
	}

      add_class_reference(super_name);

      params = build_tree_list(NULLT, 
	     my_build_string(IDENTIFIER_LENGTH(super_name) + 1,
	     	             IDENTIFIER_POINTER(super_name)));

      if (TREE_CODE(method_context) == INSTANCE_METHOD_DECL)
        funcCall = build_function_call(objc_getClass_decl, params);
      else
        funcCall = build_function_call(objc_getMetaClass_decl, params);

      /* cast! */
      TREE_TYPE(funcCall) = TREE_TYPE(_clsSuper_ref); 
      super_expr = build_modify_expr(super_expr, NOP_EXPR, funcCall);
      }
    chainon (super_expr_list, build_tree_list (NULL_TREE, super_expr));

    super_expr = build_unary_op(ADDR_EXPR,_OBJC_SUPER_decl,0);
    chainon (super_expr_list, build_tree_list (NULL_TREE, super_expr));

    return build_compound_expr(super_expr_list);
    }
  else
    {
    error("[super ...] must appear in a method context");
    return error_mark_node;
    }
}


static tree encode_method_def(tree func_decl)
{
  tree parms;
  int stack_size = 0;

  bzero(utlbuf,BUFSIZE);

  /* return type */
  encode_type(TREE_TYPE(TREE_TYPE(func_decl)), utlbuf, 
			NeXT_ENCODE_DONT_INLINE_DEFS);
  /* stack size */
  for (parms = DECL_ARGUMENTS(func_decl); parms;
       parms = TREE_CHAIN(parms))
    stack_size += TREE_INT_CST_LOW(TYPE_SIZE(DECL_ARG_TYPE(parms)));

  sprintf(&utlbuf[strlen(utlbuf)],"%d",stack_size);

  /* argument types */
  for (parms = DECL_ARGUMENTS(func_decl); parms;
       parms = TREE_CHAIN(parms))
    {
    int offset_in_bytes;

    /* type */ 
    encode_type(TREE_TYPE(parms), utlbuf, NeXT_ENCODE_DONT_INLINE_DEFS);

    /* compute offset */
    if (DECL_OFFSET (parms) >= 0)
      {
      offset_in_bytes = DECL_OFFSET (parms) / BITS_PER_UNIT;

      /* This is the case where the parm is passed as an int or double
         and it is converted to a char, short or float and stored back
	 in the parmlist.  In this case, describe the parm
	 with the variable's declared type, and adjust the address
	 if the least significant bytes (which we are using) are not
	 the first ones.  */
#ifdef BYTES_BIG_ENDIAN
      if (TREE_TYPE (parms) != DECL_ARG_TYPE (parms))
	offset_in_bytes += (GET_MODE_SIZE (TYPE_MODE (DECL_ARG_TYPE (parms)))
			      - GET_MODE_SIZE (GET_MODE (DECL_RTL (parms))));
#endif
      }
    sprintf(&utlbuf[strlen(utlbuf)],"%d",offset_in_bytes);
    }

  return get_identifier(utlbuf);
}

void 
finish_method_def()
{
  method_context->method.encode_types = 
		encode_method_def(current_function_decl);

  finish_function(lineno, 0);

  /* this must be done AFTER finish_function, since the optimizer may
     find "may be used before set" errors.  */
  method_context = NULLT; /* required to implement _msgSuper() */
}

char *
compiling_a_method()
{
  if (method_context)
    return IDENTIFIER_POINTER(method_context->method.sel_name); 
  else
    return 0;
}

static
int isComplexDecl(tree type)
{
    return (TREE_CODE(type) == ARRAY_TYPE || 
	    TREE_CODE(type) == FUNCTION_TYPE ||
	    TREE_CODE(type) == POINTER_TYPE);
}


static char tmpbuf[256];

/*
 * C unary `*'. One operand, an expression for a pointer.
 *
 * DEFTREECODE (INDIRECT_REF, "indirect_ref", "r", 1)
 *
 * Array indexing in languages other than C.
 * Operand 0 is the array; operand 1 is a list of indices
 * stored as a chain of TREE_LIST nodes.
 *
 * DEFTREECODE (ARRAY_REF, "array_ref", "r", 2)
 *
 * Function call.  Operand 0 is the function.
 * Operand 1 is the argument list, a list of expressions
 * made out of a chain of TREE_LIST nodes.
 * There is no operand 2.  That slot is used for the
 * CALL_EXPR_RTL macro (see preexpand_calls).
 * 
 * DEFTREECODE (CALL_EXPR, "call_expr", "e", 3)
 *
 */

static 
void adornDecl(tree decl, char *str)
{
  enum tree_code code = TREE_CODE(decl);

  if (code == ARRAY_REF)
    {
    tree anIntCst = TREE_OPERAND(decl,1);

    sprintf(str+strlen(str),"[%d]", TREE_INT_CST_LOW(anIntCst));
    }
  else if (code == ARRAY_TYPE)
    {
    tree anIntCst = TYPE_SIZE(decl);
    tree array_of = TREE_TYPE(decl);

    sprintf(str+strlen(str),"[%d]",
            TREE_INT_CST_LOW(anIntCst)/TREE_INT_CST_LOW(TYPE_SIZE(array_of)));
    }
  else if (code == CALL_EXPR)
    strcat(str,"()");
  else if (code == FUNCTION_TYPE)
    {
    tree chain  = TYPE_ARG_TYPES(decl);	/* a list of types */
    strcat(str,"(");
    while (chain && TREE_VALUE(chain) != void_type_node)
      {
      genDeclaration(TREE_VALUE(chain), str);
      chain = TREE_CHAIN(chain);
      if (chain && TREE_VALUE(chain) != void_type_node)
        strcat(str,",");
      }
    strcat(str,")");
    }
  else 
    {
    strcpy(tmpbuf,"*"); strcat(tmpbuf,str);
    strcpy(str,tmpbuf);
    }
}

static
char *genDeclarator(tree decl_expr, char *buf, char *name)
{
  if (decl_expr)
    {
    enum tree_code code = TREE_CODE(decl_expr);
    char *str;
    tree op;
    int wrap = 0;

    switch (code)
      {
      case ARRAY_REF: case INDIRECT_REF: case CALL_EXPR:
	{
        op = TREE_OPERAND(decl_expr, 0);

	/* we have a pointer to a function or array...(*)(),(*)[] */
	if ((code == ARRAY_REF || code == CALL_EXPR) &&
	    (op && TREE_CODE(op) == INDIRECT_REF))
	  wrap = 1;

        str = genDeclarator(op, buf, name);

        if (wrap)
          {
          strcpy(tmpbuf,"("); strcat(tmpbuf,str); strcat(tmpbuf,")");
          strcpy(str,tmpbuf);
          }

	adornDecl(decl_expr,str);
	break;
	}
      case ARRAY_TYPE: case FUNCTION_TYPE: case POINTER_TYPE:
	{
        str = strcpy(buf,name);

        /* this clause is done iteratively...rather than recursively */
	do
	  {
          op = isComplexDecl(TREE_TYPE(decl_expr)) 
	         ? TREE_TYPE(decl_expr) 
	         : NULLT;

	  adornDecl(decl_expr,str);

	  /* we have a pointer to a function or array...(*)(),(*)[] */
	  if ((code == POINTER_TYPE) &&
	      (op && (TREE_CODE(op) == FUNCTION_TYPE ||
		    TREE_CODE(op) == ARRAY_TYPE)))
            {
            strcpy(tmpbuf,"("); strcat(tmpbuf,str); strcat(tmpbuf,")");
            strcpy(str,tmpbuf);
            }

          decl_expr = isComplexDecl(TREE_TYPE(decl_expr)) 
	                ? TREE_TYPE(decl_expr) 
	                : NULLT;
          }
        while (decl_expr && (code = TREE_CODE(decl_expr)));

	break;
	}
      case IDENTIFIER_NODE:
	/* will only happen if we are processing a "raw" expr-decl. */
	return strcpy(buf,IDENTIFIER_POINTER(decl_expr));
      }

    return str;
    }
  else /* we have an abstract declarator or a _DECL node */
    {
    return strcpy(buf,name);
    }
}

static 
void gen_declspecs(tree declspecs, char *buf, int raw)
{
  if (raw)
    {
    tree chain;

    for (chain = declspecs; chain; chain = TREE_CHAIN(chain))
      {
        tree aspec = TREE_VALUE(chain);

        if (TREE_CODE(aspec) == IDENTIFIER_NODE)
          strcat(buf, IDENTIFIER_POINTER(aspec));
        else if (TREE_CODE(aspec) == RECORD_TYPE)
          {
	  if (TYPE_NAME(aspec))
	    {
    	      if (!TREE_STATIC_TEMPLATE(aspec))
	        strcat(buf,"struct ");
	      strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(aspec)));
	    }
	  else
	    strcat(buf,"untagged struct");
	  }
        else if (TREE_CODE(aspec) == UNION_TYPE)
	  {
	  if (TYPE_NAME(aspec))
	    {
	      strcat(buf,"union ");
	      strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(aspec)));
	    }
	  else
	    strcat(buf,"untagged union");
	  }
        else if (TREE_CODE(aspec) == ENUMERAL_TYPE)
          {
	  if (TYPE_NAME(aspec))
	    {
	      strcat(buf,"enum ");
	      strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(aspec)));
	    }
	  else
	    strcat(buf,"untagged enum");
	  }
        strcat(buf, " ");
      }
    }
  else
    switch (TREE_CODE(declspecs))
      {
      /* type specifiers */

      case INTEGER_TYPE: /* signed integer types */

        if (declspecs == short_integer_type_node)	/* 's' */
          strcat(buf, "short int ");
        else if (declspecs == integer_type_node)	/* 'i' */
          strcat(buf, "int ");
        else if (declspecs == long_integer_type_node)	/* 'l' */
          strcat(buf, "long int ");
        else if (declspecs == signed_char_type_node ||	/* 'c' */
  	         declspecs == char_type_node)
          strcat(buf, "char ");

        /* unsigned integer types */

        else if (declspecs == short_unsigned_type_node)	/* 'S' */
          strcat(buf, "unsigned short ");
        else if (declspecs == unsigned_type_node)	/* 'I' */
          strcat(buf, "unsigned int ");
        else if (declspecs == long_unsigned_type_node)	/* 'L' */
          strcat(buf, "unsigned long ");
        else if (declspecs == unsigned_char_type_node)	/* 'C' */
          strcat(buf, "unsigned char ");
	break;

      case REAL_TYPE: /* floating point types */

        if (declspecs == float_type_node)		/* 'f' */
          strcat(buf, "float ");
        else if (declspecs == double_type_node)		/* 'd' */
          strcat(buf, "double ");
	else if (declspecs == long_double_type_node) 	/* 'd' */
          strcat(buf, "long double ");
	break;

      case RECORD_TYPE:
    	  if (!TREE_STATIC_TEMPLATE(declspecs))
	    strcat(buf,"struct ");
#ifdef NeXT_CPLUS
          if (DECL_NAME(TYPE_NAME(declspecs)) &&
                (TREE_CODE(DECL_NAME(TYPE_NAME(declspecs))) == IDENTIFIER_NODE))
#else
          if (TYPE_NAME(declspecs) &&
                (TREE_CODE(TYPE_NAME(declspecs)) == IDENTIFIER_NODE))
#endif
	    {
#ifdef NeXT_CPLUS
	    strcat(buf,IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(declspecs))));
#else
	    strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(declspecs)));
#endif
	    strcat(buf," ");
	    }
	  break;
      case UNION_TYPE:
	  strcat(buf,"union ");
          if (TYPE_NAME(declspecs) &&
                (TREE_CODE(TYPE_NAME(declspecs)) == IDENTIFIER_NODE))
	    {
	    strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(declspecs)));
	    strcat(buf," ");
	    }
	  break;
      case ENUMERAL_TYPE:
	  strcat(buf,"enum ");
          if (TYPE_NAME(declspecs) &&
                (TREE_CODE(TYPE_NAME(declspecs)) == IDENTIFIER_NODE))
	    {
	    strcat(buf,IDENTIFIER_POINTER(TYPE_NAME(declspecs)));
	    strcat(buf," ");
	    }
	  break;
      case VOID_TYPE:
    	  strcat(buf,"void ");
      }
}

static 
char *genDeclaration(tree atype_or_adecl, char *buf)
{
  char declbuf[256];	

  if (TREE_CODE(atype_or_adecl) == TREE_LIST)
    {
    tree declspecs;	/* "identifier_node", "record_type" */
    tree declarator;	/* "array_ref", "indirect_ref", "call_expr"... */

    /* we have a "raw", abstract delclarator (typename) */
    declarator = TREE_VALUE(atype_or_adecl);
    declspecs  = TREE_PURPOSE(atype_or_adecl);

    gen_declspecs(declspecs, buf, 1);
    strcat(buf, genDeclarator(declarator,declbuf,""));
    }
  else 
    {
    tree atype;
    tree declspecs;	/* "integer_type", "real_type", "record_type"... */
    tree declarator;	/* "array_type", "function_type", "pointer_type". */

    if (TREE_CODE(atype_or_adecl) == FIELD_DECL ||
        TREE_CODE(atype_or_adecl) == PARM_DECL ||
        TREE_CODE(atype_or_adecl) == FUNCTION_DECL)
      atype = TREE_TYPE(atype_or_adecl);
    else
      atype = atype_or_adecl;	/* assume we have a *_type node */

    if (isComplexDecl(atype))
      {
      tree chain;

      /* get the declaration specifier...it is at the end of the list */
      declarator = chain = atype;
      do
	chain = TREE_TYPE(chain);	/* not TREE_CHAIN(chain); */
      while (isComplexDecl(chain));
      declspecs = chain;
      }
    else
      {
      declspecs = atype;
      declarator = NULLT;
      }

    gen_declspecs(declspecs, buf, 0);

    if (TREE_CODE(atype_or_adecl) == FIELD_DECL ||
        TREE_CODE(atype_or_adecl) == PARM_DECL ||
        TREE_CODE(atype_or_adecl) == FUNCTION_DECL)
      {
      if (declarator)
        {
        strcat(buf,genDeclarator(declarator,declbuf,
		   IDENTIFIER_POINTER(DECL_NAME(atype_or_adecl))));
        }
      else
        strcat(buf,IDENTIFIER_POINTER(DECL_NAME(atype_or_adecl)));
      }
    else
      {
      strcat(buf,genDeclarator(declarator,declbuf,""));
      }
    }
  return buf;
}

#define RAW_TYPESPEC(meth) (TREE_VALUE(TREE_PURPOSE(TREE_TYPE(meth))))

static
char *genMethodDecl(tree method, char *buf) 
{
  tree chain;

  if (RAW_TYPESPEC(method) != objc_object_reference)
    {
    strcpy(buf,"(");
    genDeclaration(TREE_TYPE(method),buf);
    strcat(buf,")");
    }

  if (chain = method->method.sel_args)
    { /* we have a chain of keyword_decls */
      do
        {
        if (chain->keyword.key_name)
          strcat(buf, IDENTIFIER_POINTER(chain->keyword.key_name));

        strcat(buf, ":");
  	if (RAW_TYPESPEC(chain) != objc_object_reference)
          {
          strcat(buf,"(");
          genDeclaration(TREE_TYPE(chain),buf);
          strcat(buf,")");
	  }
        strcat(buf, IDENTIFIER_POINTER(chain->keyword.arg_name));
        if (chain = TREE_CHAIN(chain))
	  strcat(buf," ");
        }
      while (chain);

      if (method->method.add_args == (tree)1)
        strcat(buf,", ...");
      else if (method->method.add_args)
        { /* we have a tree list node as generate by `get_parm_info()' */
	  chain  = TREE_PURPOSE(method->method.add_args);
          /* know we have a chain of parm_decls */
          do
            {
            strcat(buf,", ");
	    genDeclaration(chain, buf);
            chain = TREE_CHAIN(chain);
            }
          while (chain);
	}
    }
  else /* we have a unary selector */
    {
    strcat(buf, IDENTIFIER_POINTER(method->method.sel_name));
    }

  return buf;
}

void genPrototype(FILE *fp, tree decl)
{
  /* we have a function definition - generate prototype */
  bzero(errbuf,BUFSIZE);
  genDeclaration(decl,errbuf);
  fprintf(fp,"%s;\n", errbuf);
}
/*
 *  debug info...
 */
static void
dump_interface(FILE *fp, tree chain)
{
  char *buf = (char *)malloc(256);
  char *my_name = IDENTIFIER_POINTER(chain->class.my_name);
  tree ivar_decls = chain->class.raw_ivars;
  tree nst_methods = chain->class.nst_method_chain;
  tree cls_methods = chain->class.cls_method_chain;

  fprintf(fp,"\n@interface %s",my_name);

  if (chain->class.super_name)
    {
    char *super_name = IDENTIFIER_POINTER(chain->class.super_name);
    fprintf(fp," : %s\n",super_name);
    }
  else
    fprintf(fp,"\n");

  if (ivar_decls)
    {
    fprintf(fp,"{\n");
    do
      {
      bzero(buf,256);
      fprintf(fp,"\t%s;\n", genDeclaration(ivar_decls,buf));
      ivar_decls = TREE_CHAIN(ivar_decls);
      }
    while (ivar_decls);
    fprintf(fp,"}\n");
    }

  while (nst_methods)
    {
    bzero(buf,256);
    fprintf(fp,"- %s;\n", genMethodDecl(nst_methods,buf));
    nst_methods = TREE_CHAIN(nst_methods);
    }

  while (cls_methods)
    {
    bzero(buf,256);
    fprintf(fp,"+ %s;\n", genMethodDecl(cls_methods,buf));
    cls_methods = TREE_CHAIN(cls_methods);
    }
  fprintf(fp,"\n@end");
}

/*
 *   called from toplev.compile_file();
 */
void init_objc()
{
  errbuf = (char *)malloc(BUFSIZE);
  utlbuf = (char *)malloc(BUFSIZE);
  hash_init();
  synth_module_prologue();
}

void finish_objc()
{
  extern int gen_declaration;
  extern FILE *gen_declaration_file;
  struct imp_entry *impent;
  tree chain, align_str = my_build_string(9, ".align 2");

  generate_forward_declaration_to_string_table();
#if 0
  assemble_asm(my_build_string(6, ".data"));
  assemble_asm(align_str);
#else
  /* Force the assembler to create all the Objective-C sections,
     so that their order is guaranteed. */
  {
    extern void objc_class_section ();
    extern void objc_meta_class_section ();
    extern void objc_cat_cls_meth_section ();
    extern void objc_cat_inst_meth_section ();
    extern void objc_cls_meth_section ();
    extern void objc_inst_meth_section ();
    extern void objc_selector_refs_section ();
    extern void objc_symbols_section ();
    extern void objc_category_section ();
    extern void objc_class_vars_section ();
    extern void objc_instance_vars_section ();
    extern void objc_module_info_section ();
    extern void objc_selector_strs_section ();
    
    objc_class_section ();
    objc_meta_class_section ();
    objc_cat_cls_meth_section ();
    objc_cat_inst_meth_section ();
    objc_cls_meth_section ();
    objc_inst_meth_section ();
    objc_selector_refs_section ();
    objc_symbols_section ();
    objc_category_section ();
    objc_class_vars_section ();
    objc_instance_vars_section ();
    objc_module_info_section ();
    objc_selector_strs_section ();
  }
#endif
  if (implementation_context || sel_refdef_chain)
    generate_objc_symtab_decl();

  for (impent = imp_list; impent; impent = impent->next)
    {
    implementation_context = impent->imp_context;
    implementation_template = impent->imp_template;

    _OBJC_CLASS_decl = impent->class_decl;
    _OBJC_METACLASS_decl = impent->meta_decl;

    if (TREE_CODE(implementation_context) == IMPLEMENTATION_TYPE)
      {
      // all of the following reference the string pool...
      generate_ivar_lists();
      generate_dispatch_tables();
      generate_shared_structures();
      }
    else
      {
      generate_dispatch_tables();
      generate_category(implementation_context);
      }
    }

  if (sel_ref_chain)
    build_selector_translation_table();

  if (implementation_context || sel_refdef_chain)
    {
#if 0
    assemble_asm(align_str);
#endif
    build_module_descriptor();
    }

  /* dump the string table last */

  if (sel_refdef_chain)
    {
#if 0
    assemble_asm(align_str);
#endif
    build_message_selector_pool();
    }

  /* dump the class references...this forces the appropriate classes
     to be linked into the executable image, preserving unix archive 
     semantics...this can be removed when we move to a more dynamically
     linked environment 
   */ 
  for (chain = cls_ref_chain; chain; chain = TREE_CHAIN(chain))
    {
    sprintf(utlbuf,".reference .objc_class_name_%s", 
		IDENTIFIER_POINTER(TREE_VALUE(chain)));
    assemble_asm(my_build_string(strlen(utlbuf)+1, utlbuf));
    }
  for (impent = imp_list; impent; impent = impent->next)
    {
    implementation_context = impent->imp_context;
    implementation_template = impent->imp_template;

    if (TREE_CODE(impent->imp_context) == IMPLEMENTATION_TYPE)
      {
      sprintf(utlbuf,".objc_class_name_%s=0", 
		IDENTIFIER_POINTER(impent->imp_context->class.my_name));
      assemble_asm(my_build_string(strlen(utlbuf)+1, utlbuf));

      sprintf(utlbuf,".globl .objc_class_name_%s", 
		IDENTIFIER_POINTER(impent->imp_context->class.my_name));
      assemble_asm(my_build_string(strlen(utlbuf)+1, utlbuf));
      }
    }
  /*** this fixes a gross bug in the assembler...it `expects' #APP to have
   *** a matching #NO_APP, or it crashes (sometimes). app_disable() will
   *** insure this is the case. 5/19/89, s.naroff.
   ***/
  if (cls_ref_chain || imp_list)
    app_disable();

  if (gen_declaration && implementation_context)
    {
    add_class(implementation_context);
    dump_interface(gen_declaration_file, implementation_context);
    }
  
  if (warn_selector)
    {
      int slot;
      
      /* Run through the selector hash tables and print a warning for any
         selector which has multiple methods. */
      
      for (slot = 0; slot < SIZEHASHTABLE; slot++)
        {
	  hash hsh;
	  
	  for (hsh = cls_method_hash_list[slot]; hsh; hsh = hsh->next)
	    {
	      if (hsh->list)
	        {
	          tree meth = hsh->key;
	          char type = (TREE_CODE (meth) == INSTANCE_METHOD_DECL)
			      ? '-' : '+';
	          attr loop;
	      
	          warn_with_method ("potential selector conflict for method",
				    type, meth);
	          for (loop = hsh->list; loop; loop = loop->next)
	            warn_with_method ("also found", type, loop->value);
	        }
	    }
	}
    
      for (slot = 0; slot < SIZEHASHTABLE; slot++)
        {
	  hash hsh;
	  
	  for (hsh = nst_method_hash_list[slot]; hsh; hsh = hsh->next)
	    {
	      if (hsh->list)
	        {
	          tree meth = hsh->key;
	          char type = (TREE_CODE (meth) == INSTANCE_METHOD_DECL)
			      ? '-' : '+';
	          attr loop;
	      
	          warn_with_method ("potential selector conflict for method",
				    type, meth);
	          for (loop = hsh->list; loop; loop = loop->next)
	            warn_with_method ("also found", type, loop->value);
	        }
	    }
	}
    }
}

#ifdef DEBUG

static void objc_debug(FILE *fp)
{
  char *buf = (char *)malloc(256);

    { /* dump function prototypes */
      tree loop = _OBJC_MODULES_decl;	

      fprintf(fp,"\n\nfunction prototypes:\n");
      while (loop)
	{
	  if (TREE_CODE(loop) == FUNCTION_DECL && DECL_INITIAL(loop))
	    {
	    /* we have a function definition - generate prototype */
            bzero(errbuf,BUFSIZE);
	    genDeclaration(loop,errbuf);
	    fprintf(fp,"%s;\n", errbuf);
	    }
	  loop = TREE_CHAIN(loop);
	}
    }
    { /* dump global chains */
      tree loop;
      int i, index = 0, offset = 0;
      hash hashlist;

      for (i = 0; i < SIZEHASHTABLE; i++)
	{
	if (hashlist = nst_method_hash_list[i])
	  {
          fprintf(fp,"\n\nnst_method_hash_list[%d]:\n",i);
	  do 
	    {
	    bzero(buf,256);
	    fprintf(fp,"-%s;\n", genMethodDecl(hashlist->key,buf));
	    hashlist = hashlist->next;
	    }
          while (hashlist);
	  }
	}
      for (i = 0; i < SIZEHASHTABLE; i++)
	{
	if (hashlist = cls_method_hash_list[i])
	  {
          fprintf(fp,"\n\ncls_method_hash_list[%d]:\n",i);
	  do 
	    {
	    bzero(buf,256);
	    fprintf(fp,"-%s;\n", genMethodDecl(hashlist->key,buf));
	    hashlist = hashlist->next;
	    }
          while (hashlist);
	  }
	}
      fprintf(fp,"\nsel_refdef_chain:\n");
      for (loop = sel_refdef_chain; loop; loop = TREE_CHAIN(loop))
	{
	  fprintf(fp,"(index: %4d offset: %4d) %s\n", index, offset,
		     IDENTIFIER_POINTER(TREE_VALUE(loop)));
          index++;
          /* add one for the '\0' character */
          offset += IDENTIFIER_LENGTH(TREE_VALUE(loop)) + 1;
	}
      fprintf(fp,"\n(max_selector_index: %4d.\n", max_selector_index);
    }
}
#endif


