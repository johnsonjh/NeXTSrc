/* Subroutines for insn-output.c for Pyramid 90 Series.
   Copyright (C) 1989 Free Software Foundation, Inc.

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

/* Some output-actions in pyr.md need these.  */
#include <stdio.h>
extern FILE *asm_out_file;
#include "tree.h"

/*
 * Do FUNCTION_ARG.
 * This cannot be defined as a macro on pyramids, because Pyramid Technology's
 * C compiler dies on (several equivalent definitions of) this macro.
 * The only way around this cc bug was to make this a function.
 * While it would be possible to use a macro version for gcc, it seems
 * more reliable to have a single version of the code.
 */
void *
pyr_function_arg(cum, mode, type, named)
  CUMULATIVE_ARGS cum;
  enum machine_mode mode;
  tree type;
{
  return (void *)(FUNCTION_ARG_HELPER (cum, mode,type,named));
}

/* Do the hard part of PARAM_SAFE_FOR_REG_P.
 * This cannot be defined as a macro on pyramids, because Pyramid Technology's
 * C compiler dies on (several equivalent definitions of) this macro.
 * The only way around this cc bug was to make this a function.
 */
int
inner_param_safe_helper (type)
    tree type;
{
  return (INNER_PARAM_SAFE_HELPER(type));
}


/* Return 1 if OP is a non-indexed operand of mode MODE.
   This is either a register reference, a memory reference,
   or a constant.  In the case of a memory reference, the address
   is checked to make sure it isn't indexed.

   Register and memory references must have mode MODE in order to be valid,
   but some constants have no machine mode and are valid for any mode.

   If MODE is VOIDmode, OP is checked for validity for whatever mode
   it has.

   The main use of this function is as a predicate in match_operand
   expressions in the machine description.

   It is  useful to compare this with general_operand().  They should
   be identical except for one line.

   This function seems necessary because of the non-orthogonality of
   Pyramid insns.
   For any 2-operand insn, and any combination of operand modes,
   if indexing is valid for the isn's second operand, it is invalid
   for the first operand to be indexed. */

extern int volatile_ok;

int
nonindexed_operand(op, mode)
    register rtx op;
    enum machine_mode mode;
{
  register enum rtx_code code = GET_CODE (op);
  int mode_altering_drug = 0;

  if (mode == VOIDmode)
    mode = GET_MODE (op);

  if (CONSTANT_P (op))
    return ((GET_MODE (op) == VOIDmode || GET_MODE (op) == mode)
	    && LEGITIMATE_CONSTANT_P (op));

  /* Except for certain constants with VOIDmode, already checked for,
     OP's mode must match MODE if MODE specifies a mode.  */

  if (GET_MODE (op) != mode)
    return 0;

  while (code == SUBREG)
    {
      op = SUBREG_REG (op);
      code = GET_CODE (op);
#if 0
      /* No longer needed, since (SUBREG (MEM...))
	 will load the MEM into a reload reg in the MEM's own mode.  */
      mode_altering_drug = 1;
#endif
    }
  if (code == REG)
    return 1;
  if (code == CONST_DOUBLE)
    return LEGITIMATE_CONSTANT_P (op);
  if (code == MEM)
    {
      register rtx y = XEXP (op, 0);
      if (! volatile_ok && MEM_VOLATILE_P (op))
	return 0;
    GO_IF_NONINDEXED_ADDRESS (y, win);
    }
  return 0;

 win:
  if (mode_altering_drug)
    return ! mode_dependent_address_p (XEXP (op, 0));
  return 1;
}

int
has_direct_base (op)
     rtx op;
{
  if ((GET_CODE (op) == PLUS
       && (CONSTANT_ADDRESS_P (XEXP (op, 1))
	   || CONSTANT_ADDRESS_P (XEXP (op, 0))))
      || CONSTANT_ADDRESS_P (op))
    return 1;

  return 0;
}

int
has_index (op)
     rtx op;
{
  if (GET_CODE (op) == PLUS
      && (GET_CODE (XEXP (op, 0)) == MULT
	  || (GET_CODE (XEXP (op, 1)) == MULT)))
    return 1;
  else
    return 0;
}

int swap_operands;

