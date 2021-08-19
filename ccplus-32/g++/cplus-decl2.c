/* Process declarations and variables for C compiler.
   Copyright (C) 1988 Free Software Foundation, Inc.
   Hacked by Michael Tiemann (tiemann@mcc.com)

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

#include "config.h"
#include "tree.h"
#include "flags.h"
#include "cplus-tree.h"
#include "cplus-parse.h"
#include "cplus-decl.h"
#include <assert.h>

#define NULL 0

extern tree grokdeclarator ();
static void grok_function_init ();

/* A list of virtual function tables we must make sure to write out.  */
tree pending_vtables;

/* A list of static class variables.  This is needed, because a
   static class variable can be declared inside the class without
   an initializer, and then initialized, staticly, outside the class.  */
tree pending_statics;

extern tree pending_addressable_inlines;

/* Used to help generate temporary names which are unique within
   a function.  Reset to 0 by start_function.  */

static int temp_name_counter;

/* Same, but not reset.  Local temp variables and global temp variables
   can have the same name.  */
static int global_temp_name_counter;

/* The (assembler) name of the first globally-visible object output.  */
extern char * first_global_object_name;

/* Incorporate `const' and `volatile' qualifiers for member functions.
   FUNCTION is a TYPE_DECL or a FUNCTION_DECL.
   QUALS is a list of qualifiers.  */
tree
grok_method_quals (ctype, function, quals)
     tree ctype, function, quals;
{
  tree fntype = TREE_TYPE (function);
  tree raises = TYPE_RAISES_EXCEPTIONS (fntype);

  assert (quals != NULL_TREE);
  do
    {
      extern tree ridpointers[];

      if (TREE_VALUE (quals) == ridpointers[(int)RID_CONST])
	{
	  if (TREE_READONLY (ctype))
	    error ("duplicate `%s' %s",
		   IDENTIFIER_POINTER (TREE_VALUE (quals)),
		   (TREE_CODE (function) == FUNCTION_DECL
		    ? "for member function" : "in type declaration"));
	  ctype = build_type_variant (ctype, 1, TREE_VOLATILE (ctype));
	  build_pointer_type (ctype);
	}
      else if (TREE_VALUE (quals) == ridpointers[(int)RID_VOLATILE])
	{
	  if (TREE_VOLATILE (ctype))
	    error ("duplicate `%s' %s",
		   IDENTIFIER_POINTER (TREE_VALUE (quals)),
		   (TREE_CODE (function) == FUNCTION_DECL
		    ? "for member function" : "in type declaration"));
	  ctype = build_type_variant (ctype, TREE_READONLY (ctype), 1);
	  build_pointer_type (ctype);
	}
      else
	abort ();
      quals = TREE_CHAIN (quals);
    }
  while (quals);
  fntype = build_cplus_method_type (ctype, TREE_TYPE (fntype),
				    (TREE_CODE (fntype) == METHOD_TYPE
				     ? TREE_CHAIN (TYPE_ARG_TYPES (fntype))
				     : TYPE_ARG_TYPES (fntype)));
  if (raises)
    fntype = build_exception_variant (ctype, fntype, raises);

  TREE_TYPE (function) = fntype;
  return ctype;
}


/* Classes overload their constituent function names automatically.
   When a function name is declared in a record structure,
   its name is changed to it overloaded name.  Since names for
   constructors and destructors can conflict, we place a leading
   '$' for destructors.

   CNAME is the name of the class we are grokking for.

   FUNCTION is a FUNCTION_DECL.  It was created by `grokdeclarator'.

   FLAGS contains bits saying what's special about today's
   arguments.  1 == DESTRUCTOR.  2 == OPERATOR.

   If FUNCTION is a destructor, then we must add the `auto-delete' field
   as a second parameter.  There is some hair associated with the fact
   that we must "declare" this variable in the manner consistent with the
   way the rest of the arguements were declared.

   If FUNCTION is a constructor, and we are doing SOS hacks for dynamic
   classes, then the second hidden argument is the virtual function table
   pointer with which to initialize the object.

   QUALS are the qualifiers for the this pointer.  */

void
grokclassfn (ctype, cname, function, flags, complain, quals)
     tree ctype, cname, function;
     enum overload_flags flags;
     tree quals;
{
  tree fn_name = DECL_NAME (function);
  tree arg_types;
  tree parm;
  char *name;

  if (fn_name == NULL_TREE)
    {
      error ("name missing for member function");
      fn_name = get_identifier ("<anonymous>");
      DECL_NAME (function) = DECL_ORIGINAL_NAME (function) = fn_name;
    }

  if (quals)
    ctype = grok_method_quals (ctype, function, quals);

  arg_types = TYPE_ARG_TYPES (TREE_TYPE (function));
  if (TREE_CODE (TREE_TYPE (function)) == METHOD_TYPE)
    {
      /* Must add the class instance variable up front.  */
      /* Right now we just make this a pointer.  But later
	 we may wish to make it special.  */
      tree type = TREE_VALUE (arg_types);

      if (flags == DTOR_FLAG)
	type = TYPE_MAIN_VARIANT (type);
      else if (DECL_CONSTRUCTOR_P (function))
	{
	  if (TYPE_DYNAMIC (ctype))
	    {
	      parm = build_decl (PARM_DECL, get_identifier (AUTO_VTABLE_NAME), TYPE_POINTER_TO (ptr_type_node));
	      TREE_USED (parm) = 1;
	      TREE_READONLY (parm) = 1;
	      DECL_ARG_TYPE (parm) = TYPE_POINTER_TO (ptr_type_node);
	      TREE_CHAIN (parm) = last_function_parms;
	      last_function_parms = parm;
	    }

	  if (TYPE_USES_VIRTUAL_BASECLASSES (ctype))
	    {
	      DECL_CONSTRUCTOR_FOR_VBASE_P (function) = 1;
	      /* In this case we need "in-charge" flag saying whether
		 this constructor is responsible for initialization
		 of virtual baseclasses or not.  */
	      parm = build_decl (PARM_DECL, in_charge_identifier, integer_type_node);
	      DECL_ARG_TYPE (parm) = integer_type_node;
	      TREE_REGDECL (parm) = 1;
	      TREE_CHAIN (parm) = last_function_parms;
	      last_function_parms = parm;
	    }
	}

      parm = build_decl (PARM_DECL, this_identifier, type);
      DECL_ARG_TYPE (parm) = type;
      /* We can make this a register, so long as we don't
	 accidently complain if someone tries to take its address.  */
      TREE_REGDECL (parm) = 1;
      if (flags != DTOR_FLAG
	  && (!flag_this_is_variable || TREE_READONLY (type)))
	TREE_READONLY (parm) = 1;
      TREE_CHAIN (parm) = last_function_parms;
      last_function_parms = parm;
    }

  if (flags == DTOR_FLAG)
    {
      tree const_integer_type = build_type_variant (integer_type_node, 1, 0);
      arg_types = hash_tree_chain (const_integer_type, void_list_node);
      name = (char *)alloca (sizeof (DESTRUCTOR_DECL_FORMAT)
			     + IDENTIFIER_LENGTH (cname) + 2);
      sprintf (name, DESTRUCTOR_DECL_FORMAT, IDENTIFIER_POINTER (cname));
      DECL_NAME (function) = get_identifier (name);
      DECL_ASSEMBLER_NAME (function) = IDENTIFIER_POINTER (DECL_NAME (function));
      parm = build_decl (PARM_DECL, in_charge_identifier, const_integer_type);
      TREE_USED (parm) = 1;
      TREE_READONLY (parm) = 1;
      DECL_ARG_TYPE (parm) = const_integer_type;
      /* This is the same chain as DECL_ARGUMENTS (fndecl).  */
      TREE_CHAIN (last_function_parms) = parm;

      TREE_TYPE (function) = build_cplus_method_type (ctype, void_type_node, arg_types);
      TYPE_HAS_DESTRUCTOR (ctype) = 1;
    }
  else if (flags == WRAPPER_FLAG || flags == ANTI_WRAPPER_FLAG)
    {
      name = (char *)alloca (sizeof (WRAPPER_DECL_FORMAT)
			     + sizeof (ANTI_WRAPPER_DECL_FORMAT)
			     + IDENTIFIER_LENGTH (cname) + 2);
      sprintf (name,
	       flags == WRAPPER_FLAG ? WRAPPER_DECL_FORMAT : ANTI_WRAPPER_DECL_FORMAT,
	       IDENTIFIER_POINTER (cname));
      DECL_NAME (function) = build_decl_overload (name, arg_types, 1);
      DECL_ASSEMBLER_NAME (function) = IDENTIFIER_POINTER (DECL_NAME (function));
      sprintf (name, flags == WRAPPER_FLAG ? WRAPPER_NAME_FORMAT : ANTI_WRAPPER_NAME_FORMAT,
	       IDENTIFIER_POINTER (cname));
      DECL_ORIGINAL_NAME (function) = fn_name = get_identifier (name);
    }
  else if (flags == WRAPPER_PRED_FLAG)
    {
      name = (char *)alloca (sizeof (WRAPPER_PRED_DECL_FORMAT)
			     + sizeof (WRAPPER_PRED_NAME_FORMAT)
			     + IDENTIFIER_LENGTH (cname) + 2);
      sprintf (name, WRAPPER_PRED_DECL_FORMAT, IDENTIFIER_POINTER (cname));
      DECL_NAME (function) = build_decl_overload (name, arg_types, 1);
      DECL_ASSEMBLER_NAME (function) = IDENTIFIER_POINTER (DECL_NAME (function));
      sprintf (name, WRAPPER_PRED_NAME_FORMAT, IDENTIFIER_POINTER (cname));
      DECL_ORIGINAL_NAME (function) = fn_name = get_identifier (name);
    }
  else
    {
      tree these_arg_types;

      if (TYPE_DYNAMIC (ctype) && DECL_CONSTRUCTOR_P (function))
	{
	  arg_types = hash_tree_chain (build_pointer_type (ptr_type_node),
				       TREE_CHAIN (arg_types));
	  TREE_TYPE (function)
	    = build_cplus_method_type (ctype, TREE_TYPE (TREE_TYPE (function)), arg_types);
	  arg_types = TYPE_ARG_TYPES (TREE_TYPE (function));
	}

      if (DECL_CONSTRUCTOR_FOR_VBASE_P (function))
	{
	  arg_types = hash_tree_chain (integer_type_node, TREE_CHAIN (arg_types));
	  TREE_TYPE (function)
	    = build_cplus_method_type (ctype, TREE_TYPE (TREE_TYPE (function)), arg_types);
	  arg_types = TYPE_ARG_TYPES (TREE_TYPE (function));
	}

      these_arg_types = arg_types;

      if (TREE_CODE (TREE_TYPE (function)) == FUNCTION_TYPE)
	/* Only true for static member functions.  */
	these_arg_types = hash_tree_chain (TYPE_POINTER_TO (ctype), arg_types);

      DECL_NAME (function) = build_decl_overload (IDENTIFIER_POINTER (fn_name),
						  these_arg_types,
						  1 + DECL_CONSTRUCTOR_P (function));
      DECL_ASSEMBLER_NAME (function) = IDENTIFIER_POINTER (DECL_NAME (function));
      if (flags == TYPENAME_FLAG)
	TREE_TYPE (DECL_NAME (function)) = TREE_TYPE (fn_name);
    }

  DECL_ARGUMENTS (function) = last_function_parms;

  /* now, the sanity check: report error if this function is not
     really a member of the class it is supposed to belong to.  */
  if (complain)
    {
      tree field;
      int need_quotes = 0;
      char *err_name;
      tree method_vec = CLASSTYPE_METHOD_VEC (ctype);
      tree *methods = 0;
      tree *end = 0;

      if (method_vec != 0)
	{
	  methods = &TREE_VEC_ELT (method_vec, 0);
	  end = TREE_VEC_END (method_vec);

	  if (*methods == 0)
	    methods++;

	  while (methods != end)
	    {
	      if (fn_name == DECL_ORIGINAL_NAME (*methods))
		{
		  field = *methods;
		  while (field)
		    {
		      if (DECL_NAME (function) == DECL_NAME (field))
			return;
		      field = TREE_CHAIN (field);
		    }
		  break;	/* loser */
		}
	      methods++;
	    }
	}

      if (OPERATOR_NAME_P (fn_name))
	{
	  err_name = (char *)alloca (1024);
	  sprintf (err_name, "`operator %s'", operator_name_string (fn_name));
	}
      else if (OPERATOR_TYPENAME_P (fn_name))
	if (complain && TYPE_HAS_CONVERSION (ctype))
	  err_name = "such type conversion operator";
	else
	  err_name = "type conversion operator";
      else if (flags == WRAPPER_FLAG)
	err_name = "wrapper";
      else if (flags == WRAPPER_PRED_FLAG)
	err_name = "wrapper predicate";
      else
	{
	  err_name = IDENTIFIER_POINTER (fn_name);
	  need_quotes = 1;
	}

      if (methods != end)
	if (need_quotes)
	  error ("argument list for `%s' does not match any in class", err_name);
	else
	  error ("argument list for %s does not match any in class", err_name);
      else
	{
	  methods = 0;
	  if (need_quotes)
	    error ("no `%s' member function declared in class", err_name);
	  else
	    error ("no %s declared in class", err_name);
	}

      /* If we did not find the method in the class, add it to
	 avoid spurious errors.  */
      add_method (ctype, methods, function);
    }
}

