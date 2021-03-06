/* Convert language-specific tree expression to rtl instructions,
   for GNU compiler.  Copyright (C) 1988 Free Software Foundation, Inc.

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


#include "config.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "expr.h"
#include "cplus-tree.h"

rtx
cplus_expand_expr (exp, target, tmode, modifier)
     tree exp;
     rtx target;
     enum machine_mode tmode;
     enum expand_modifier modifier;
{
  register rtx op0, op1, temp;
  tree type = TREE_TYPE (exp);
  register enum machine_mode mode = TYPE_MODE (type);
  register enum tree_code code = TREE_CODE (exp);
  rtx original_target = target;
  int ignore = target == const0_rtx;

  if (ignore) target = 0, original_target = 0;

  /* No sense saving up arithmetic to be done
     if it's all in the wrong mode to form part of an address.
     And force_operand won't know whether to sign-extend or zero-extend.  */

  if (mode != Pmode && modifier == EXPAND_SUM)
    modifier = EXPAND_NORMAL;

  switch (code)
    {
    case CPLUS_NEW_EXPR:
      {
	/* Something needs to be initialized, but we didn't know
	   where that thing was when building the tree.  For example,
	   it could be the return value of a function, or a parameter
	   to a function which lays down in the stack, or a temporary
	   variable which must be passed by reference .

	   Cleanups are handled in a language-specific way: they
	   might be run by the called function (true in GNU C++
	   for parameters with cleanups), or they might be
	   run by the caller, after the call (true in GNU C++
	   for other cleanup needs).  */

	tree func = TREE_OPERAND (exp, 0);
	tree args = TREE_OPERAND (exp, 1);
	tree type = TREE_TYPE (exp), slot;
	tree fn_type = TREE_TYPE (TREE_TYPE (func));
	tree return_type = TREE_TYPE (fn_type);
	rtx call_target;

	/* The expression `init' wants to initialize what
	   `target' represents.  SLOT holds the slot for TARGET.  */
	slot = TREE_OPERAND (exp, 2);

	/* Should always be called with a target.  */
	if (target == 0 || DECL_RTL (slot) != target)
	  abort ();

	/* The target the initializer will initialize (CALL_TARGET)
	   must now be directed to initialize the target we are
	   supposed to initialize (TARGET).  The semantics for
	   choosing what CALL_TARGET is is language-specific,
	   as is building the call which will perform the
	   initialization.  It is left here to show the choices that
	   exist for C++.  */
	   
	if (TREE_CODE (func) == ADDR_EXPR
	    && TREE_CODE (TREE_OPERAND (func, 0)) == FUNCTION_DECL
	    && DECL_CONSTRUCTOR_P (TREE_OPERAND (func, 0)))
	  {
	    type = TYPE_POINTER_TO (type);
	    TREE_VALUE (args) = build (ADDR_EXPR, type, slot);
	    call_target = 0;
	  }
	else if (TREE_CODE (return_type) == REFERENCE_TYPE)
	  {
	    type = return_type;
	    call_target = 0;
	  }
	else
	  {
	    call_target = target;
	  }
	call_target = expand_expr (build (CALL_EXPR, type, func, args, 0), call_target, 0, 0);

	if (TREE_CODE (return_type) == REFERENCE_TYPE)
	  {
	    tree init;

	    if (GET_CODE (call_target) == REG
		&& REGNO (call_target) < FIRST_PSEUDO_REGISTER)
	      abort ();

	    type = TREE_TYPE (exp);

	    init = build (RTL_EXPR, return_type, 0, call_target);
	    /* We got back a reference to the type we want.  Now intialize
	       target with that.  */
	    expand_aggr_init (slot, init, 0);
	  }

	return target;
      }

    default:
      break;
    }
  abort ();
}