/* weird_memory_memory -- return 1 if OP1 and OP2 can be compared (or
   exchanged with xchw) with one instruction.  If the operands need to
   be swapped, set the global variable SWAP_OPERANDS.  This function
   silently assumes that both OP0 and OP1 are valid memory references.
   */

int
weird_memory_memory (op0, op1)
     rtx op0, op1;
{
  int ret;
  int c;
  enum rtx_code code0, code1;

  op0 = XEXP (op0, 0);
  op1 = XEXP (op1, 0);
  code0 = GET_CODE (op0);
  code1 = GET_CODE (op1);

  swap_operands = 0;

  if (code1 == REG)
    {
      return 1;
    }
  if (code0 == REG)
    {
      swap_operands = 1;
      return 1;
    }
  if (has_direct_base (op0) && has_direct_base (op1))
    {
      if (has_index (op1))
	{
	  if (has_index (op0))
	    return 0;
	  swap_operands = 1;
	}

      return 1;
    }
  return 0;
}

int
signed_comparison (x, mode)
     rtx x;
     enum machine_mode mode;
{
  enum rtx_code code = GET_CODE (x);

  return (code == NE || code == EQ || code == GE || code == GT || code == LE
	 || code == LT);
}

char *
output_branch (code)
     enum rtx_code code;
{
  switch (code)
    {
    case NE:  return "bne %l4";
    case EQ:  return "beq %l4";
    case GE:  return "bge %l4";
    case GT:  return "bgt %l4";
    case LE:  return "ble %l4";
    case LT:  return "blt %l4";
    }
}

char *
output_inv_branch (code)
     enum rtx_code code;
{
  switch (code)
    {
    case NE:  return "beq %l4";
    case EQ:  return "bne %l4";
    case GE:  return "ble %l4";
    case GT:  return "blt %l4";
    case LE:  return "bge %l4";
    case LT:  return "bgt %l4";
    }
}

extern rtx force_reg ();
rtx test_op0, test_op1;

rtx
ensure_extended (op, extop)
     rtx op;
     enum rtx_code extop;
{
  if (GET_MODE (op) == HImode || GET_MODE (op) == QImode)
    op = gen_rtx (extop, SImode, op);
  op = force_reg (SImode, op);
  return op;
}

/* Sign-extend or zero-extend constant X from FROM_MODE to TO_MODE.  */

rtx
extend_const (x, extop, from_mode, to_mode)
    rtx x;
    enum rtx_code extop;
    enum machine_mode from_mode, to_mode;
{
  int val = INTVAL (x);
  int negative = val & (1 << (GET_MODE_BITSIZE (from_mode) - 1));
  if (from_mode == to_mode)
    return x;
  if (GET_MODE_BITSIZE (from_mode) == HOST_BITS_PER_INT)
    abort ();
  if (negative && extop == SIGN_EXTEND)
    val = val | ((-1) << (GET_MODE_BITSIZE (from_mode)));
  else
    val = val & ~((-1) << (GET_MODE_BITSIZE (from_mode)));
  if (GET_MODE_BITSIZE (to_mode) == HOST_BITS_PER_INT)
    return gen_rtx (CONST_INT, VOIDmode, val);
  return gen_rtx (CONST_INT, VOIDmode,
		  val & ~((-1) << (GET_MODE_BITSIZE (to_mode))));
}

/* Emit rtl for a branch, as well as any delayed (integer) compare insns.
   The compare insn to perform is determined by the global variables
   test_op0 and test_op1.  */

void
extend_and_branch (extop)
     enum rtx_code extop;
{
  rtx op0, op1;
  enum rtx_code code0, code1;

  op0 = test_op0, op1 = test_op1;
  if (op0 == 0)
    return;

  code0 = GET_CODE (op0);
  if (op1 != 0)
    code1 = GET_CODE (op1);
  test_op0 = test_op1 = 0;