/* Process the specs, declarator (NULL if omitted) and width (NULL if omitted)
   of a structure component, returning a FIELD_DECL node.
   QUALS is a list of type qualifiers for this decl (such as for declaring
   const member functions).

   This is done during the parsing of the struct declaration.
   The FIELD_DECL nodes are chained together and the lot of them
   are ultimately passed to `build_struct' to make the RECORD_TYPE node.

   C++:

   If class A defines that certain functions in class B are friends, then
   the way I have set things up, it is B who is interested in permission
   granted by A.  However, it is in A's context that these declarations
   are parsed.  By returning a void_type_node, class A does not attempt
   to incorporate the declarations of the friends within its structure.

   DO NOT MAKE ANY CHANGES TO THIS CODE WITHOUT MAKING CORRESPONDING
   CHANGES TO CODE IN `start_method'.  */

tree
grokfield (declarator, declspecs, raises, init, asmspec_tree)
     tree declarator, declspecs, raises, init;
     tree asmspec_tree;
{
  register tree value = grokdeclarator (declarator, declspecs, FIELD, init != 0, raises);
  char *asmspec = 0;

  if (! value) return NULL_TREE; /* friends went bad.  */

  /* Pass friendly classes back.  */
  if (TREE_CODE (value) == VOID_TYPE)
    return void_type_node;

  if (DECL_NAME (value) != NULL_TREE
      && IDENTIFIER_POINTER (DECL_NAME (value))[0] == '_'
      && ! strcmp (IDENTIFIER_POINTER (DECL_NAME (value)), "_vptr"))
    error_with_decl (value, "member `%s' conflicts with virtual function table field name");

  /* Stash away type declarations.  */
  if (TREE_CODE (value) == TYPE_DECL)
    {
      TREE_NONLOCAL (value) = 1;
      CLASSTYPE_LOCAL_TYPEDECLS (current_class_type) = 1;
      pushdecl_class_level (value);
      return value;
    }

  if (DECL_IN_AGGR_P (value))
    {
      error_with_decl (value, "`%s' is already defined in aggregate scope");
      return void_type_node;
    }

  if (asmspec_tree)
    asmspec = TREE_STRING_POINTER (asmspec_tree);

  if (init != 0)
    {
      if (TREE_CODE (value) == FUNCTION_DECL)
	{
	  grok_function_init (value, init);
	  init = NULL_TREE;
	}
      else if (pedantic
	  && ! TREE_STATIC (value)
	  && ! TREE_READONLY (value))
	{
	  error ("fields cannot have initializers");
	  init = error_mark_node;
	}
      else
	{
	  /* We allow initializers to become parameters to base initializers.  */
	  if (TREE_CODE (init) == CONST_DECL)
	    init = DECL_INITIAL (init);
	  else if (TREE_READONLY (init) && TREE_CODE (init) == VAR_DECL)
	    init = decl_constant_value (init);
	  else if (TREE_CODE (init) == CONSTRUCTOR)
	    init = digest_init (TREE_TYPE (value), init, 0);
	  assert (TREE_PERMANENT (init));
	  if (init == error_mark_node)
	    /* We must make this look different than `error_mark_node'
	       because `decl_const_value' would mis-interpret it
	       as only meaning that this VAR_DECL is defined.  */
	    init = build1 (NOP_EXPR, TREE_TYPE (value), init);
	  else if (! TREE_LITERAL (init))
	    {
	      /* We can allow references to things that are effectively
		 static, since references are initialized with the address.  */
	      if (TREE_CODE (TREE_TYPE (value)) != REFERENCE_TYPE
		  || (TREE_EXTERNAL (init) == 0
		      && TREE_STATIC (init) == 0))
		{
		  error ("field initializer is not constant");
		  init = error_mark_node;
		}
	    }
	}
    }

  if (TREE_CODE (value) == VAR_DECL)
    {
      /* We cannot call pushdecl here, because that would
	 fill in the value of our TREE_CHAIN.  Instead, we
	 modify finish_decl to do the right thing, namely, to
	 put this decl out straight away.  */
      if (TREE_STATIC (value))
	{
	  if (asmspec == 0)
	    {
	      char *buf
		= (char *)alloca (IDENTIFIER_LENGTH (current_class_name)
				  + IDENTIFIER_LENGTH (DECL_NAME (value))
				  + sizeof (STATIC_NAME_FORMAT));
	      tree name;

	      sprintf (buf, STATIC_NAME_FORMAT,
		       IDENTIFIER_POINTER (current_class_name),
		       IDENTIFIER_POINTER (DECL_NAME (value)));
	      name = get_identifier (buf);
	      TREE_PUBLIC (value) = 1;
	      DECL_INITIAL (value) = error_mark_node;
	      asmspec = IDENTIFIER_POINTER (name);
	      DECL_ASSEMBLER_NAME (value) = asmspec;
	      asmspec_tree = build_string (IDENTIFIER_LENGTH (name), asmspec);
	    }
	  pending_statics = perm_tree_cons (NULL_TREE, value, pending_statics);
	}
      DECL_INITIAL (value) = init;
      DECL_IN_AGGR_P (value) = 1;
      finish_decl (value, init, asmspec_tree);
      pushdecl_class_level (value);
      return value;
    }
  if (TREE_CODE (value) == FIELD_DECL)
    {
      DECL_ASSEMBLER_NAME (value) = asmspec;
      if (DECL_INITIAL (value) == error_mark_node)
	init = error_mark_node;
      finish_decl (value, init, asmspec_tree);
      DECL_INITIAL (value) = init;
      DECL_IN_AGGR_P (value) = 1;
      return value;
    }
  if (TREE_CODE (value) == FUNCTION_DECL)
    {
      /* grokdeclarator defers setting this.  */
      TREE_PUBLIC (value) = 1;
      if (TREE_CHAIN (value) != NULL_TREE)
	{
	  /* Need a fresh node here so that we don't get circularity
	     when we link these together.  */
	  value = copy_node (value);
	  /* When does this happen?  */
	  assert (init == NULL_TREE);
	}
      finish_decl (value, init, asmspec_tree);

      /* Pass friends back this way.  */
      if (DECL_FRIEND_P (value))
	return void_type_node;

      DECL_IN_AGGR_P (value) = 1;
      return value;
    }
  abort ();
}

/* Like `grokfield', but for bitfields.
   WIDTH is non-NULL for bit fields only, and is an INTEGER_CST node.  */

tree
grokbitfield (declarator, declspecs, width)
     tree declarator, declspecs, width;
{
  register tree value = grokdeclarator (declarator, declspecs, FIELD, 0, NULL_TREE, NULL_TREE);

  if (! value) return NULL_TREE; /* friends went bad.  */

  /* Pass friendly classes back.  */
  if (TREE_CODE (value) == VOID_TYPE)
    return void_type_node;

  if (DECL_IN_AGGR_P (value))
    {
      error_with_decl (value, "`%s' is already defined in aggregate scope");
      return void_type_node;
    }

  if (TREE_STATIC (value))
    {
      error_with_decl (value, "static member `%s' cannot be a bitfield");
      return NULL_TREE;
    }
  if (TREE_CODE (value) == FIELD_DECL)
    {
      finish_decl (value, NULL_TREE, NULL_TREE);
      /* detect invalid field size.  */
      if (TREE_CODE (width) == CONST_DECL)
	width = DECL_INITIAL (width);
      else if (TREE_READONLY (width) && TREE_CODE (width) == VAR_DECL)
	width = decl_constant_value (width);

      if (TREE_CODE (width) != INTEGER_CST)
	{
	  error_with_decl (value, "structure field `%s' width not an integer constant");
	  DECL_INITIAL (value) = NULL;
	}
      else
	{
	  DECL_INITIAL (value) = width;
	  TREE_PACKED (value) = 1;
	}
      DECL_IN_AGGR_P (value) = 1;
      return value;
    }
  abort ();
}

/* Like GROKFIELD, except that the declarator has been
   buried in DECLSPECS.  Find the declarator, and
   return something that looks like it came from
   GROKFIELD.  */
tree
groktypefield (declspecs, parmlist)
     tree declspecs;
     tree parmlist;
{
  tree spec = declspecs;
  tree prev = NULL_TREE;

  tree type_id = NULL_TREE;
  tree quals = NULL_TREE;
  tree lengths = NULL_TREE;
  tree decl = NULL_TREE;

  while (spec)
    {
      register tree id = TREE_VALUE (spec);

      if (TREE_CODE (spec) != TREE_LIST)
	/* Certain parse errors slip through.  For example,
	   `int class ();' is not caught by the parser. Try
	   weakly to recover here.  */
	return NULL_TREE;

      if (TREE_CODE (id) == TYPE_DECL
	  || (TREE_CODE (id) == IDENTIFIER_NODE && TREE_TYPE (id)))
	{
	  /* We have a constructor/destructor or
	     conversion operator.  Use it.  */
	  if (prev)
	    TREE_CHAIN (prev) = TREE_CHAIN (spec);
	  else
	    {
	      declspecs = TREE_CHAIN (spec);
	    }
	  type_id = id;
	  goto found;
	}
      prev = spec;
      spec = TREE_CHAIN (spec);
    }

  /* Nope, we have a conversion operator to a scalar type.  */
  spec = declspecs;
  while (spec)
    {
      tree id = TREE_VALUE (spec);

      if (TREE_CODE (id) == IDENTIFIER_NODE)
	{
	  if (id == ridpointers[(int)RID_INT]
	      || id == ridpointers[(int)RID_DOUBLE]
	      || id == ridpointers[(int)RID_FLOAT])
	    {
	      if (type_id)
		error ("extra `%s' ignored",
		       IDENTIFIER_POINTER (id));
	      else
		type_id = id;
	    }
	  else if (id == ridpointers[(int)RID_LONG]
		   || id == ridpointers[(int)RID_SHORT]
		   || id == ridpointers[(int)RID_CHAR])
	    {
	      lengths = tree_cons (NULL_TREE, id, lengths);
	    }
	  else if (id == ridpointers[(int)RID_VOID])
	    {
	      if (type_id)
		error ("spurious `void' type ignored");
	      else
		error ("conversion to `void' type invalid");
	    }
	  else if (id == ridpointers[(int)RID_AUTO]
		   || id == ridpointers[(int)RID_REGISTER]
		   || id == ridpointers[(int)RID_TYPEDEF]
		   || id == ridpointers[(int)RID_CONST]
		   || id == ridpointers[(int)RID_VOLATILE])
	    {
	      error ("type specifier `%s' used invalidly",
		     IDENTIFIER_POINTER (id));
	    }
	  else if (id == ridpointers[(int)RID_FRIEND]
		   || id == ridpointers[(int)RID_VIRTUAL]
		   || id == ridpointers[(int)RID_INLINE]
		   || id == ridpointers[(int)RID_UNSIGNED]
		   || id == ridpointers[(int)RID_SIGNED]
		   || id == ridpointers[(int)RID_STATIC]
		   || id == ridpointers[(int)RID_EXTERN])
	    {
	      quals = tree_cons (NULL_TREE, id, quals);
	    }
	  else
	    {
	      /* Happens when we have a global typedef
		 and a class-local member function with
		 the same name.  */
	      type_id = id;
	      goto found;
	    }
	}
      else if (TREE_CODE (id) == RECORD_TYPE)
	error ("identifier for aggregate type conversion omitted");
      else
	assert (0);
      spec = TREE_CHAIN (spec);
    }

  if (type_id)
    {
      declspecs = chainon (lengths, quals);
    }
  else if (lengths)
    {
      if (TREE_CHAIN (lengths))
	error ("multiple length specifiers");
      type_id = ridpointers[(int)RID_INT];
      declspecs = chainon (lengths, quals);
    }
  else if (quals)
    {
      error ("no type given, defaulting to `operator int ...'");
      type_id = ridpointers[(int)RID_INT];
      declspecs = quals;
    }
  else return NULL_TREE;
 found:
  decl = grokdeclarator (build_parse_node (CALL_EXPR, type_id, parmlist, NULL_TREE),
			 declspecs, FIELD, 0, NULL_TREE);
  if (decl == NULL_TREE)
    return NULL_TREE;

  if (TREE_CODE (decl) == FUNCTION_DECL && TREE_CHAIN (decl) != NULL_TREE)
    {
      /* Need a fresh node here so that we don't get circularity
	 when we link these together.  */
      decl = copy_node (decl);
    }

  if (decl == void_type_node
      || (TREE_CODE (decl) == FUNCTION_DECL
	  && TREE_CODE (TREE_TYPE (decl)) != METHOD_TYPE))
    /* bunch of friends.  */
    return decl;

  if (DECL_IN_AGGR_P (decl))
    {
      error_with_decl (decl, "`%s' already defined in aggregate scope");
      return void_type_node;
    }

  finish_decl (decl, NULL_TREE, NULL_TREE);

  /* If this declaration is common to another declaration
     complain about such redundancy, and return NULL_TREE
     so that we don't build a circular list.  */
  if (TREE_CHAIN (decl))
    {
      error_with_decl (decl, "function `%s' declared twice in aggregate");
      return NULL_TREE;
    }
  DECL_IN_AGGR_P (decl) = 1;
  return decl;
}

/* The precedence rules of this grammar (or any other deterministic LALR
   grammar, for that matter), place the CALL_EXPR somewhere where we
   may not want it.  The solution is to grab the first CALL_EXPR we see,
   pretend that that is the one that belongs to the parameter list of
   the type conversion function, and leave everything else alone.
   We pull it out in place.

   CALL_REQUIRED is non-zero if we should complain if a CALL_EXPR
   does not appear in DECL.  */