  if (op1 == 0)
    {
      op0 = ensure_extended (op0, extop);
      emit_insn (gen_rtx (SET, VOIDmode, cc0_rtx, op0));
    }
  else
    {
      if (CONSTANT_P (op0) && CONSTANT_P (op1))
	{
	  op0 = force_reg (SImode, op0);
	  op1 = force_reg (SImode, op1);
	}
      else if (extop == ZERO_EXTEND && GET_MODE (op0) == HImode)
	{
	  /* Pyramids have no unsigned "cmphi" instructions.  We need to
	     zero extend unsigned halfwords into temporary registers. */
	  op0 = ensure_extended (op0, extop);
	  op1 = ensure_extended (op1, extop);
	}
      else if (CONSTANT_P (op0))
	{
	  op0 = extend_const (op0, extop, GET_MODE (op1), SImode);
	  op1 = ensure_extended (op1, extop);
	}
      else if (CONSTANT_P (op1))
	{
	  op1 = extend_const (op1, extop, GET_MODE (op0), SImode);
	  op0 = ensure_extended (op0, extop);
	}
      else if (code0 == REG && code1 == REG)
	{
	  /* I could do this case without extension, by using the virtual
	     register address (but that would lose for global regs).  */
	  op0 = ensure_extended (op0, extop);
	  op1 = ensure_extended (op1, extop);
	}
      else if (code0 == MEM && code1 == MEM)
	{
	  /* Load into a reg if the address combination can't be handled
	     directly.  */
	  if (! weird_memory_memory (op0, op1))
	    op0 = force_reg (GET_MODE (op0), op0);
	}

      emit_insn (gen_rtx (SET, VOIDmode, cc0_rtx,
			  gen_rtx (COMPARE, VOIDmode, op0, op1)));
    }
}

/* Return non-zero if the two single-word operations with operands[0]
   and operands[1] for the first single-word operation, and operands[2]
   and operands[3] for the second single-word operation, is possible to
   combine to a double word operation.

   The criterion is whether the operands are in consecutive memory cells,
   registers, etc.  */

int
movdi_possible (operands)
     rtx operands[];
{
  int cnst_diff0, cnst_diff1;

  cnst_diff0 = consecutive_operands (operands[0], operands[2]);
  if (cnst_diff0 == 0)
    return 0;

  cnst_diff1 = consecutive_operands (operands[1], operands[3]);
  if (cnst_diff0 & cnst_diff1)
    {
      if (cnst_diff0 & 1)
	swap_operands = 0;
      else
	swap_operands = 1;
      return 1;
    }
  return 0;
}

/* Return +1 of OP0 is a consecutive operand to OP1, -1 if OP1 is a
   consecutive operand to OP0.

   This function is used to determine if addresses are consecutive,
   and therefore possible to combine to fewer instructions.  */

int
consecutive_operands (op0, op1)
     rtx op0, op1;
{
  enum rtx_code code0, code1;
  int cnst_diff;

  code0 = GET_CODE (op0);
  code1 = GET_CODE (op1);

  if (CONSTANT_P (op0) && CONSTANT_P (op1))
    {
      if (op0 == const0_rtx)
	if (op1 == const0_rtx)
	  return 3;
	else
	  return 2;
      if (op1 == const0_rtx)
	return 1;
    }

  if (code0 != code1)
    return 0;

  if (code0 == REG)
    {
      cnst_diff = REGNO (op0) - REGNO (op1);
      if (cnst_diff == 1)
	return 1;
      else if (cnst_diff == -1)
	return 2;
    }
  else if (code0 == MEM)
    {
      cnst_diff = radr_diff (XEXP (op0, 0), XEXP (op1, 0));
      if (cnst_diff)
	if (cnst_diff == 4)
	  return 1;
	else if (cnst_diff == -4)
	  return 2;
    }
  return 0;
}

/* Return the constant difference of the rtx expressions OP0 and OP1,
   or 0 if the y don't have a constant difference.

   This function is used to determine if addresses are consecutive,
   and therefore possible to combine to fewer instructions.  */