tree
grokoptypename (decl, call_required)
     tree decl;
     int call_required;
{
  tree tmp, last;

  assert (TREE_CODE (decl) == TYPE_EXPR);

  tmp = TREE_OPERAND (decl, 0);
  last = NULL_TREE;

  while (tmp)
    {
      switch (TREE_CODE (tmp))
	{
	case CALL_EXPR:
	  {
	    tree parms = TREE_OPERAND (tmp, 1);

	    if (last)
	      TREE_OPERAND (last, 0) = TREE_OPERAND (tmp, 0);
	    else
	      TREE_OPERAND (decl, 0) = TREE_OPERAND (tmp, 0);
	    if (parms
		&& TREE_CODE (TREE_VALUE (parms)) == TREE_LIST)
	      TREE_VALUE (parms)
		= grokdeclarator (TREE_VALUE (TREE_VALUE (parms)),
				  TREE_PURPOSE (TREE_VALUE (parms)),
				  TYPENAME, 0, NULL_TREE);
	    if (parms)
	      if (parms != void_list_node)
		{
		  error ("operator <typename> requires empty parameter list");
		  TREE_OPERAND (tmp, 1) = void_list_node;
		}
	      else
		/* If user specifies `void' explicitly, avoid having
		   two voids, since we put one invisible one on.  */
		TREE_CHAIN (parms) = NULL_TREE;

	    last = grokdeclarator (TREE_OPERAND (decl, 0),
				   TREE_TYPE (decl),
				   TYPENAME, 0, NULL_TREE);
	    TREE_OPERAND (tmp, 0) = build_typename_overload (last);
	    TREE_TYPE (TREE_OPERAND (tmp, 0)) = last;
	    return tmp;
	  }

	case INDIRECT_REF:
	case ADDR_EXPR:
	case ARRAY_REF:
	  break;

	case SCOPE_REF:
	  /* This is legal when declaring a conversion to
	     something of type pointer-to-member.  */
	  if (TREE_CODE (TREE_OPERAND (tmp, 1)) == INDIRECT_REF)
	    {
	      tmp = TREE_OPERAND (tmp, 1);
	    }
	  else
	    {
#if 0
	      /* We may need to do this if grokdeclarator cannot handle this.  */
	      error ("type `member of class %s' invalid return type",
		     TYPE_NAME_STRING (TREE_OPERAND (tmp, 0)));
	      TREE_OPERAND (tmp, 1) = build_parse_node (INDIRECT_REF, TREE_OPERAND (tmp, 1));
#endif
	      tmp = TREE_OPERAND (tmp, 1);
	    }
	  break;

	default:
	  assert (0);
	}
      last = tmp;
      tmp = TREE_OPERAND (tmp, 0);
    }

  if (call_required)
    error ("operator <typename> construct requires parameter list");

  last = grokdeclarator (TREE_OPERAND (decl, 0),
			 TREE_TYPE (decl),
			 TYPENAME, 0, NULL_TREE);
  tmp = build_parse_node (CALL_EXPR, build_typename_overload (last),
			  void_list_node, NULL_TREE);
  TREE_TYPE (TREE_OPERAND (tmp, 0)) = last;
  return tmp;
}

/* Given an encoding for an operator name (see parse.y, the rules
   for making an `operator_name' in the variable DECLARATOR,
   return the name of the operator prefix as an IDENTIFIER_NODE.

   CTYPE is the class type to which this operator belongs.
   Needed in case it is a static member function.

   TYPE, if nonnull, is the function type for this declarator.
   This information helps to resolve potential ambiguities.

   If REPORT_AMBIGUOUS is non-zero, an error message
   is reported, and a default arity of the operator is
   returned.  Otherwise, return the operator under an OP_EXPR,
   for later evaluation when type information will enable
   proper instantiation.

   IS_DECL is 1 if this is a decl (as opposed to an expression).
   IS_DECL is 2 if this is a static function decl.
   Otherwise IS_DECL is 0.  */
tree
grokopexpr (declp, ctype, type, report_ambiguous, is_decl)
     tree *declp;
     tree ctype, type;
     int report_ambiguous;
{
  tree declarator = *declp;
  tree name, parmtypes;
  int seen_classtype_parm
    = (type != NULL_TREE && TREE_CODE (type) == METHOD_TYPE)
      || is_decl == 2;

  if (type != NULL_TREE)
    {
      if (ctype == 0 && TREE_CODE (TREE_OPERAND (declarator, 0)) == NEW_EXPR)
	{
	  if (TYPE_ARG_TYPES (type)
	      && TREE_CHAIN (TYPE_ARG_TYPES (type))
	      && TREE_CHAIN (TYPE_ARG_TYPES (type)) != void_list_node)
	    return get_identifier ("__user_new");
	  return get_identifier ("__builtin_new");
	}
      else if (ctype == 0 && TREE_CODE (TREE_OPERAND (declarator, 0)) == DELETE_EXPR)
	return get_identifier ("__builtin_delete");
      else
	{
	  /* Now we know the number of parameters,
	     so build the real operator fnname.  */
	  parmtypes = TYPE_ARG_TYPES (type);
	  if (is_decl > 1 && TREE_CODE (type) == METHOD_TYPE)
	    parmtypes = TREE_CHAIN (parmtypes);
	  name = build_operator_fnname (declp, parmtypes, is_decl > 1);
	  declarator = *declp;
	}
    }
  else
    {
      if (TREE_PURPOSE (declarator) == NULL_TREE)
	switch (TREE_CODE (TREE_VALUE (declarator)))
	  {
	  case PLUS_EXPR:
	  case CONVERT_EXPR:
	  case ADDR_EXPR:
	  case BIT_AND_EXPR:
	  case INDIRECT_REF:
	  case MULT_EXPR:
	  case NEGATE_EXPR:
	  case MINUS_EXPR:
	    if (report_ambiguous)
	      {
		error ("operator '%s' ambiguous, (default binary)",
		       opname_tab[(int)TREE_CODE (TREE_VALUE (declarator))]);
		name = build_operator_fnname (declp, NULL_TREE, 2);
		declarator = *declp;
	      }
	    else
	      {
		/* do something intellegent.  */
		TREE_TYPE (declarator) = unknown_type_node;
		return declarator;
	      }
	    break;
	  default:
	    name = build_operator_fnname (declp, NULL_TREE, -1);
	    declarator = *declp;
	    break;
	  }
      else if (TREE_CODE (TREE_PURPOSE (declarator)) == MODIFY_EXPR)
	{
	  name = build_operator_fnname (declp, NULL_TREE, -1);
	  declarator = *declp;
	}
      else abort ();
    }
  /* Now warn if the parameter list does not contain any args
     which are of aggregate type.  */
  if (is_decl && type != NULL_TREE && ! seen_classtype_parm)
    for (parmtypes = TYPE_ARG_TYPES (type);
	 parmtypes;
	 parmtypes = TREE_CHAIN (parmtypes))
      if (IS_AGGR_TYPE (TREE_VALUE (parmtypes))
	  || (TREE_CODE (TREE_VALUE (parmtypes)) == REFERENCE_TYPE
	      && IS_AGGR_TYPE (TREE_TYPE (TREE_VALUE (parmtypes)))))
	{
	  seen_classtype_parm = 1;
	  break;
	}

  if (is_decl && seen_classtype_parm == 0)
    if (TREE_CODE (declarator) == OP_IDENTIFIER
	&& (TREE_CODE (TREE_OPERAND (declarator, 0)) == NEW_EXPR
	    || TREE_CODE (TREE_OPERAND (declarator, 0)) == DELETE_EXPR))
      /* Global operators new and delete are not overloaded.  */
      TREE_OVERLOADED (name) = 0;
    else
      error ("operator has no %suser-defined argument type",
	     type == NULL_TREE ? "(default) " : "");

  return name;
}

/* When a function is declared with an initialializer,
   do the right thing.  Currently, there are two possibilities:

   class B
   {
    public:
     // initialization possibility #1.
     virtual void f () = 0;
     int g ();
   };
   
   class D1 : B
   {
    public:
     int d1;
     // error, no f ();
   };
   
   class D2 : B
   {
    public:
     int d2;
     void f ();
   };
   
   class D3 : B
   {
    public:
     int d3;
     // initialization possibility #2
     void f () = B::f;
   };

*/

static void
grok_function_init (decl, init)
     tree decl;
     tree init;
{
  /* An initializer for a function tells how this function should
     be inherited.  */
  tree type = TREE_TYPE (decl);
  extern tree abort_fndecl;

  if (TREE_CODE (type) == FUNCTION_TYPE)
    error_with_decl (decl, "initializer specified for non-member function `%s'");
  else if (! DECL_VIRTUAL_P (decl))
    error_with_decl (decl, "initializer specified for non-virtual method `%s'");
  else if (integer_zerop (init))
    {
      /* Mark this function as being "defined".  */
      DECL_INITIAL (decl) = error_mark_node;
      /* Give this node rtl from `abort'.  */
      DECL_RTL (decl) = DECL_RTL (abort_fndecl);
      DECL_ABSTRACT_VIRTUAL_P (decl) = 1;
    }
  else if (TREE_CODE (init) == OFFSET_REF
	   && TREE_OPERAND (init, 0) == NULL_TREE
	   && TREE_CODE (TREE_TYPE (init)) == METHOD_TYPE)
    {
      tree basetype = TYPE_METHOD_BASETYPE (TREE_TYPE (init));
      tree basefn = TREE_OPERAND (init, 1);
      if (TREE_CODE (basefn) != FUNCTION_DECL)
	error_with_decl (decl, "non-method initializer invalid for method `%s'");
      else if (DECL_OFFSET (TYPE_NAME (basefn)) != 0)
	sorry ("base member function from other than first base class");
      else
	{
	  basetype = get_base_type (basetype, TYPE_METHOD_BASETYPE (type), 1);
	  if (basetype == error_mark_node)
	    ;
	  else if (basetype == 0)
	    error_not_base_type (TYPE_METHOD_BASETYPE (TREE_TYPE (init)),
				 TYPE_METHOD_BASETYPE (type));
	  else
	    {
	      /* Mark this function as being defined,
		 and give it new rtl.  */
	      DECL_INITIAL (decl) = error_mark_node;
	      DECL_RTL (decl) = DECL_RTL (basefn);
	    }
	}
    }
  else
    error_with_decl (decl, "invalid initializer for virtual method `%s'");
}

/* Cache the value of this class's main virtual function table pointer
   in a register variable.  This will save one indirection if a
   more than one virtual function call is made this function.  */
void
setup_vtbl_ptr ()
{
  if (! flag_this_is_variable
      && optimize
      && current_class_type
      && CLASSTYPE_VSIZE (current_class_type)
      && ! DECL_STATIC_FUNCTION_P (current_function_decl))
    {
      tree vfield = build_vfield_ref (C_C_D, current_class_type);
      current_vtable_decl = CLASSTYPE_VTBL_PTR (current_class_type);
      DECL_RTL (current_vtable_decl) = 0;
      DECL_INITIAL (current_vtable_decl) = error_mark_node;
      finish_decl (current_vtable_decl, vfield, 0);
      current_vtable_decl = build_indirect_ref (current_vtable_decl, 0);
    }
  else
    current_vtable_decl = NULL_TREE;
}

/* Record the existence of an addressable inline function.  */
void
mark_inline_for_output (decl)
     tree decl;
{
  pending_addressable_inlines = perm_tree_cons (NULL_TREE, decl,
						pending_addressable_inlines);
}

void
clear_temp_name ()
{
  temp_name_counter = 0;
}

/* Hand off a unique name which can be used for variable we don't really
   want to know about anyway, for example, the anonymous variables which
   are needed to make references work.  Declare this thing so we can use it.
   The variable created will be of type TYPE.

   STATICP is nonzero if this variable should be static.  */

tree
get_temp_name (type, staticp)
     tree type;
     int staticp;
{
  char buf[sizeof (AUTO_TEMP_FORMAT) + 12];
  tree decl;
  int temp = 0;
  int toplev = global_bindings_p ();
  if (toplev || staticp)
    {
      temp = allocation_temporary_p ();
      if (temp)
	end_temporary_allocation ();
      sprintf (buf, AUTO_TEMP_FORMAT, global_temp_name_counter++);
      decl = pushdecl_top_level (build_decl (VAR_DECL, get_identifier (buf), type));
    }
  else
    {
      sprintf (buf, AUTO_TEMP_FORMAT, temp_name_counter++);
      decl = pushdecl (build_decl (VAR_DECL, get_identifier (buf), type));
    }
  TREE_USED (decl) = 1;
  TREE_STATIC (decl) = staticp;

  /* If this is a local variable, then lay out its rtl now.
     Otherwise, callers of this function are responsible for dealing
     with this variable's rtl.  */
  if (! toplev)
    {
      expand_decl (decl);
      expand_decl_init (decl);
    }
  else if (temp)
    resume_temporary_allocation ();

  return decl;
}

/* Get a variable which we can use for multiple assignments.
   It is not entered into current_binding_level, because
   that breaks things when it comes time to do final cleanups
   (which take place "outside" the binding contour of the function).

   Because it is not entered into the binding contour, `expand_end_bindings'
   does not see this variable automatically.  Users of this function
   must either pass this variable to expand_end_bindings or do
   themselves what expand_end_bindings was meant to do (like keeping
   the variable live if -noreg was specified).  */
tree
get_temp_regvar (type, init)
     tree type, init;
{
  static char buf[sizeof (AUTO_TEMP_FORMAT) + 8] = { '_' };
  tree decl;

  sprintf (buf+1, AUTO_TEMP_FORMAT, temp_name_counter++);
  decl = pushdecl (build_decl (VAR_DECL, get_identifier (buf), type));
  TREE_USED (decl) = 1;
  TREE_REGDECL (decl) = 1;

  if (init)
    store_init_value (decl, init);

  /* We can expand these without fear, since they cannot need
     constructors or destructors.  */
  expand_decl (decl);
  expand_decl_init (decl);
  return decl;
}

/* Make the macro TEMP_NAME_P available to units which do not
   include c-tree.h.  */
int
temp_name_p (decl)
     tree decl;
{
  return TEMP_NAME_P (decl);
}

/* Finish off the processing of a UNION_TYPE structure.
   If there are static members, then all members are
   static, and must be laid out together.  If the
   union is an anonymous union, we arrage for that
   as well.  PUBLICP is nonzero if this union is
   not declared static.  */
void
finish_anon_union (anon_union_decl)
     tree anon_union_decl;
{
  tree type = TREE_TYPE (anon_union_decl);
  tree field, decl;
  tree elems = NULL_TREE;
  int public_p = TREE_PUBLIC (anon_union_decl);
  int static_p = TREE_STATIC (anon_union_decl);
  int external_p = TREE_EXTERNAL (anon_union_decl);

  if ((field = TYPE_FIELDS (type)) == NULL_TREE)
    return;

  if (public_p && (static_p || external_p))
    error ("optimizer cannot handle global anonymous unions");

  while (field)
    {
      decl = build_decl (VAR_DECL, DECL_NAME (field), TREE_TYPE (field));
      /* tell `pushdecl' that this is not tentative.  */
      DECL_INITIAL (decl) = error_mark_node;
      TREE_PUBLIC (decl) = public_p;
      TREE_STATIC (decl) = static_p;
      TREE_EXTERNAL (decl) = external_p;
      decl = pushdecl (decl);
      DECL_INITIAL (decl) = NULL_TREE;
      elems = tree_cons (DECL_ASSEMBLER_NAME (field), decl, elems);
      TREE_TYPE (elems) = type;
      field = TREE_CHAIN (field);
    }
  if (static_p)
    make_decl_rtl (decl, 0, global_bindings_p ());
  expand_anon_union_decl (decl, NULL_TREE, elems);
}

/* Finish and output a table which is generated by the compiler.
   NAME is the name to give the table.
   TYPE is the type of the table entry.
   INIT is all the elements in the table.
   PUBLICP is non-zero if this table should be given external visibility.  */
tree
finish_table (name, type, init, publicp)
     tree name, type, init;
{
  tree itype, atype, decl;

  itype = build_index_type (build_int_2 (list_length (init), 0));
  atype = build_cplus_array_type (type, itype);
  layout_type (atype);
  decl = build_decl (VAR_DECL, name, atype);
  decl = pushdecl (decl);
  TREE_STATIC (decl) = 1;
  TREE_PUBLIC (decl) = publicp;
  init = build (CONSTRUCTOR, atype, NULL_TREE, init);
  TREE_LITERAL (init) = 1;
  TREE_STATIC (init) = 1;
  DECL_INITIAL (decl) = init;
  finish_decl (decl, init,
	       build_string (IDENTIFIER_LENGTH (DECL_NAME (decl)),
			     IDENTIFIER_POINTER (DECL_NAME (decl))));
  return decl;
}

/* Auxilliary functions to make type signatures for
   `operator new' and `operator delete' correspond to
   what compiler will be expecting.  */

extern tree sizetype;

tree
coerce_new_type (ctype, type)
     tree ctype;
     tree type;
{
  int e1 = 0, e2 = 0;

  if (TREE_CODE (type) == METHOD_TYPE)
    type = build_function_type (TREE_TYPE (type), TREE_CHAIN (TYPE_ARG_TYPES (type)));
  if (TREE_TYPE (type) != ptr_type_node)
    e1 = 1, error ("`operator new' must return type `void *'");

  /* Technically the type must be `size_t', but we may not know
     what that is.  */
  if (TYPE_ARG_TYPES (type) == NULL_TREE)
    e1 = 1, error ("`operator new' takes type `size_t' parameter");
  else if (TREE_CODE (TREE_VALUE (TYPE_ARG_TYPES (type))) != INTEGER_TYPE
	   || TYPE_PRECISION (TREE_VALUE (TYPE_ARG_TYPES (type))) != TYPE_PRECISION (sizetype))
    e2 = 1, error ("`operator new' takes type `size_t' as first parameter");
  if (e2)
    type = build_function_type (ptr_type_node, tree_cons (NULL_TREE, sizetype, TREE_CHAIN (TYPE_ARG_TYPES (type))));
  else if (e1)
    type = build_function_type (ptr_type_node, TYPE_ARG_TYPES (type));
  return type;
}