int
radr_diff (op0, op1)
     rtx op0, op1;
{
  enum rtx_code code0, code1;
  int cnst_diff;

  code0 = GET_CODE (op0);
  code1 = GET_CODE (op1);

  if (code0 != code1)
    {
      if (code0 == PLUS)
	{
	  if (GET_CODE (XEXP (op0, 1)) == CONST_INT
	      && rtx_equal_p (op1, XEXP (op0, 0)))
	    return INTVAL (XEXP (op0, 1));
	}
      else if (code1 == PLUS)
	{
	  if (GET_CODE (XEXP (op1, 1)) == CONST_INT
	      && rtx_equal_p (op0, XEXP (op1, 0)))
	    return -INTVAL (XEXP (op1, 1));
	}
      return 0;
    }

  if (code0 == CONST_INT)
    return INTVAL (op0) - INTVAL (op1);

  if (code0 == PLUS)
    {
      cnst_diff = radr_diff (XEXP (op0, 0), XEXP (op1, 0));
      if (cnst_diff)
	return (rtx_equal_p (XEXP (op0, 1), XEXP (op1, 1)))
	  ? cnst_diff : 0;
      cnst_diff = radr_diff (XEXP (op0, 1), XEXP (op1, 1));
      if (cnst_diff)
	return (rtx_equal_p (XEXP (op0, 0), XEXP (op1, 0)))
	  ? cnst_diff : 0;
    }

  return 0;
}

int
already_sign_extended (insn, from_mode, op)
     rtx insn;
     enum machine_mode from_mode;
     rtx op;
{
  rtx xinsn;

  return 0;

#if 0
  for (;;)
    {
      insn = PREV_INSN (insn);
      if (insn == 0)
	return 0;
      if (GET_CODE (insn) == NOTE)
	continue;
      if (GET_CODE (insn) != INSN)
	return 0;
      xinsn = PATTERN (insn);

      if (GET_CODE (xinsn) != SET)
	return 0;

      /* Is it another register that is set in this insn?  */
      if (GET_CODE (SET_DEST (xinsn)) != REG
	  || REGNO (SET_DEST (xinsn)) != REGNO (op))
	continue;

      if (GET_CODE (SET_SRC (xinsn)) == SIGN_EXTEND
	  || (GET_CODE (SET_SRC (xinsn)) == MEM
	      && GET_MODE (SET_SRC (xinsn)) == from_mode))
	return 1;

      /* Is the register modified by another operation?  */
      if (REGNO (SET_DEST (xinsn)) == REGNO (op))
	return 0;
    }
#endif
}

char *
output_move_double (operands)
     rtx *operands;
{
  CC_STATUS_INIT;
  if (GET_CODE (operands[1]) == CONST_DOUBLE)
    {
      if (GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT)
	{
	  /* In an integer, the low-order word is in CONST_DOUBLE_LOW.  */
	  rtx const_op = operands[1];
	  if (CONST_DOUBLE_HIGH (const_op) == 0)
	    {
	      operands[1] = gen_rtx (CONST_INT, VOIDmode,
				     CONST_DOUBLE_LOW (const_op));
	      return "movl %1,%0";
	    }
	  operands[1] = gen_rtx (CONST_INT, VOIDmode,
				 CONST_DOUBLE_HIGH (const_op));
	  output_asm_insn ("movw %1,%0", operands);
	  operands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
	  operands[1] = gen_rtx (CONST_INT, VOIDmode,
				 CONST_DOUBLE_LOW (const_op));
	  return "movw %1,%0";
	}
      else
	{
	  /* In a real, the low-address word is in CONST_DOUBLE_LOW.  */
	  rtx const_op = operands[1];
	  if (CONST_DOUBLE_LOW (const_op) == 0)
	    {
	      operands[1] = gen_rtx (CONST_INT, VOIDmode,
				     CONST_DOUBLE_HIGH (const_op));
	      return "movl %1,%0";
	    }
	  operands[1] = gen_rtx (CONST_INT, VOIDmode,
				 CONST_DOUBLE_LOW (const_op));
	  output_asm_insn ("movw %1,%0", operands);
	  operands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
	  operands[1] = gen_rtx (CONST_INT, VOIDmode,
				 CONST_DOUBLE_HIGH (const_op));
	  return "movw %1,%0";
	}
    }

  return "movl %1,%0";
}

/* Return non-zero if the code of this rtx pattern is a relop.  */
int
relop (op, mode)
     rtx op;
     enum machine_mode mode;
{
  switch (GET_CODE (op))
    {
    case EQ:
    case NE:
    case LT:
    case LE:
    case GE:
    case GT:
    case LTU:
    case LEU:
    case GEU:
    case GTU:
      return 1;
    }
  return 0;
}