tree
coerce_delete_type (ctype, type)
     tree ctype;
     tree type;
{
  int e1 = 0, e2 = 0, e3 = 0;

  if (TREE_CODE (type) == METHOD_TYPE)
    type = build_function_type (TREE_TYPE (type), TREE_CHAIN (TYPE_ARG_TYPES (type)));
  if (TREE_TYPE (type) != void_type_node)
    e1 = 1, error ("`operator delete' must return type `void'");
  if (TYPE_ARG_TYPES (type) == NULL_TREE
      || TREE_VALUE (TYPE_ARG_TYPES (type)) != ptr_type_node)
    e2 = 1, error ("`operator delete' takes type `void *' as first parameter");

  if (TYPE_ARG_TYPES (type)
      && TREE_CHAIN (TYPE_ARG_TYPES (type))
      && TREE_CHAIN (TYPE_ARG_TYPES (type)) != void_list_node)
    {
      /* Again, technically this argument must be `size_t', but again
	 we may not know what that is.  */
      tree t2 = TREE_VALUE (TREE_CHAIN (TYPE_ARG_TYPES (type)));
      if (TREE_CODE (t2) != INTEGER_TYPE
	  || TYPE_PRECISION (t2) != TYPE_PRECISION (sizetype))
	e3 = 1, error ("second argument to `operator delete' must be of type `size_t'");
      else if (TREE_CHAIN (TREE_CHAIN (TYPE_ARG_TYPES (type))) != void_list_node)
	{
	  e3 = 1;
	  if (TREE_CHAIN (TREE_CHAIN (TYPE_ARG_TYPES (type))))
	    error ("too many arguments in declaration of `operator delete'");
	  else
	    error ("`...' invalid in specification of `operator delete'");
	}
    }
  if (e3)
    type = build_function_type (void_type_node, tree_cons (NULL_TREE, ptr_type_node, build_tree_list (NULL_TREE, sizetype)));
  else if (e2)
    type = build_function_type (void_type_node, tree_cons (NULL_TREE, ptr_type_node, TREE_CHAIN (TYPE_ARG_TYPES (type))));
  else if (e1)
    type = build_function_type (void_type_node, TYPE_ARG_TYPES (type));
  return type;
}

extern int parse_time, varconst_time;

#define TIMEVAR(VAR, BODY)    \
do { int otime = gettime (); BODY; VAR += gettime () - otime; } while (0)

/* This routine is called from the last rule in yyparse ().
   Its job is to create all the code needed to initialize and
   destroy the global aggregates.  We do the destruction
   first, since that way we only need to reverse the decls once.  */

void
finish_file ()
{
  extern struct rtx_def *const0_rtx;
  extern int lineno;
  extern struct _iob *asm_out_file;
  int start_time, this_time;
  char *init_function_name;

  char *buf;
  char *p;
  tree fnname;
  tree vars = static_aggregates;
  int needs_cleaning = 0, needs_messing_up = 0;

  if (main_input_filename == 0)
    main_input_filename = input_filename;
  if (!first_global_object_name)
    first_global_object_name = main_input_filename;

  buf = (char *) alloca (sizeof (FILE_FUNCTION_FORMAT)
			 + strlen (first_global_object_name));

#ifndef MERGED
  if (flag_detailed_statistics)
    dump_tree_statistics ();
#endif

  /* Bad parse errors.  Just forget about it.  */
  if (! global_bindings_p ())
    return;

#ifndef MERGED
  /* This is the first run of an unexec'd program, so save this till 
     we come back again. -- bryan@kewill.uucp */
  {
    extern int just_done_unexec;
    if (just_done_unexec)
      return;
  }
#endif

  start_time = gettime ();

  /* Push into C language context, because that's all
     we'll need here.  */
  push_lang_context (lang_name_c);

  /* Set up the name of the file-level functions we may need.  */
  /* Use a global object (which is already required to be unique over
     the program) rather than the file name (which imposes extra
     constraints).  -- Raeburn@MIT.EDU, 10 Jan 1990.  */
  sprintf (buf, FILE_FUNCTION_FORMAT, first_global_object_name);

  /* Don't need to pull wierd characters out of global names.  */
  if (first_global_object_name == main_input_filename)
    {
      for (p = buf+11; *p; p++)
	if (! ((*p >= '0' && *p <= '9')
#ifndef ASM_IDENTIFY_GCC	/* this is required if `.' is invalid -- k. raeburn */
	       || *p == '.'
#endif
#ifndef NO_DOLLAR_IN_LABEL	/* this for `$'; unlikely, but... -- kr */
	       || *p == '$'
#endif
	       || (*p >= 'A' && *p <= 'Z')
	       || (*p >= 'a' && *p <= 'z')))
	  *p = '_';
    }

  /* See if we really need the hassle.  */
  while (vars && needs_cleaning == 0)
    {
      tree decl = TREE_VALUE (vars);
      tree type = TREE_TYPE (decl);
      if (TYPE_NEEDS_DESTRUCTOR (type))
	{
	  needs_cleaning = 1;
	  needs_messing_up = 1;
	  break;
	}
      else
	needs_messing_up |= TYPE_NEEDS_CONSTRUCTING (type);
      vars = TREE_CHAIN (vars);
    }
  if (needs_cleaning == 0)
    goto mess_up;

  /* Otherwise, GDB can get confused, because in only knows
     about source for LINENO-1 lines.  */
  lineno -= 1;

#if defined(sun)
  /* Point Sun linker at this function.  */
  fprintf (asm_out_file, ".stabs \"_fini\",10,0,0,0\n.stabs \"");
  assemble_name (asm_out_file, buf);
  fprintf (asm_out_file, "\",4,0,0,0\n");
#endif

  fnname = get_identifier (buf);
  start_function (void_list_node, build_parse_node (CALL_EXPR, fnname, void_list_node, NULL_TREE), 0, 0);
  DECL_PRINT_NAME (current_function_decl) = "file cleanup";
  fnname = DECL_NAME (current_function_decl);
  store_parm_decls ();

  pushlevel (0);
  clear_last_expr ();
  push_momentary ();
  expand_start_bindings (0);

  /* These must be done in backward order to destroy,
     in which they happen to be!  */
  while (vars)
    {
      tree decl = TREE_VALUE (vars);
      tree type = TREE_TYPE (decl);
      tree temp = TREE_PURPOSE (vars);

      if (TYPE_NEEDS_DESTRUCTOR (type))
	{
	  if (TREE_STATIC (vars))
	    expand_start_cond (build_binary_op (NE_EXPR, temp, integer_zero_node), 0);
	  if (TREE_CODE (type) == ARRAY_TYPE)
	    temp = decl;
	  else
	    {
	      mark_addressable (decl);
	      temp = build (ADDR_EXPR, TYPE_POINTER_TO (type), decl);
	    }
	  temp = build_delete (TREE_TYPE (temp), temp,
			       integer_zero_node, LOOKUP_NORMAL, 0);
	  expand_expr_stmt (temp);

	  if (TYPE_LANG_SPECIFIC (type)
	      && TYPE_USES_VIRTUAL_BASECLASSES (type))
	    expand_expr_stmt (build_vbase_delete (type, decl));

	  if (TREE_STATIC (vars))
	    expand_end_cond ();
	}
      vars = TREE_CHAIN (vars);
    }

  expand_end_bindings (getdecls (), 1, 0);
  poplevel (1, 0, 1);
  pop_momentary ();

  finish_function (lineno, 0);

#if defined(DBX_DEBUGGING_INFO) && !defined(FASCIST_ASSEMBLER)
  /* Now tell GNU LD that this is part of the static destructor set.  */
  {
    extern struct _iob *asm_out_file;
    fprintf (asm_out_file, ".stabs \"___DTOR_LIST__\",22,0,0,");
    assemble_name (asm_out_file, IDENTIFIER_POINTER (fnname));
    fputc ('\n', asm_out_file);
  }
#endif

  /* if it needed cleaning, then it will need messing up: drop through  */

 mess_up:
  /* Must do this while we think we are at the top level.  */
  vars = nreverse (static_aggregates);
  if (vars != NULL_TREE)
    {
      buf[FILE_FUNCTION_PREFIX_LEN] = 'I';

#if defined(sun)
      /* Point Sun linker at this function.  */
      fprintf (asm_out_file, ".stabs \"_init\",10,0,0,0\n.stabs \"");
      assemble_name (asm_out_file, buf);
      fprintf (asm_out_file, "\",4,0,0,0\n");
#endif

      fnname = get_identifier (buf);
      start_function (void_list_node, build_parse_node (CALL_EXPR, fnname, void_list_node, NULL_TREE), 0, 0);
      DECL_PRINT_NAME (current_function_decl) = "file initialization";
      fnname = DECL_NAME (current_function_decl);
      store_parm_decls ();

      pushlevel (0);
      clear_last_expr ();
      push_momentary ();
      expand_start_bindings (0);

#ifdef SOS
      if (flag_all_virtual == 2)
	{
	  tree decl;
	  char c = buf[FILE_FUNCTION_PREFIX_LEN];
	  buf[FILE_FUNCTION_PREFIX_LEN] = 'Z';

	  decl = pushdecl (build_lang_decl (FUNCTION_DECL, get_identifier (buf), default_function_type));
	  finish_decl (decl, NULL_TREE, NULL_TREE);
	  expand_expr_stmt (build_function_call (decl, NULL_TREE));
	  buf[FILE_FUNCTION_PREFIX_LEN] = c;
	}
#endif

      while (vars)
	{
	  tree decl = TREE_VALUE (vars);
	  tree init = TREE_PURPOSE (vars);

	  /* If this was a static attribute within some function's scope,
	     then don't initialize it here.  Also, don't bother
	     with initializers that contain errors.  */
	  if (TREE_STATIC (vars)
	      || (init && TREE_CODE (init) == TREE_LIST
		  && value_member (error_mark_node, init)))
	    {
	      vars = TREE_CHAIN (vars);
	      continue;
	    }

	  if (TREE_CODE (decl) == VAR_DECL)
	    {
	      /* Set these global variables so that GDB at least puts
		 us near the declaration which required the initialization.  */
	      input_filename = DECL_SOURCE_FILE (decl);
	      lineno = DECL_SOURCE_LINE (decl);
	      emit_note (input_filename, lineno);

	      if (init)
		{
		  if (TREE_CODE (init) == VAR_DECL)
		    {
		      /* This behavior results when there are
			 multiple declarations of an aggregate,
			 the last of which defines it.  */
		      if (DECL_RTL (init) == DECL_RTL (decl))
			{
			  assert (DECL_INITIAL (decl) == error_mark_node
				  || (TREE_CODE (DECL_INITIAL (decl)) == CONSTRUCTOR
				      && CONSTRUCTOR_ELTS (DECL_INITIAL (decl)) == NULL_TREE));
			  init = DECL_INITIAL (init);
			  if (TREE_CODE (init) == CONSTRUCTOR
			      && CONSTRUCTOR_ELTS (init) == NULL_TREE)
			    init = NULL_TREE;
			}
#if 0
		      else if (TREE_TYPE (decl) == TREE_TYPE (init))
			{
#if 1
			  assert (0);
#else
			  /* point to real decl's rtl anyway.  */
			  DECL_RTL (init) = DECL_RTL (decl);
			  assert (DECL_INITIAL (decl) == error_mark_node);
			  init = DECL_INITIAL (init);
#endif				/* 1 */
			}
#endif				/* 0 */
		    }
		}
	      if (IS_AGGR_TYPE (TREE_TYPE (decl))
		  || init == 0
		  || TREE_CODE (TREE_TYPE (decl)) == ARRAY_TYPE)
		expand_aggr_init (decl, init, 0);
	      else if (TREE_CODE (init) == TREE_VEC)
		expand_expr (expand_vec_init (decl, TREE_VEC_ELT (init, 0),
					      TREE_VEC_ELT (init, 1),
					      TREE_VEC_ELT (init, 2), 0),
			     const0_rtx, VOIDmode, 0);
	      else
		expand_assignment (decl, init, 0, 0);
	    }
	  else if (TREE_CODE (decl) == SAVE_EXPR)
	    {
	      if (! PARM_DECL_EXPR (decl))
		{
		  /* a `new' expression at top level.  */
		  expand_expr (decl, const0_rtx, VOIDmode, 0);
		  expand_aggr_init (build_indirect_ref (decl, 0), init, 0);
		}
	    }
	  else if (decl == error_mark_node)
	    ;
	  else abort ();
	  vars = TREE_CHAIN (vars);
	}

      expand_end_bindings (getdecls (), 1, 0);
      poplevel (1, 0, 1);
      pop_momentary ();

      finish_function (lineno, 0);
#if defined(DBX_DEBUGGING_INFO) && !defined(FASCIST_ASSEMBLER)
      /* Now tell GNU LD that this is part of the static constructor set.  */
      {
	extern struct _iob *asm_out_file;
	fprintf (asm_out_file, ".stabs \"___CTOR_LIST__\",22,0,0,");
	assemble_name (asm_out_file, IDENTIFIER_POINTER (fnname));
	fputc ('\n', asm_out_file);
      }
#endif
    }

#ifdef SOS
  if (flag_all_virtual == 2)
    {
      tree __sosDynError = default_conversion (lookup_name (get_identifier ("sosDynError")));

      tree null_string = build1 (ADDR_EXPR, string_type_node, combine_strings (build_string (0, "")));
      tree tags = gettags ();
      tree decls = getdecls ();
      tree entry;
      int i;

      entry = NULL_TREE;
      for (i = 0; i < 3; i++)
	entry = tree_cons (NULL_TREE, integer_zero_node, entry);
      zlink = build_tree_list (NULL_TREE, build (CONSTRUCTOR, zlink_type, NULL_TREE, entry));
      TREE_LITERAL (TREE_VALUE (zlink)) = 1;
      TREE_STATIC (TREE_VALUE (zlink)) = 1;

      entry = NULL_TREE;
      for (i = 0; i < 5; i++)
	entry = tree_cons (NULL_TREE, integer_zero_node, entry);
      zret = build_tree_list (NULL_TREE, build (CONSTRUCTOR, zret_type, NULL_TREE, entry));
      TREE_LITERAL (TREE_VALUE (zret)) = 1;
      TREE_STATIC (TREE_VALUE (zret)) = 1;

      /* Symbols with external visibility (except globally visible
	 dynamic member functions) into the `zlink' table.  */
      while (decls)
	{
	  if (TREE_PUBLIC (decls)
	      && TREE_ASM_WRITTEN (decls)
	      && (TREE_CODE (decls) != FUNCTION_DECL
		  || TREE_CODE (TREE_TYPE (decls)) != METHOD_TYPE
		  || TYPE_DYNAMIC (TYPE_METHOD_BASETYPE (TREE_TYPE (decls))) == 0))
	    {
	      entry = build (CONSTRUCTOR, zlink_type, NULL_TREE,
			     tree_cons (NULL_TREE,
					build1 (ADDR_EXPR, string_type_node,
						combine_strings (build_string (IDENTIFIER_LENGTH (DECL_NAME (decls)),
									       IDENTIFIER_POINTER (DECL_NAME (decls))))),
					tree_cons (NULL_TREE, integer_one_node,
						   build_tree_list (NULL_TREE, build_unary_op (ADDR_EXPR, decls, 0)))));
	      TREE_LITERAL (entry) = 1;
	      TREE_STATIC (entry) = 1;
	      zlink = tree_cons (NULL_TREE, entry, zlink);
	    }
	  decls = TREE_CHAIN (decls);
	}

      buf[FILE_FUNCTION_PREFIX_LEN] = 'Z';
  
      fnname = get_identifier (buf);
      start_function (void_list_node, build_parse_node (CALL_EXPR, fnname, void_list_node, NULL_TREE), 0, 0);
      fnname = DECL_NAME (current_function_decl);
      store_parm_decls ();

      pushlevel (0);
      clear_last_expr ();
      push_momentary ();
      expand_start_bindings (0);

      {
	tree decl, type;
	/* Lay out a table static to this function
	   with information about all text and data that
	   this file provides.  */
	tree zlink_table = finish_table (get_identifier ("__ZLINK_tbl"), zlink_type, zlink, 0);
	decl = pushdecl (build_decl (VAR_DECL, get_identifier ("_ZLINK_once"), integer_type_node));
	TREE_STATIC (decl) = 1;
	finish_decl (decl, NULL_TREE, NULL_TREE);
	expand_start_cond (truthvalue_conversion (decl), 0);
	expand_null_return ();
	expand_end_cond ();
	finish_stmt ();
	expand_expr_stmt (build_modify_expr (decl, NOP_EXPR, integer_one_node));
	type = build_function_type (void_type_node, NULL_TREE);
	decl = pushdecl (build_lang_decl (FUNCTION_DECL, get_identifier ("_Ztable_from_cfront"), type));
	TREE_EXTERNAL (decl) = 1;
	finish_decl (decl, NULL_TREE, NULL_TREE);
	expand_expr_stmt (build_function_call (decl, build_tree_list (NULL_TREE, zlink_table)));
	expand_null_return ();
      }
      expand_end_bindings (0, 1, 0);
      poplevel (1, 0, 1);
      pop_momentary ();
      finish_function (lineno, 0);

      buf[FILE_FUNCTION_PREFIX_LEN] = 'Y';
      fnname = get_identifier (buf);
      start_function (build_tree_list (NULL_TREE, get_identifier ("int")),
		      build_parse_node (INDIRECT_REF, build_parse_node (CALL_EXPR, fnname, void_list_node, NULL_TREE)), 0, 0);
      fnname = DECL_NAME (current_function_decl);
      store_parm_decls ();

      pushlevel (0);
      clear_last_expr ();
      push_momentary ();
      expand_start_bindings (0);

      {
#define SOS_VERSION 2
	tree sosVersionNumber = build_int_2 (SOS_VERSION, 0);
	tree zret_table;

	/* For each type defined, if is a dynamic type, write out
	   an entry linking its member functions with their names.  */

	while (tags)
	  {
	    tree type = TREE_VALUE (tags);
	    if (TYPE_DYNAMIC (type))
	      {
		/* SOS currently only implements single inheritance,
		   so we just pick up one string, if this class
		   has a base class.  */
		tree base_name = CLASSTYPE_N_BASECLASSES (type) > 0 && TYPE_DYNAMIC (CLASSTYPE_BASECLASS (type, 1))
		  ? build1 (ADDR_EXPR, string_type_node, CLASSTYPE_TYPENAME_AS_STRING (CLASSTYPE_BASECLASS (type, 1)))
		    : null_string;
		tree dyn_table, dyn_entry = NULL_TREE, fns = CLASS_ASSOC_VIRTUALS (type);
		while (fns)
		  {
		    if (TREE_ASM_WRITTEN (TREE_OPERAND (TREE_VALUE (fns), 0)))
		      dyn_entry = tree_cons (NULL_TREE, TREE_VALUE (fns), dyn_entry);
		    else
		      dyn_entry = tree_cons (NULL_TREE, __sosDynError, dyn_entry);
		    fns = TREE_CHAIN (fns);
		  }
		dyn_entry = nreverse (dyn_entry);
		dyn_entry = tree_cons (NULL_TREE, build1 (NOP_EXPR, TYPE_POINTER_TO (default_function_type), sosVersionNumber),
				       tree_cons (NULL_TREE, integer_zero_node,
						  tree_cons (NULL_TREE, integer_zero_node,
							     dyn_entry)));
		dyn_table = finish_table (DECL_NAME (TYPE_NAME (type)), TYPE_POINTER_TO (default_function_type), dyn_entry, 0);
		entry = build (CONSTRUCTOR, zret_type, NULL_TREE,
			       tree_cons (NULL_TREE, build1 (ADDR_EXPR, string_type_node, CLASSTYPE_TYPENAME_AS_STRING (type)),
					  tree_cons (NULL_TREE, default_conversion (dyn_table),
						     tree_cons (NULL_TREE, build_int_2 (CLASSTYPE_VSIZE (type), 0),
								tree_cons (NULL_TREE, base_name,
									   build_tree_list (NULL_TREE, integer_zero_node))))));
		TREE_LITERAL (entry) = 1;
		TREE_STATIC (entry) = 1;
		zret = tree_cons (NULL_TREE, entry, zret);
	      }
	    tags = TREE_CHAIN (tags);
	  }
	zret_table = finish_table (get_identifier ("__Zret"), zret_type, zret, 0);
	c_expand_return (convert (build_pointer_type (integer_type_node), default_conversion (zret_table)));
      }
      expand_end_bindings (0, 1, 0);
      poplevel (1, 0, 1);
      pop_momentary ();
      finish_function (lineno, 0);

#if defined(DBX_DEBUGGING_INFO) && !defined(FASCIST_ASSEMBLER)
      {
	extern struct _iob *asm_out_file;
	fprintf (asm_out_file, ".stabs \"___ZTOR_LIST__\",22,0,0,");
	assemble_name (asm_out_file, IDENTIFIER_POINTER (fnname));
	fputc ('\n', asm_out_file);
      }
#endif
    }
#endif

  /* Done with C language context needs.  */
  pop_lang_context ();

  /* Now write out any static class variables (which may have since
     learned how to be initialized).  */
  while (pending_statics)
    {
      tree decl = TREE_VALUE (pending_statics);
      if (TREE_USED (decl) == 1
	  || TREE_READONLY (decl) == 0
	  || DECL_INITIAL (decl) == 0)
	rest_of_decl_compilation (decl, DECL_ASSEMBLER_NAME (decl), 1, 1);
      pending_statics = TREE_CHAIN (pending_statics);
    }

  this_time = gettime ();
  parse_time -= this_time - start_time;
  varconst_time += this_time - start_time;

  /* Now write out inline functions which had their addresses taken
     and which were not declared virtual and which were not declared
     `extern inline'.  */
  while (pending_addressable_inlines)
    {
      tree decl = TREE_VALUE (pending_addressable_inlines);
      if (! TREE_ASM_WRITTEN (decl)
	  && ! TREE_EXTERNAL (decl)
	  && DECL_SAVED_INSNS (decl))
	output_inline_function (decl);
      pending_addressable_inlines = TREE_CHAIN (pending_addressable_inlines);
    }

  start_time = gettime ();

  /* Now delete from the chain of variables all virtual function tables.
     We output them all ourselves, because each will be treated specially.  */
  {
    tree prev;
    /* Make last thing in global scope not be a virtual function table.  */
    prev = build_decl (VAR_DECL, get_identifier (" @%$#@!"), integer_type_node);
    TREE_EXTERNAL (prev) = 1;
    pushdecl (prev);
    /* Make this dummy variable look used.  */
    TREE_ASM_WRITTEN (prev) = 1;
    DECL_INITIAL (prev) = error_mark_node;

    for (prev = 0, vars = getdecls ();
	 vars;
	 vars = TREE_CHAIN (vars))
      {
	if (TREE_CODE (vars) == TYPE_DECL
	    && TYPE_LANG_SPECIFIC (TREE_TYPE (vars))
	    && CLASSTYPE_VSIZE (TREE_TYPE (vars)))
	  {
	    tree decl = CLASS_ASSOC_VTABLE (TREE_TYPE (vars));

	    /* If we are controlled by `+e2', obey.  */
	    if (write_virtuals == 2)
	      {
		tree assoc = value_member (DECL_NAME (vars), pending_vtables);
		if (assoc)
		  TREE_PURPOSE (assoc) = void_type_node;
		else
		  decl = NULL_TREE;
	      }
	    /* If this type has inline virtual functions, then
	       write those functions out now.  */
	    if (decl && ! TREE_EXTERNAL (decl) && TREE_USED (decl))
	      {
		tree entries;

		for (entries = TREE_CHAIN (CONSTRUCTOR_ELTS (DECL_INITIAL (decl)));
		     entries; entries = TREE_CHAIN (entries))
		  {
		    tree fnaddr = FNADDR_FROM_VTABLE_ENTRY (TREE_VALUE (entries));
		    tree fn = TREE_OPERAND (fnaddr, 0);
		    if (! TREE_ASM_WRITTEN (fn)
			&& DECL_PENDING_INLINE_INFO (fn)
			&& TREE_ADDRESSABLE (fn))
		      output_inline_function (fn);
		  }
	      }
	  }
	if (TREE_CODE (vars) == VAR_DECL && DECL_VIRTUAL_P (vars))
	  {
	    if ((write_virtuals == 2
		 && value_member (DECL_NAME (TYPE_NAME (DECL_VPARENT (vars))),
				  pending_vtables))
		|| write_virtuals == 1
		|| (write_virtuals == 0 && TREE_USED (vars)))
	      {
		extern tree the_null_vtable_entry;

		/* Stuff this virtual function table's size into
		   `pfn' slot of `the_null_vtable_entry'.  */
		tree nelts = array_type_nelts (TREE_TYPE (vars));
		tree *ppfn = &FNADDR_FROM_VTABLE_ENTRY (the_null_vtable_entry);
		*ppfn = nelts;
		assert (TREE_VALUE (TREE_CHAIN (TREE_CHAIN (CONSTRUCTOR_ELTS (TREE_VALUE (CONSTRUCTOR_ELTS (DECL_INITIAL (vars))))))) == nelts);
		/* Write it out.  */
		rest_of_decl_compilation (vars, 0, 1, 1);
	      }
	    /* We know that PREV must be non-zero here.  */
	    TREE_CHAIN (prev) = TREE_CHAIN (vars);
	  }
	else
	  prev = vars;
      }
    if (write_virtuals == 2)
      {
	/* Now complain about an virtual function tables promised
	   but not delivered.  */
	while (pending_vtables)
	  {
	    if (TREE_PURPOSE (pending_vtables) == NULL_TREE)
	      error ("virtual function table for `%s' not defined",
		     IDENTIFIER_POINTER (TREE_VALUE (pending_vtables)));
	    pending_vtables = TREE_CHAIN (pending_vtables);
	  }
      }
  }
  permanent_allocation ();
  this_time = gettime ();
  parse_time -= this_time - start_time;
  varconst_time += this_time - start_time;

  if (flag_detailed_statistics)
    dump_time_statistics ();
}
