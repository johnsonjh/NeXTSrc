/* Generated automatically by the program `genoutput'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "conditions.h"
#include "insn-flags.h"
#include "insn-config.h"

#ifndef __STDC__
#define const
#endif

#include "output.h"
#include "aux-output.c"

#ifndef INSN_MACHINE_INFO
#define INSN_MACHINE_INFO struct dummy1 {int i;}
#endif


static char *
output_0 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[1]))
    return "fmove%.d %f1,%0";
  if (FPA_REG_P (operands[1]))
    return "fpmove%.d %1, %x0";
  return output_move_double (operands);
}
}

static char *
output_1 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return output_move_double (operands);
}
}

static char *
output_2 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef ISI_OV
  /* ISI's assembler fails to handle tstl a0.  */
  if (! ADDRESS_REG_P (operands[0]))
#else
  if (TARGET_68020 || ! ADDRESS_REG_P (operands[0]))
#endif
    return "tst%.l %0";
  /* If you think that the 68020 does not support tstl a0,
     reread page B-167 of the 68020 manual more carefully.  */
  /* On an address reg, cmpw may replace cmpl.  */
#ifdef HPUX_ASM
  return "cmp%.w %0,%#0";
#else
  return "cmp%.w %#0,%0";
#endif
}
}

static char *
output_3 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef ISI_OV
  if (! ADDRESS_REG_P (operands[0]))
#else
  if (TARGET_68020 || ! ADDRESS_REG_P (operands[0]))
#endif
    return "tst%.w %0";
#ifdef HPUX_ASM
  return "cmp%.w %0,%#0";
#else
  return "cmp%.w %#0,%0";
#endif
}
}

static char *
output_7 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.flags = CC_IN_68881;
  if (FP_REG_P (operands[0]))
    return "ftst%.x %0";
  return "ftst%.s %0";
}
}

static char *
output_10 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.flags = CC_IN_68881;
  if (FP_REG_P (operands[0]))
    return "ftst%.x %0";
  return "ftst%.d %0";
}
}

static char *
output_11 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM)
    return "cmpm%.l %1,%0";
  if (REG_P (operands[1])
      || (!REG_P (operands[0]) && GET_CODE (operands[0]) != MEM))
    { cc_status.flags |= CC_REVERSED;
#ifdef HPUX_ASM
      return "cmp%.l %d1,%d0";
#else
      return "cmp%.l %d0,%d1"; 
#endif
    }
#ifdef HPUX_ASM
  return "cmp%.l %d0,%d1";
#else
  return "cmp%.l %d1,%d0";
#endif
}
}

static char *
output_12 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM)
    return "cmpm%.w %1,%0";
  if ((REG_P (operands[1]) && !ADDRESS_REG_P (operands[1]))
      || (!REG_P (operands[0]) && GET_CODE (operands[0]) != MEM))
    { cc_status.flags |= CC_REVERSED;
#ifdef HPUX_ASM
      return "cmp%.w %d1,%d0";
#else
      return "cmp%.w %d0,%d1"; 
#endif
    }
#ifdef HPUX_ASM
  return "cmp%.w %d0,%d1";
#else
  return "cmp%.w %d1,%d0";
#endif
}
}

static char *
output_13 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM)
    return "cmpm%.b %1,%0";
  if (REG_P (operands[1])
      || (!REG_P (operands[0]) && GET_CODE (operands[0]) != MEM))
    { cc_status.flags |= CC_REVERSED;
#ifdef HPUX_ASM
      return "cmp%.b %d1,%d0";
#else
      return "cmp%.b %d0,%d1";
#endif
    }
#ifdef HPUX_ASM
  return "cmp%.b %d0,%d1";
#else
  return "cmp%.b %d1,%d0";
#endif
}
}

static char *
output_16 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.flags = CC_IN_68881;
#ifdef HPUX_ASM
  if (REG_P (operands[0]))
    {
      if (REG_P (operands[1]))
	return "fcmp%.x %0,%1";
      else
        return "fcmp%.d %0,%f1";
    }
  cc_status.flags |= CC_REVERSED;
  return "fcmp%.d %1,%f0";
#else
  if (REG_P (operands[0]))
    {
      if (REG_P (operands[1]))
	return "fcmp%.x %1,%0";
      else
        return "fcmp%.d %f1,%0";
    }
  cc_status.flags |= CC_REVERSED;
  return "fcmp%.d %f0,%1";
#endif
}
}

static char *
output_19 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.flags = CC_IN_68881;
#ifdef HPUX_ASM
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fcmp%.x %0,%1";
      else
        return "fcmp%.s %0,%f1";
    }
  cc_status.flags |= CC_REVERSED;
  return "fcmp%.s %1,%f0";
#else
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fcmp%.x %1,%0";
      else
        return "fcmp%.s %f1,%0";
    }
  cc_status.flags |= CC_REVERSED;
  return "fcmp%.s %f0,%1";
#endif
}
}

static char *
output_20 (operands, insn)
     rtx *operands;
     rtx insn;
{
 { return output_btst (operands, operands[1], operands[0], insn, 7); }
}

static char *
output_21 (operands, insn)
     rtx *operands;
     rtx insn;
{
 { return output_btst (operands, operands[1], operands[0], insn, 31); }
}

static char *
output_22 (operands, insn)
     rtx *operands;
     rtx insn;
{
 { return output_btst (operands, operands[1], operands[0], insn, 7); }
}

static char *
output_23 (operands, insn)
     rtx *operands;
     rtx insn;
{
 { return output_btst (operands, operands[1], operands[0], insn, 31); }
}

static char *
output_24 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  operands[1] = gen_rtx (CONST_INT, VOIDmode, 7 - INTVAL (operands[1]));
  return output_btst (operands, operands[1], operands[0], insn, 7);
}
}

static char *
output_25 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM)
    {
      operands[0] = adj_offsettable_operand (operands[0],
					     INTVAL (operands[1]) / 8);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, 
			     7 - INTVAL (operands[1]) % 8);
      return output_btst (operands, operands[1], operands[0], insn, 7);
    }
  operands[1] = gen_rtx (CONST_INT, VOIDmode,
			 15 - INTVAL (operands[1]));
  return output_btst (operands, operands[1], operands[0], insn, 15);
}
}

static char *
output_26 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM)
    {
      operands[0] = adj_offsettable_operand (operands[0],
					     INTVAL (operands[1]) / 8);
      operands[1] = gen_rtx (CONST_INT, VOIDmode, 
			     7 - INTVAL (operands[1]) % 8);
      return output_btst (operands, operands[1], operands[0], insn, 7);
    }
  operands[1] = gen_rtx (CONST_INT, VOIDmode,
			 31 - INTVAL (operands[1]));
  return output_btst (operands, operands[1], operands[0], insn, 31);
}
}

static char *
output_27 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.flags = CC_Z_IN_NOT_N | CC_NOT_NEGATIVE;
  return "tst%.b %0";
}
}

static char *
output_28 (operands, insn)
     rtx *operands;
     rtx insn;
{

{ register int log = exact_log2 (INTVAL (operands[1]));
  operands[1] = gen_rtx (CONST_INT, VOIDmode, log);
  return output_btst (operands, operands[1], operands[0], insn, 7);
}
}

static char *
output_29 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const0_rtx)
    return "clr%.l %0";
  return "pea %a1";
}
}

static char *
output_30 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (ADDRESS_REG_P (operands[0]))
    return "sub%.l %0,%0";
  /* moveq is faster on the 68000.  */
  if (DATA_REG_P (operands[0]) && !TARGET_68020)
#ifdef MOTOROLA
    return "moveq%.l %#0,%0";
#else
    return "moveq %#0,%0";
#endif
  return "clr%.l %0";
}
}

static char *
output_31 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 3)
    return "fpmove%.l %x1,fpa0\n\tfpmove%.l fpa0,%x0";	
  if (FPA_REG_P (operands[1]) || FPA_REG_P (operands[0]))
    return "fpmove%.l %x1,%x0";
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      if (operands[1] == const0_rtx
	  && (DATA_REG_P (operands[0])
	      || GET_CODE (operands[0]) == MEM))
	return "clr%.l %0";
      else if (DATA_REG_P (operands[0])
	       && INTVAL (operands[1]) < 128
	       && INTVAL (operands[1]) >= -128)
        {
#ifdef MOTOROLA
          return "moveq%.l %1,%0";
#else
	  return "moveq %1,%0";
#endif
	}
      else if (ADDRESS_REG_P (operands[0])
	       && INTVAL (operands[1]) < 0x8000
	       && INTVAL (operands[1]) >= -0x8000)
	return "move%.w %1,%0";
      else if (push_operand (operands[0], SImode)
	       && INTVAL (operands[1]) < 0x8000
	       && INTVAL (operands[1]) >= -0x8000)
        return "pea %a1";
    }
  else if ((GET_CODE (operands[1]) == SYMBOL_REF
	    || GET_CODE (operands[1]) == CONST)
	   && push_operand (operands[0], SImode))
    return "pea %a1";
  else if ((GET_CODE (operands[1]) == SYMBOL_REF
	    || GET_CODE (operands[1]) == CONST)
	   && ADDRESS_REG_P (operands[0]))
    return "lea %a1,%0";
  return "move%.l %1,%0";
}
}

static char *
output_32 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      if (operands[1] == const0_rtx
	  && (DATA_REG_P (operands[0])
	      || GET_CODE (operands[0]) == MEM))
	return "clr%.w %0";
      else if (DATA_REG_P (operands[0])
	       && INTVAL (operands[1]) < 128
	       && INTVAL (operands[1]) >= -128)
        {
#ifdef MOTOROLA
          return "moveq%.l %1,%0";
#else
	  return "moveq %1,%0";
#endif
	}
      else if (INTVAL (operands[1]) < 0x8000
	       && INTVAL (operands[1]) >= -0x8000)
	return "move%.w %1,%0";
    }
  else if (CONSTANT_P (operands[1]))
    return "move%.l %1,%0";
#ifndef SONY_ASM
  /* Recognize the insn before a tablejump, one that refers
     to a table of offsets.  Such an insn will need to refer
     to a label on the insn.  So output one.  Use the label-number
     of the table of offsets to generate this label.  */
  if (GET_CODE (operands[1]) == MEM
      && GET_CODE (XEXP (operands[1], 0)) == PLUS
      && (GET_CODE (XEXP (XEXP (operands[1], 0), 0)) == LABEL_REF
	  || GET_CODE (XEXP (XEXP (operands[1], 0), 1)) == LABEL_REF)
      && GET_CODE (XEXP (XEXP (operands[1], 0), 0)) != PLUS
      && GET_CODE (XEXP (XEXP (operands[1], 0), 1)) != PLUS)
    {
      rtx labelref;
      if (GET_CODE (XEXP (XEXP (operands[1], 0), 0)) == LABEL_REF)
	labelref = XEXP (XEXP (operands[1], 0), 0);
      else
	labelref = XEXP (XEXP (operands[1], 0), 1);
#if defined (MOTOROLA) && ! defined (SGS_3B1)
#ifdef SGS
      fprintf (asm_out_file, "\tset %s%d,.+2\n", "LI",
	       CODE_LABEL_NUMBER (XEXP (labelref, 0)));
#else /* not SGS */
      fprintf (asm_out_file, "\t.set %s%d,.+2\n", "LI",
	       CODE_LABEL_NUMBER (XEXP (labelref, 0)));
#endif /* not SGS */
#else /* SGS_3B1 or not MOTOROLA */
      ASM_OUTPUT_INTERNAL_LABEL (asm_out_file, "LI",
				 CODE_LABEL_NUMBER (XEXP (labelref, 0)));
      /* For sake of 3b1, set flag saying we need to define the symbol
         LD%n (with value L%n-LI%n) at the end of the switch table.  */
      RTX_INTEGRATED_P (next_real_insn (XEXP (labelref, 0))) = 1;
#endif /* SGS_3B1 or not MOTOROLA */
    }
#endif /* SONY_ASM */
  return "move%.w %1,%0";
}
}

static char *
output_33 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      if (operands[1] == const0_rtx
	  && (DATA_REG_P (operands[0])
	      || GET_CODE (operands[0]) == MEM))
	return "clr%.w %0";
    }
  return "move%.w %1,%0";
}
}

static char *
output_34 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  rtx xoperands[4];
  if (ADDRESS_REG_P (operands[0]) && GET_CODE (operands[1]) == MEM)
    {
      xoperands[1] = operands[1];
      xoperands[2]
        = gen_rtx (MEM, QImode,
		   gen_rtx (PLUS, VOIDmode, stack_pointer_rtx, const1_rtx));
      xoperands[3] = stack_pointer_rtx;
      /* Just pushing a byte puts it in the high byte of the halfword.  */
      /* We must put it in the low half, the second byte.  */
      output_asm_insn ("subq%.w %#2,%3\n\tmove%.b %1,%2", xoperands);
      return "move%.w %+,%0";
    }
  if (ADDRESS_REG_P (operands[1]) && GET_CODE (operands[0]) == MEM)
    {
      xoperands[0] = operands[0];
      xoperands[1] = operands[1];
      xoperands[2]
        = gen_rtx (MEM, QImode,
		   gen_rtx (PLUS, VOIDmode, stack_pointer_rtx, const1_rtx));
      xoperands[3] = stack_pointer_rtx;
      output_asm_insn ("move%.w %1,%-\n\tmove%.b %2,%0\n\taddq%.w %#2,%3", xoperands);
      return "";
    }
  if (operands[1] == const0_rtx)
    return "clr%.b %0";
  if (GET_CODE (operands[1]) == CONST_INT
      && INTVAL (operands[1]) == -1)
    return "st %0";
  if (GET_CODE (operands[1]) != CONST_INT && CONSTANT_P (operands[1]))
    return "move%.l %1,%0";
  if (ADDRESS_REG_P (operands[0]) || ADDRESS_REG_P (operands[1]))
    return "move%.w %1,%0";
  return "move%.b %1,%0";
}
}

static char *
output_35 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const0_rtx)
    return "clr%.b %0";
  return "move%.b %1,%0";
}
}

static char *
output_36 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative >= 4)
    return "fpmove%.s %1,fpa0\n\tfpmove%.s fpa0,%0";
  if (FPA_REG_P (operands[0]))
    {
      if (FPA_REG_P (operands[1]))
	return "fpmove%.s %x1,%x0";
      else if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_single (operands);
      else if (FP_REG_P (operands[1]))
        return "fmove%.s %1,sp@-\n\tfpmove%.d sp@+, %0";
      return "fpmove%.s %x1,%x0";
    }
  if (FPA_REG_P (operands[1]))
    {
      if (FP_REG_P (operands[0]))
	return "fpmove%.s %x1,sp@-\n\tfmove%.s sp@+,%0";
      else
	return "fpmove%.s %x1,%x0";
    }
  if (FP_REG_P (operands[0]))
    {
#ifdef NeXT
      if (TARGET_IEEE_MATH)
	{
	  if (FP_REG_P (operands[1]))
	    return "fsmove%.x %1,%0";
	  else if (ADDRESS_REG_P (operands[1]))
	    return "move%.l %1,%-\n\tfsmove%.s %+,%0";
	  else if (GET_CODE (operands[1]) == CONST_DOUBLE)
	    return output_move_const_single (operands);
	  return "fsmove%.s %f1,%0";
	}
#endif  /* NeXT */
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      else if (ADDRESS_REG_P (operands[1]))
	return "move%.l %1,%-\n\tfmove%.s %+,%0";
      else if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_single (operands);
      return "fmove%.s %f1,%0";
    }
  if (FP_REG_P (operands[1]))
    {
      if (ADDRESS_REG_P (operands[0]))
	return "fmove%.s %1,%-\n\tmove%.l %+,%0";
      return "fmove%.s %f1,%0";
    }
  return "move%.l %1,%0";
}
}

static char *
output_37 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 6)
    return "fpmove%.d %x1,fpa0\n\tfpmove%.d fpa0,%x0";
  if (FPA_REG_P (operands[0]))
    {
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      if (FP_REG_P (operands[1]))
        return "fmove%.d %1,sp@-\n\tfpmove%.d sp@+,%x0";
      return "fpmove%.d %x1,%x0";
    }
  else if (FPA_REG_P (operands[1]))
    {
      if (FP_REG_P(operands[0]))
        return "fpmove%.d %x1,sp@-\n\tfmoved sp@+,%0";
      else
        return "fpmove%.d %x1,%x0";
    }
  if (FP_REG_P (operands[0]))
    {
#ifdef NeXT
      if (TARGET_IEEE_MATH)
	{
	  if (FP_REG_P (operands[1]))
	    return "fdmove%.x %1,%0";
	  if (REG_P (operands[1]))
	    {
	      rtx xoperands[2];
	      xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[1]) + 1);
	      output_asm_insn ("move%.l %1,%-", xoperands);
	      output_asm_insn ("move%.l %1,%-", operands);
	      return "fdmove%.d %+,%0";
	    }
	  if (GET_CODE (operands[1]) == CONST_DOUBLE)
	    return output_move_const_double (operands);
	  return "fdmove%.d %f1,%0";
	}
#endif  /* NeXT */
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "fmove%.d %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      return "fmove%.d %f1,%0";
    }
  else if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
	{
	  output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
	  operands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
	  return "move%.l %+,%0";
	}
      else
        return "fmove%.d %f1,%0";
    }
  return output_move_double (operands);
}

}

static char *
output_38 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 8)
    return "fpmove%.d %x1,fpa0\n\tfpmove%.d fpa0,%x0";
  if (FPA_REG_P (operands[0]) || FPA_REG_P (operands[1]))
    return "fpmove%.d %x1,%x0";
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "fmove%.d %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      return "fmove%.d %f1,%0";
    }
  else if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
	{
	  output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
	  operands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
	  return "move%.l %+,%0";
	}
      else
        return "fmove%.d %f1,%0";
    }
  return output_move_double (operands);
}

}

static char *
output_40 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";
  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adj_offsettable_operand (operands[1], 3);
  return "move%.b %1,%0";
}
}

static char *
output_41 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == REG
      && (GET_CODE (operands[1]) == MEM
	  || GET_CODE (operands[1]) == CONST_INT))
    return "move%.w %1,%0";
  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";
  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adj_offsettable_operand (operands[1], 1);
  return "move%.b %1,%0";
}
}

static char *
output_42 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";
  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adj_offsettable_operand (operands[1], 2);
  return "move%.w %1,%0";
}
}

static char *
output_46 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (DATA_REG_P (operands[0]))
    {
      if (GET_CODE (operands[1]) == REG
	  && REGNO (operands[0]) == REGNO (operands[1]))
	return "and%.l %#0xFFFF,%0";
      if (reg_mentioned_p (operands[0], operands[1]))
        return "move%.w %1,%0\n\tand%.l %#0xFFFF,%0";
      return "clr%.l %0\n\tmove%.w %1,%0";
    }
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
    return "move%.w %1,%0\n\tclr%.w %0";
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == POST_INC)
    return "clr%.w %0\n\tmove%.w %1,%0";
  else
    {
      output_asm_insn ("clr%.w %0", operands);
      operands[0] = adj_offsettable_operand (operands[0], 2);
      return "move%.w %1,%0";
    }
}
}

static char *
output_47 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (DATA_REG_P (operands[0]))
    {
      if (GET_CODE (operands[1]) == REG
	  && REGNO (operands[0]) == REGNO (operands[1]))
	return "and%.w %#0xFF,%0";
      if (reg_mentioned_p (operands[0], operands[1]))
        return "move%.b %1,%0\n\tand%.w %#0xFF,%0";
      return "clr%.w %0\n\tmove%.b %1,%0";
    }
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
    {
      if (REGNO (XEXP (XEXP (operands[0], 0), 0))
	  == STACK_POINTER_REGNUM)
	return "clr%.w %-\n\tmove%.b %1,%0";
      else
	return "move%.b %1,%0\n\tclr%.b %0";
    }
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == POST_INC)
    return "clr%.b %0\n\tmove%.b %1,%0";
  else
    {
      output_asm_insn ("clr%.b %0", operands);
      operands[0] = adj_offsettable_operand (operands[0], 1);
      return "move%.b %1,%0";
    }
}
}

static char *
output_48 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (DATA_REG_P (operands[0]))
    {
      if (GET_CODE (operands[1]) == REG
	  && REGNO (operands[0]) == REGNO (operands[1]))
	return "and%.l %#0xFF,%0";
      if (reg_mentioned_p (operands[0], operands[1]))
        return "move%.b %1,%0\n\tand%.l %#0xFF,%0";
      return "clr%.l %0\n\tmove%.b %1,%0";
    }
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
    {
      operands[0] = XEXP (XEXP (operands[0], 0), 0);
#ifdef MOTOROLA
#ifdef SGS
      return "clr.l -(%0)\n\tmove%.b %1,3(%0)";
#else
      return "clr.l -(%0)\n\tmove%.b %1,(3,%0)";
#endif
#else
      return "clrl %0@-\n\tmoveb %1,%0@(3)";
#endif
    }
  else if (GET_CODE (operands[0]) == MEM
	   && GET_CODE (XEXP (operands[0], 0)) == POST_INC)
    {
      operands[0] = XEXP (XEXP (operands[0], 0), 0);
#ifdef MOTOROLA
#ifdef SGS
      return "clr.l (%0)+\n\tmove%.b %1,-1(%0)";
#else
      return "clr.l (%0)+\n\tmove%.b %1,(-1,%0)";
#endif
#else
      return "clrl %0@+\n\tmoveb %1,%0@(-1)";
#endif
    }
  else
    {
      output_asm_insn ("clr%.l %0", operands);
      operands[0] = adj_offsettable_operand (operands[0], 3);
      return "move%.b %1,%0";
    }
}
}

static char *
output_49 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (ADDRESS_REG_P (operands[0]))
    return "move%.w %1,%0";
  return "ext%.l %0";
}
}

static char *
output_54 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      if (REGNO (operands[0]) == REGNO (operands[1]))
	{
	  /* Extending float to double in an fp-reg is a no-op.
	     NOTICE_UPDATE_CC has already assumed that the
	     cc will be set.  So cancel what it did.  */
	  cc_status = cc_prev_status;
	  return "";
	}
#ifdef NeXT
      if (TARGET_IEEE_MATH)
        return "fdmove%.x %1,%0";
#endif  /* NeXT */
      return "fmove%.x %1,%0";
    }
#ifdef NeXT
    if (TARGET_IEEE_MATH && FP_REG_P (operands[0]))
      return "fdmove%.s %f1,%0";
#endif  /* NeXT */
  if (FP_REG_P (operands[0]))
    return "fmove%.s %f1,%0";
  if (DATA_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
      operands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
      return "move%.l %+,%0";
    }
  return "fmove%.d %f1,%0";
}
}

static char *
output_57 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[1]))
    return "fsmove%.x %1,%0";
  return "fsmove%.d %f1,%0";
}
}

static char *
output_61 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fsmove%.l %1,%0";
#endif  /* NeXT */
  return "fmove%.l %1,%0";
}
}

static char *
output_64 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fdmove%.l %1,%0";
#endif  /* NeXT */
  return "fmove%.l %1,%0";
}
}

static char *
output_65 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fsmove%.w %1,%0";
#endif  /* NeXT */
  return "fmove%.w %1,%0";
}
}

static char *
output_66 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fdmove%.l %1,%0";
#endif  /* NeXT */
  return "fmove%.w %1,%0";
}
}

static char *
output_67 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fsmove%.b %1,%0";
#endif  /* NeXT */
  return "fmove%.b %1,%0";
}
}

static char *
output_68 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    return "fdmove%.l %1,%0";
#endif  /* NeXT */
  return "fmove%.b %1,%0";
}
}

static char *
output_69 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[1]))
    return "fintrz%.x %f1,%0";
  return "fintrz%.d %f1,%0";
}
}

static char *
output_70 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[1]))
    return "fintrz%.x %f1,%0";
  return "fintrz%.s %f1,%0";
}
}

static char *
output_79 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (! operands_match_p (operands[0], operands[1]))
    {
      if (!ADDRESS_REG_P (operands[1]))
	{
	  rtx tmp = operands[1];

	  operands[1] = operands[2];
	  operands[2] = tmp;
	}

      /* These insns can result from reloads to access
	 stack slots over 64k from the frame pointer.  */
      if (GET_CODE (operands[2]) == CONST_INT
	  && INTVAL (operands[2]) + 0x8000 >= (unsigned) 0x10000)
        return "move%.l %2,%0\n\tadd%.l %1,%0";
#ifdef SGS
      if (GET_CODE (operands[2]) == REG)
	return "lea 0(%1,%2.l),%0";
      else
	return "lea %c2(%1),%0";
#else /* not SGS */
#ifdef MOTOROLA
      if (GET_CODE (operands[2]) == REG)
	return "lea (%1,%2.l),%0";
      else
	return "lea (%c2,%1),%0";
#else /* not MOTOROLA (MIT syntax) */
      if (GET_CODE (operands[2]) == REG)
	return "lea %1@(0,%2:l),%0";
      else
	return "lea %1@(%c2),%0";
#endif /* not MOTOROLA */
#endif /* not SGS */
    }
  if (GET_CODE (operands[2]) == CONST_INT)
    {
#ifndef NO_ADDSUB_Q
      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return (ADDRESS_REG_P (operands[0])
		? "addq%.w %2,%0"
		: "addq%.l %2,%0");
      if (INTVAL (operands[2]) < 0
	  && INTVAL (operands[2]) >= -8)
        {
	  operands[2] = gen_rtx (CONST_INT, VOIDmode,
			         - INTVAL (operands[2]));
	  return (ADDRESS_REG_P (operands[0])
		  ? "subq%.w %2,%0"
		  : "subq%.l %2,%0");
	}
#endif
      if (ADDRESS_REG_P (operands[0])
	  && INTVAL (operands[2]) >= -0x8000
	  && INTVAL (operands[2]) < 0x8000)
	return "add%.w %2,%0";
    }
  return "add%.l %2,%0";
}
}

static char *
output_81 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifndef NO_ADDSUB_Q
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return "addq%.w %2,%0";
    }
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) < 0
	  && INTVAL (operands[2]) >= -8)
	{
	  operands[2] = gen_rtx (CONST_INT, VOIDmode,
			         - INTVAL (operands[2]));
	  return "subq%.w %2,%0";
	}
    }
#endif
  return "add%.w %2,%0";
}
}

static char *
output_83 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifndef NO_ADDSUB_Q
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return "addq%.b %2,%0";
    }
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) < 0 && INTVAL (operands[2]) >= -8)
       {
	 operands[2] = gen_rtx (CONST_INT, VOIDmode, - INTVAL (operands[2]));
	 return "subq%.b %2,%0";
       }
    }
#endif
  return "add%.b %2,%0";
}
}

static char *
output_86 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpadd%.d %y2,%0";
  if (rtx_equal_p (operands[0], operands[2]))
    return "fpadd%.d %y1,%0";
  if (which_alternative == 0)
    return "fpadd3%.d %w2,%w1,%0";
  return "fpadd3%.d %x2,%x1,%0";
}
}

static char *
output_87 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]))
	return "fdadd%.x %2,%0";
      return "fdadd%.d %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]))
    return "fadd%.x %2,%0";
  return "fadd%.d %f2,%0";
}
}

static char *
output_89 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpadd%.s %w2,%0";
  if (rtx_equal_p (operands[0], operands[2]))
    return "fpadd%.s %w1,%0";
  if (which_alternative == 0)
    return "fpadd3%.s %w2,%w1,%0";
  return "fpadd3%.s %2,%1,%0";
}
}

static char *
output_90 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
	return "fsadd%.x %2,%0";
      return "fsadd%.s %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
    return "fadd%.x %2,%0";
  return "fadd%.s %f2,%0";
}
}

static char *
output_91 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (! operands_match_p (operands[0], operands[1]))
    {
      if (operands_match_p (operands[0], operands[2]))
	{
#ifndef NO_ADDSUB_Q
	  if (GET_CODE (operands[1]) == CONST_INT)
	    {
	      if (INTVAL (operands[1]) > 0
		  && INTVAL (operands[1]) <= 8)
		return "subq%.l %1,%0\n\tneg%.l %0";
	    }
#endif
	  return "sub%.l %1,%0\n\tneg%.l %0";
	}
      /* This case is matched by J, but negating -0x8000
         in an lea would give an invalid displacement.
	 So do this specially.  */
      if (INTVAL (operands[2]) == -0x8000)
	return "move%.l %1,%0\n\tsub%.l %2,%0";
#ifdef SGS
      return "lea %n2(%1),%0";
#else
#ifdef MOTOROLA
      return "lea (%n2,%1),%0";
#else /* not MOTOROLA (MIT syntax) */
      return "lea %1@(%n2),%0";
#endif /* not MOTOROLA */
#endif /* not SGS */
    }
  if (GET_CODE (operands[2]) == CONST_INT)
    {
#ifndef NO_ADDSUB_Q
      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return "subq%.l %2,%0";
#endif
      if (ADDRESS_REG_P (operands[0])
	  && INTVAL (operands[2]) >= -0x8000
	  && INTVAL (operands[2]) < 0x8000)
	return "sub%.w %2,%0";
    }
  return "sub%.l %2,%0";
}
}

static char *
output_98 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[2]))
    return "fprsub%.d %y1,%0";
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpsub%.d %y2,%0";
  if (which_alternative == 0)
    return "fpsub3%.d %w2,%w1,%0";
  return "fpsub3%.d %x2,%x1,%0";
}
}

static char *
output_99 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]))
	return "fdsub%.x %2,%0";
      return "fdsub%.d %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]))
    return "fsub%.x %2,%0";
  return "fsub%.d %f2,%0";
}
}

static char *
output_101 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[2]))
    return "fprsub%.s %w1,%0";
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpsub%.s %w2,%0";
  if (which_alternative == 0)
    return "fpsub3%.s %w2,%w1,%0";
  return "fpsub3%.s %2,%1,%0";
}
}

static char *
output_102 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
	return "fssub%.x %2,%0";
      return "fssub%.s %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
    return "fsub%.x %2,%0";
  return "fsub%.s %f2,%0";
}
}

static char *
output_103 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "muls.w %2,%0";
#else
  return "muls %2,%0";
#endif
}
}

static char *
output_104 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "muls.w %2,%0";
#else
  return "muls %2,%0";
#endif
}
}

static char *
output_106 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "mulu.w %2,%0";
#else
  return "mulu %2,%0";
#endif
}
}

static char *
output_107 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "mulu.w %2,%0";
#else
  return "mulu %2,%0";
#endif
}
}

static char *
output_110 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[1], operands[2]))
    return "fpsqr%.d %y1,%0";
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpmul%.d %y2,%0";
  if (rtx_equal_p (operands[0], operands[2]))
    return "fpmul%.d %y1,%0";
  if (which_alternative == 0)
    return "fpmul3%.d %w2,%w1,%0"; 
  return "fpmul3%.d %x2,%x1,%0";
}
}

static char *
output_111 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]))
	return "fdmul%.x %2,%0";
      return "fdmul%.d %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]))
    return "fmul%.x %2,%0";
  return "fmul%.d %f2,%0";
}
}

static char *
output_113 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[1], operands[2]))
    return "fpsqr%.s %w1,%0";
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpmul%.s %w2,%0";
  if (rtx_equal_p (operands[0], operands[2]))
    return "fpmul%.s %w1,%0";
  if (which_alternative == 0)
    return "fpmul3%.s %w2,%w1,%0";
  return "fpmul3%.s %2,%1,%0";
}
}

static char *
output_114 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
        return "fsmul%.x %2,%0";
      return "fsmul%.s %f2,%0";
    }
#endif  /* NeXT */
#if 0 /* we are assuming that Motorola WILL implement fsglmul on the 68040 */
  if (TARGET_68040)
    {
      /* Don't use fsglmul on the 68040, since it must be emulated. */
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
      return "fmul%.x %2,%0";
      return "fmul%.s %f2,%0";
    }
#endif
  if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
    return "fsglmul%.x %2,%0";
  return "fsglmul%.s %f2,%0";
}
}

static char *
output_115 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "ext.l %0\n\tdivs.w %2,%0";
#else
  return "extl %0\n\tdivs %2,%0";
#endif
}
}

static char *
output_116 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "divs.w %2,%0";
#else
  return "divs %2,%0";
#endif
}
}

static char *
output_118 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "and.l %#0xFFFF,%0\n\tdivu.w %2,%0";
#else
  return "andl %#0xFFFF,%0\n\tdivu %2,%0";
#endif
}
}

static char *
output_119 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  return "divu.w %2,%0";
#else
  return "divu %2,%0";
#endif
}
}

static char *
output_122 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[2]))
    return "fprdiv%.d %y1,%0";
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpdiv%.d %y2,%0";
  if (which_alternative == 0)
    return "fpdiv3%.d %w2,%w1,%0";
  return "fpdiv3%.d %x2,%x1,%x0";
}
}

static char *
output_123 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]))
	return "fddiv%.x %2,%0";
      return "fddiv%.d %f2,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[2]))
    return "fdiv%.x %2,%0";
  return "fdiv%.d %f2,%0";
}
}

static char *
output_125 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (rtx_equal_p (operands[0], operands[1]))
    return "fpdiv%.s %w2,%0";
  if (rtx_equal_p (operands[0], operands[2]))
    return "fprdiv%.s %w1,%0";
  if (which_alternative == 0)
    return "fpdiv3%.s %w2,%w1,%0";
  return "fpdiv3%.s %2,%1,%0";
}
}

static char *
output_126 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
        return "fsdiv%.x %2,%0";
      return "fsdiv%.s %f2,%0";
    }
#endif  /* NeXT */
#if 0 /* we are assuming that Motorola WILL implement fsgldiv on the 68040 */
  if (TARGET_68040)
    {
      /* Don't use fsgldiv on the 68040, since it must be emulated. */
      if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
      return "fdiv%.x %2,%0";
      return "fdiv%.s %f2,%0";
    }
#endif
  if (REG_P (operands[2]) && ! DATA_REG_P (operands[2]))
    return "fsgldiv%.x %2,%0";
  return "fsgldiv%.s %f2,%0";
}
}

static char *
output_127 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  /* The swap insn produces cc's that don't correspond to the result.  */
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifdef SGS_3B1
  return "ext.l %0\n\tdivs.w %2,%0\n\tswap.w %0";
#else
  return "ext.l %0\n\tdivs.w %2,%0\n\tswap %0";
#endif
#else
  return "extl %0\n\tdivs %2,%0\n\tswap %0";
#endif
}
}

static char *
output_128 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  /* The swap insn produces cc's that don't correspond to the result.  */
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifdef SGS_3B1
  return "divs.w %2,%0\n\tswap.w %0";
#else
  return "divs.w %2,%0\n\tswap %0";
#endif
#else
  return "divs %2,%0\n\tswap %0";
#endif
}
}

static char *
output_129 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  /* The swap insn produces cc's that don't correspond to the result.  */
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifdef SGS_3B1
  return "and.l %#0xFFFF,%0\n\tdivu.w %2,%0\n\tswap.w %0";
#else
  return "and.l %#0xFFFF,%0\n\tdivu.w %2,%0\n\tswap %0";
#endif
#else
  return "andl %#0xFFFF,%0\n\tdivu %2,%0\n\tswap %0";
#endif
}
}

static char *
output_130 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  /* The swap insn produces cc's that don't correspond to the result.  */
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifdef SGS_3B1
  return "divu.w %2,%0\n\tswap.w %0";
#else
  return "divu.w %2,%0\n\tswap %0";
#endif
#else
  return "divu %2,%0\n\tswap %0";
#endif
}
}

static char *
output_133 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[2]) == CONST_INT
      && (INTVAL (operands[2]) | 0xffff) == 0xffffffff
      && (DATA_REG_P (operands[0])
	  || offsettable_memref_p (operands[0])))
    { 
      if (GET_CODE (operands[0]) != REG)
        operands[0] = adj_offsettable_operand (operands[0], 2);
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     INTVAL (operands[2]) & 0xffff);
      /* Do not delete a following tstl %0 insn; that would be incorrect.  */
      CC_STATUS_INIT;
      if (operands[2] == const0_rtx)
        return "clr%.w %0";
      return "and%.w %2,%0";
    }
  return "and%.l %2,%0";
}
}

static char *
output_138 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  register int logval;
  if (GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[2]) >> 16 == 0
      && (DATA_REG_P (operands[0])
	  || offsettable_memref_p (operands[0])))
    { 
      if (GET_CODE (operands[0]) != REG)
        operands[0] = adj_offsettable_operand (operands[0], 2);
      /* Do not delete a following tstl %0 insn; that would be incorrect.  */
      CC_STATUS_INIT;
      return "or%.w %2,%0";
    }
#ifndef NeXT
  /* The bset insn is very slow on the '040.  It is pretty slow on the '030.
     It is best not to use it at all.  (We never use bclr or bchg, in any case) */
  if (GET_CODE (operands[2]) == CONST_INT
      && (logval = exact_log2 (INTVAL (operands[2]))) >= 0
      && (DATA_REG_P (operands[0])
	  || offsettable_memref_p (operands[0])))
    { 
      if (DATA_REG_P (operands[0]))
	{
	  operands[1] = gen_rtx (CONST_INT, VOIDmode, logval);
	}
      else
        {
	  operands[0] = adj_offsettable_operand (operands[0], 3 - (logval / 8));
	  operands[1] = gen_rtx (CONST_INT, VOIDmode, logval % 8);
	}
      return "bset %1,%0";
    }
#endif  /* NeXT */
  return "or%.l %2,%0";
}
}

static char *
output_141 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[2]) >> 16 == 0
      && (offsettable_memref_p (operands[0]) || DATA_REG_P (operands[0])))
    { 
      if (! DATA_REG_P (operands[0]))
	operands[0] = adj_offsettable_operand (operands[0], 2);
      /* Do not delete a following tstl %0 insn; that would be incorrect.  */
      CC_STATUS_INIT;
      return "eor%.w %2,%0";
    }
  return "eor%.l %2,%0";
}
}

static char *
output_149 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
	return "fsneg%.x %1,%0";
      return "fsneg%.s %f1,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
    return "fneg%.x %1,%0";
  return "fneg%.s %f1,%0";
}
}

static char *
output_152 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
	return "fdneg%.x %1,%0";
      return "fdneg%.d %f1,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
    return "fneg%.x %1,%0";
  return "fneg%.d %f1,%0";
}
}

static char *
output_155 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
	return "fsabs%.x %1,%0";
      return "fsabs%.s %f1,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
    return "fabs%.x %1,%0";
  return "fabs%.s %f1,%0";
}
}

static char *
output_158 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef NeXT
  if (TARGET_IEEE_MATH)
    {
      if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
	return "fdabs%.x %1,%0";
      return "fdabs%.d %f1,%0";
    }
#endif  /* NeXT */
  if (REG_P (operands[1]) && ! DATA_REG_P (operands[1]))
    return "fabs%.x %1,%0";
  return "fabs%.d %f1,%0";
}
}

static char *
output_162 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (TARGET_68020)
    return "move%.b %1,%0\n\textb%.l %0";
  return "move%.b %1,%0\n\text%.w %0\n\text%.l %0";
}
}

static char *
output_163 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (reg_mentioned_p (operands[0], operands[1]))
    return "move%.b %1,%0\n\tand%.l %#0xFF,%0";
  return "clr%.l %0\n\tmove%.b %1,%0";
}
}

static char *
output_164 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status.flags |= CC_REVERSED;
#ifdef HPUX_ASM
  return "cmp%.b %1,%0";
#else
  return "cmp%.b %0,%1";
#endif

}

static char *
output_165 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef HPUX_ASM
  return "cmp%.b %0,%1";
#else
  return "cmp%.b %1,%0";
#endif

}

static char *
output_166 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status.flags |= CC_REVERSED;
#ifdef HPUX_ASM
  return "cmp%.b %1,%0";
#else
  return "cmp%.b %0,%1";
#endif

}

static char *
output_167 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef HPUX_ASM
  return "cmp%.b %0,%1";
#else
  return "cmp%.b %1,%0";
#endif

}

static char *
output_186 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (REG_P (operands[0]))
    {
      if (INTVAL (operands[1]) + INTVAL (operands[2]) != 32)
        return "bfins %3,%0{%b2:%b1}";
    }
  else
    operands[0]
      = adj_offsettable_operand (operands[0], INTVAL (operands[2]) / 8);

  if (GET_CODE (operands[3]) == MEM)
    operands[3] = adj_offsettable_operand (operands[3],
					   (32 - INTVAL (operands[1])) / 8);
  if (INTVAL (operands[1]) == 8)
    return "move%.b %3,%0";
  return "move%.w %3,%0";
}
}

static char *
output_187 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (REG_P (operands[1]))
    {
      if (INTVAL (operands[2]) + INTVAL (operands[3]) != 32)
	return "bfextu %1{%b3:%b2},%0";
    }
  else
    operands[1]
      = adj_offsettable_operand (operands[1], INTVAL (operands[3]) / 8);

  output_asm_insn ("clr%.l %0", operands);
  if (GET_CODE (operands[0]) == MEM)
    operands[0] = adj_offsettable_operand (operands[0],
					   (32 - INTVAL (operands[1])) / 8);
  if (INTVAL (operands[2]) == 8)
    return "move%.b %1,%0";
  return "move%.w %1,%0";
}
}

static char *
output_188 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (REG_P (operands[1]))
    {
      if (INTVAL (operands[2]) + INTVAL (operands[3]) != 32)
	return "bfexts %1{%b3:%b2},%0";
    }
  else
    operands[1]
      = adj_offsettable_operand (operands[1], INTVAL (operands[3]) / 8);

  if (INTVAL (operands[2]) == 8)
    return "move%.b %1,%0\n\textb%.l %0";
  return "move%.w %1,%0\n\text%.l %0";
}
}

static char *
output_191 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  return "bfchg %0{%b2:%b1}";
}
}

static char *
output_192 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  return "bfclr %0{%b2:%b1}";
}
}

static char *
output_193 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  return "bfset %0{%b2:%b1}";
}
}

static char *
output_197 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  return "bfclr %0{%b2:%b1}";
}
}

static char *
output_198 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  return "bfset %0{%b2:%b1}";
}
}

static char *
output_199 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#if 0
  /* These special cases are now recognized by a specific pattern.  */
  if (GET_CODE (operands[1]) == CONST_INT && GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[1]) == 16 && INTVAL (operands[2]) == 16)
    return "move%.w %3,%0";
  if (GET_CODE (operands[1]) == CONST_INT && GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[1]) == 24 && INTVAL (operands[2]) == 8)
    return "move%.b %3,%0";
#endif
  return "bfins %3,%0{%b2:%b1}";
}
}

static char *
output_200 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_201 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_202 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_203 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_204 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_205 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (operands[1] == const1_rtx
      && GET_CODE (operands[2]) == CONST_INT)
    {    
      int width = GET_CODE (operands[0]) == REG ? 31 : 7;
      return output_btst (operands,
			  gen_rtx (CONST_INT, VOIDmode,
				   width - INTVAL (operands[2])),
			  operands[0],
			  insn, 1000);
      /* Pass 1000 as SIGNPOS argument so that btst will
         not think we are testing the sign bit for an `and'
	 and assume that nonzero implies a negative result.  */
    }
  if (INTVAL (operands[1]) != 32)
    cc_status.flags = CC_NOT_NEGATIVE;
  return "bftst %0{%b2:%b1}";
}
}

static char *
output_206 (operands, insn)
     rtx *operands;
     rtx insn;
{

  cc_status = cc_prev_status;
  OUTPUT_JUMP ("seq %0", "fseq %0", "seq %0");

}

static char *
output_207 (operands, insn)
     rtx *operands;
     rtx insn;
{

  cc_status = cc_prev_status;
  OUTPUT_JUMP ("sne %0", "fsne %0", "sne %0");

}

static char *
output_208 (operands, insn)
     rtx *operands;
     rtx insn;
{

  cc_status = cc_prev_status;
  OUTPUT_JUMP ("sgt %0", "fsgt %0", 0);

}

static char *
output_209 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     return "shi %0"; 
}

static char *
output_210 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     OUTPUT_JUMP ("slt %0", "fslt %0", "smi %0"); 
}

static char *
output_211 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     return "scs %0"; 
}

static char *
output_212 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     OUTPUT_JUMP ("sge %0", "fsge %0", "spl %0"); 
}

static char *
output_213 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     return "scc %0"; 
}

static char *
output_214 (operands, insn)
     rtx *operands;
     rtx insn;
{

  cc_status = cc_prev_status;
  OUTPUT_JUMP ("sle %0", "fsle %0", 0);

}

static char *
output_215 (operands, insn)
     rtx *operands;
     rtx insn;
{
 cc_status = cc_prev_status;
     return "sls %0"; 
}

static char *
output_216 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  OUTPUT_JUMP ("jbeq %l0", "fbeq %l0", "jbeq %l0");
#else
  OUTPUT_JUMP ("jeq %l0", "fjeq %l0", "jeq %l0");
#endif
}
}

static char *
output_217 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  OUTPUT_JUMP ("jbne %l0", "fbne %l0", "jbne %l0");
#else
  OUTPUT_JUMP ("jne %l0", "fjne %l0", "jne %l0");
#endif
}
}

static char *
output_218 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jbgt %l0", "fbgt %l0", 0);
#else
  OUTPUT_JUMP ("jgt %l0", "fjgt %l0", 0);
#endif

}

static char *
output_219 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbhi %l0";
#else
  return "jhi %l0";
#endif

}

static char *
output_220 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jblt %l0", "fblt %l0", "jbmi %l0");
#else
  OUTPUT_JUMP ("jlt %l0", "fjlt %l0", "jmi %l0");
#endif

}

static char *
output_221 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbcs %l0";
#else
  return "jcs %l0";
#endif

}

static char *
output_222 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jbge %l0", "fbge %l0", "jbpl %l0");
#else
  OUTPUT_JUMP ("jge %l0", "fjge %l0", "jpl %l0");
#endif

}

static char *
output_223 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbcc %l0";
#else
  return "jcc %l0";
#endif

}

static char *
output_224 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jble %l0", "fble %l0", 0);
#else
  OUTPUT_JUMP ("jle %l0", "fjle %l0", 0);
#endif

}

static char *
output_225 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbls %l0";
#else
  return "jls %l0";
#endif

}

static char *
output_226 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  OUTPUT_JUMP ("jbne %l0", "fbne %l0", "jbne %l0");
#else
  OUTPUT_JUMP ("jne %l0", "fjne %l0", "jne %l0");
#endif
}
}

static char *
output_227 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
#ifdef MOTOROLA
  OUTPUT_JUMP ("jbeq %l0", "fbeq %l0", "jbeq %l0");
#else
  OUTPUT_JUMP ("jeq %l0", "fjeq %l0", "jeq %l0");
#endif
}
}

static char *
output_228 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jble %l0", "fbngt %l0", 0);
#else
  OUTPUT_JUMP ("jle %l0", "fjngt %l0", 0);
#endif

}

static char *
output_229 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbls %l0";
#else
  return "jls %l0";
#endif

}

static char *
output_230 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jbge %l0", "fbnlt %l0", "jbpl %l0");
#else
  OUTPUT_JUMP ("jge %l0", "fjnlt %l0", "jpl %l0");
#endif

}

static char *
output_231 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbcc %l0";
#else
  return "jcc %l0";
#endif

}

static char *
output_232 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jblt %l0", "fbnge %l0", "jbmi %l0");
#else
  OUTPUT_JUMP ("jlt %l0", "fjnge %l0", "jmi %l0");
#endif

}

static char *
output_233 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbcs %l0";
#else
  return "jcs %l0";
#endif

}

static char *
output_234 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  OUTPUT_JUMP ("jbgt %l0", "fbnle %l0", 0);
#else
  OUTPUT_JUMP ("jgt %l0", "fjnle %l0", 0);
#endif

}

static char *
output_235 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbhi %l0";
#else
  return "jhi %l0";
#endif

}

static char *
output_239 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef SGS
#ifdef ASM_OUTPUT_CASE_LABEL
  return "jmp 6(%%pc,%0.w)";
#else
  return "jmp 2(%%pc,%0.w)";
#endif
#else /* not SGS */
#ifdef MOTOROLA
  return "jmp (2,pc,%0.w)";
#else
  return "jmp pc@(2,%0:w)";
#endif
#endif

}

static char *
output_240 (operands, insn)
     rtx *operands;
     rtx insn;
{

  operands[0] = gen_rtx (MEM, SImode, operands[0]);
  return "jmp %0";

}

static char *
output_241 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jbra %l0";
#else
  return "jra %l0";
#endif

}

static char *
output_242 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1";
  if (GET_CODE (operands[0]) == MEM)
    {
#ifdef MOTOROLA
#ifdef NO_ADDSUB_Q
      return "sub%.w %#1,%0\n\tjbcc %l1";
#else
      return "subq%.w %#1,%0\n\tjbcc %l1";
#endif
#else /* not MOTOROLA */
      return "subqw %#1,%0\n\tjcc %l1";
#endif
    }
#ifdef MOTOROLA
#ifdef HPUX_ASM
#ifndef NO_ADDSUB_Q
  return "sub%.w %#1,%0\n\tcmp%.w %0,%#-1\n\tjbne %l1";
#else
  return "subq%.w %#1,%0\n\tcmp%.w %0,%#-1\n\tjbne %l1";
#endif
#else /* not HPUX_ASM */
  return "subq%.w %#1,%0\n\tcmp%.w %#-1,%0\n\tjbne %l1";
#endif
#else /* not MOTOROLA */
  return "subqw %#1,%0\n\tcmpw %#-1,%0\n\tjne %l1";
#endif
}
}

static char *
output_243 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifndef NO_ADDSUB_Q
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr.w %0\n\tsub.l %#1,%0\n\tjbcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "sub.l %#1,%0\n\tjbcc %l1";
#else
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr.w %0\n\tsubq.l %#1,%0\n\tjbcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq.l %#1,%0\n\tjbcc %l1";
#endif /* not NO_ADDSUB_Q */
#ifdef HPUX_ASM
#ifndef NO_ADDSUB_Q
  return "sub.l %#1,%0\n\tcmp.l %0,%#-1\n\tjbne %l1";
#else
  return "subq.l %#1,%0\n\tcmp.l %0,%#-1\n\tjbne %l1";
#endif
#else
  return "subq.l %#1,%0\n\tcmp.l %#-1,%0\n\tjbne %l1";
#endif
#else
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclrw %0\n\tsubql %#1,%0\n\tjcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subql %#1,%0\n\tjcc %l1";
  return "subql %#1,%0\n\tcmpl %#-1,%0\n\tjne %l1";
#endif
}
}

static char *
output_244 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  CC_STATUS_INIT;
#ifdef MOTOROLA
#ifndef NO_ADDSUB_Q
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr.w %0\n\tsub.l %#1,%0\n\tjbcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "sub.l %#1,%0\n\tjbcc %l1";
#else
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr.w %0\n\tsubq.l %#1,%0\n\tjbcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq.l %#1,%0\n\tjbcc %l1";
#endif
#ifdef HPUX_ASM
#ifndef NO_ADDSUB_Q
  return "sub.l %#1,%0\n\tcmp.l %0,%#-1\n\tjbne %l1";
#else
  return "subq.l %#1,%0\n\tcmp.l %0,%#-1\n\tjbne %l1";
#endif
#else
  return "subq.l %#1,%0\n\tcmp.l %#-1,%0\n\tjbne %l1";
#endif
#else
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclrw %0\n\tsubql %#1,%0\n\tjcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subql %#1,%0\n\tjcc %l1";
  return "subql %#1,%0\n\tcmpl %#-1,%0\n\tjne %l1";
#endif
}
}

static char *
output_245 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jsr %0";
#else
  return "jbsr %0";
#endif

}

static char *
output_246 (operands, insn)
     rtx *operands;
     rtx insn;
{

#ifdef MOTOROLA
  return "jsr %1";
#else
  return "jbsr %1";
#endif

}

static char *
output_249 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  rtx xoperands[2];
  xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[1]) + 1);
  output_asm_insn ("move%.l %1,%@", xoperands);
  output_asm_insn ("move%.l %1,%-", operands);
  return "fmove%.d %+,%0";
}

}

static char *
output_250 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpma%.d %1,%w2,%w3,%0";
  return "fpma%.d %x1,%x2,%x3,%0";
}
}

static char *
output_251 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpma%.d %2,%w3,%w1,%0";
  return "fpma%.d %x2,%x3,%x1,%0";
}
}

static char *
output_252 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpma%.s %1,%w2,%w3,%0";
  return "fpma%.s %1,%2,%3,%0";
}
}

static char *
output_253 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpma%.s %2,%w3,%w1,%0";
  return "fpma%.s %2,%3,%1,%0";
}
}

static char *
output_254 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpms%.d %3,%w2,%w1,%0";
  return "fpms%.d %x3,%2,%x1,%0";
}
}

static char *
output_255 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpms%.s %3,%w2,%w1,%0";
  return "fpms%.s %3,%2,%1,%0";
}
}

static char *
output_256 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpmr%.d %2,%w1,%w3,%0";
  return "fpmr%.d %x2,%1,%x3,%0";
}
}

static char *
output_257 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpmr%.s %2,%w1,%w3,%0";
  return "fpmr%.s %x2,%1,%x3,%0";
}
}

static char *
output_258 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpam%.d %2,%w1,%w3,%0";
  return "fpam%.d %x2,%1,%x3,%0";
}
}

static char *
output_259 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpam%.d %3,%w2,%w1,%0";
  return "fpam%.d %x3,%2,%x1,%0";
}
}

static char *
output_260 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpam%.s %2,%w1,%w3,%0";
  return "fpam%.s %x2,%1,%x3,%0";
}
}

static char *
output_261 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpam%.s %3,%w2,%w1,%0";
  return "fpam%.s %x3,%2,%x1,%0";
}
}

static char *
output_262 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpsm%.d %2,%w1,%w3,%0";
  return "fpsm%.d %x2,%1,%x3,%0";
}
}

static char *
output_263 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpsm%.d %3,%w2,%w1,%0";
  return "fpsm%.d %x3,%2,%x1,%0";
}
}

static char *
output_264 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpsm%.s %2,%w1,%w3,%0";
  return "fpsm%.s %x2,%1,%x3,%0";
}
}

static char *
output_265 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (which_alternative == 0)
    return "fpsm%.s %3,%w2,%w1,%0";
  return "fpsm%.s %x3,%2,%x1,%0";
}
}

char * const insn_template[] =
  {
    0,
    0,
    0,
    0,
    "tst%.b %0",
    0,
    "fptst%.s %x0\n\tfpmove fpastatus,%1\n\tmovw %1,cc",
    0,
    0,
    "fptst%.d %x0\n\tfpmove fpastatus,%1\n\tmovw %1,cc",
    0,
    0,
    0,
    0,
    0,
    "fpcmp%.d %y1,%0\n\tfpmove fpastatus,%2\n\tmovw %2,cc",
    0,
    0,
    "fpcmp%.s %w1,%x0\n\tfpmove fpastatus,%2\n\tmovw %2,cc",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "pea %a1",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "ext%.w %0",
    "extb%.l %0",
    0,
    "fpstod %w1,%0",
    0,
    0,
    "fpdtos %y1,%0",
    0,
    "fmove%.s %f1,%0",
    0,
    "fpltos %1,%0",
    0,
    0,
    "fpltod %1,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "fmove%.b %1,%0",
    "fmove%.w %1,%0",
    "fmove%.l %1,%0",
    "fmove%.b %1,%0",
    "fmove%.w %1,%0",
    "fmove%.l %1,%0",
    "fpstol %w1,%0",
    "fpdtol %y1,%0",
    0,
    "add%.w %2,%0",
    0,
    "add%.w %1,%0",
    0,
    "add%.b %1,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "sub%.w %2,%0",
    "sub%.w %2,%0",
    "sub%.w %1,%0",
    "sub%.b %2,%0",
    "sub%.b %1,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "muls%.l %2,%0",
    0,
    0,
    "mulu%.l %2,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "divs%.l %2,%0",
    0,
    0,
    "divu%.l %2,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "divsl%.l %2,%3:%0",
    "divul%.l %2,%3:%0",
    0,
    "and%.w %2,%0",
    "and%.b %2,%0",
    "and%.w %1,%0",
    "and%.b %1,%0",
    0,
    "or%.w %2,%0",
    "or%.b %2,%0",
    0,
    "eor%.w %2,%0",
    "eor%.b %2,%0",
    "neg%.l %0",
    "neg%.w %0",
    "neg%.b %0",
    0,
    "fpneg%.s %w1,%0",
    0,
    0,
    "fpneg%.d %y1, %0",
    0,
    0,
    "fpabs%.s %y1,%0",
    0,
    0,
    "fpabs%.d %y1,%0",
    0,
    "not%.l %0",
    "not%.w %0",
    "not%.b %0",
    0,
    0,
    0,
    0,
    0,
    0,
    "asl%.l %2,%0",
    "asl%.w %2,%0",
    "asl%.b %2,%0",
    "asr%.l %2,%0",
    "asr%.w %2,%0",
    "asr%.b %2,%0",
    "lsl%.l %2,%0",
    "lsl%.w %2,%0",
    "lsl%.b %2,%0",
    "lsr%.l %2,%0",
    "lsr%.w %2,%0",
    "lsr%.b %2,%0",
    "rol%.l %2,%0",
    "rol%.w %2,%0",
    "rol%.b %2,%0",
    "ror%.l %2,%0",
    "ror%.w %2,%0",
    "ror%.b %2,%0",
    0,
    0,
    0,
    "bfexts %1{%b3:%b2},%0",
    "bfextu %1{%b3:%b2},%0",
    0,
    0,
    0,
    "bfins %3,%0{%b2:%b1}",
    "bfexts %1{%b3:%b2},%0",
    "bfextu %1{%b3:%b2},%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "nop",
    "lea %a1,%0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };

char *(*const insn_outfun[])() =
  {
    output_0,
    output_1,
    output_2,
    output_3,
    0,
    0,
    0,
    output_7,
    0,
    0,
    output_10,
    output_11,
    output_12,
    output_13,
    0,
    0,
    output_16,
    0,
    0,
    output_19,
    output_20,
    output_21,
    output_22,
    output_23,
    output_24,
    output_25,
    output_26,
    output_27,
    output_28,
    output_29,
    output_30,
    output_31,
    output_32,
    output_33,
    output_34,
    output_35,
    output_36,
    output_37,
    output_38,
    0,
    output_40,
    output_41,
    output_42,
    0,
    0,
    0,
    output_46,
    output_47,
    output_48,
    output_49,
    0,
    0,
    0,
    0,
    output_54,
    0,
    0,
    output_57,
    0,
    0,
    0,
    output_61,
    0,
    0,
    output_64,
    output_65,
    output_66,
    output_67,
    output_68,
    output_69,
    output_70,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_79,
    0,
    output_81,
    0,
    output_83,
    0,
    0,
    output_86,
    output_87,
    0,
    output_89,
    output_90,
    output_91,
    0,
    0,
    0,
    0,
    0,
    0,
    output_98,
    output_99,
    0,
    output_101,
    output_102,
    output_103,
    output_104,
    0,
    output_106,
    output_107,
    0,
    0,
    output_110,
    output_111,
    0,
    output_113,
    output_114,
    output_115,
    output_116,
    0,
    output_118,
    output_119,
    0,
    0,
    output_122,
    output_123,
    0,
    output_125,
    output_126,
    output_127,
    output_128,
    output_129,
    output_130,
    0,
    0,
    output_133,
    0,
    0,
    0,
    0,
    output_138,
    0,
    0,
    output_141,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_149,
    0,
    0,
    output_152,
    0,
    0,
    output_155,
    0,
    0,
    output_158,
    0,
    0,
    0,
    output_162,
    output_163,
    output_164,
    output_165,
    output_166,
    output_167,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_186,
    output_187,
    output_188,
    0,
    0,
    output_191,
    output_192,
    output_193,
    0,
    0,
    0,
    output_197,
    output_198,
    output_199,
    output_200,
    output_201,
    output_202,
    output_203,
    output_204,
    output_205,
    output_206,
    output_207,
    output_208,
    output_209,
    output_210,
    output_211,
    output_212,
    output_213,
    output_214,
    output_215,
    output_216,
    output_217,
    output_218,
    output_219,
    output_220,
    output_221,
    output_222,
    output_223,
    output_224,
    output_225,
    output_226,
    output_227,
    output_228,
    output_229,
    output_230,
    output_231,
    output_232,
    output_233,
    output_234,
    output_235,
    0,
    0,
    0,
    output_239,
    output_240,
    output_241,
    output_242,
    output_243,
    output_244,
    output_245,
    output_246,
    0,
    0,
    output_249,
    output_250,
    output_251,
    output_252,
    output_253,
    output_254,
    output_255,
    output_256,
    output_257,
    output_258,
    output_259,
    output_260,
    output_261,
    output_262,
    output_263,
    output_264,
    output_265,
  };

rtx (*const insn_gen_function[]) () =
  {
    0,
    0,
    gen_tstsi,
    gen_tsthi,
    gen_tstqi,
    gen_tstsf,
    0,
    0,
    gen_tstdf,
    0,
    0,
    gen_cmpsi,
    gen_cmphi,
    gen_cmpqi,
    gen_cmpdf,
    0,
    0,
    gen_cmpsf,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_movsi,
    gen_movhi,
    gen_movstricthi,
    gen_movqi,
    gen_movstrictqi,
    gen_movsf,
    gen_movdf,
    gen_movdi,
    gen_pushasi,
    gen_truncsiqi2,
    gen_trunchiqi2,
    gen_truncsihi2,
    gen_zero_extendhisi2,
    gen_zero_extendqihi2,
    gen_zero_extendqisi2,
    0,
    0,
    0,
    gen_extendhisi2,
    gen_extendqihi2,
    gen_extendqisi2,
    gen_extendsfdf2,
    0,
    0,
    gen_truncdfsf2,
    0,
    0,
    0,
    gen_floatsisf2,
    0,
    0,
    gen_floatsidf2,
    0,
    0,
    gen_floathisf2,
    gen_floathidf2,
    gen_floatqisf2,
    gen_floatqidf2,
    gen_ftruncdf2,
    gen_ftruncsf2,
    gen_fixsfqi2,
    gen_fixsfhi2,
    gen_fixsfsi2,
    gen_fixdfqi2,
    gen_fixdfhi2,
    gen_fixdfsi2,
    gen_fix_truncsfsi2,
    gen_fix_truncdfsi2,
    gen_addsi3,
    0,
    gen_addhi3,
    0,
    gen_addqi3,
    0,
    gen_adddf3,
    0,
    0,
    gen_addsf3,
    0,
    0,
    gen_subsi3,
    0,
    gen_subhi3,
    0,
    gen_subqi3,
    0,
    gen_subdf3,
    0,
    0,
    gen_subsf3,
    0,
    0,
    gen_mulhi3,
    gen_mulhisi3,
    gen_mulsi3,
    gen_umulhi3,
    gen_umulhisi3,
    gen_umulsi3,
    gen_muldf3,
    0,
    0,
    gen_mulsf3,
    0,
    0,
    gen_divhi3,
    gen_divhisi3,
    gen_divsi3,
    gen_udivhi3,
    gen_udivhisi3,
    gen_udivsi3,
    gen_divdf3,
    0,
    0,
    gen_divsf3,
    0,
    0,
    gen_modhi3,
    gen_modhisi3,
    gen_umodhi3,
    gen_umodhisi3,
    gen_divmodsi4,
    gen_udivmodsi4,
    gen_andsi3,
    gen_andhi3,
    gen_andqi3,
    0,
    0,
    gen_iorsi3,
    gen_iorhi3,
    gen_iorqi3,
    gen_xorsi3,
    gen_xorhi3,
    gen_xorqi3,
    gen_negsi2,
    gen_neghi2,
    gen_negqi2,
    gen_negsf2,
    0,
    0,
    gen_negdf2,
    0,
    0,
    gen_abssf2,
    0,
    0,
    gen_absdf2,
    0,
    0,
    gen_one_cmplsi2,
    gen_one_cmplhi2,
    gen_one_cmplqi2,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_ashlsi3,
    gen_ashlhi3,
    gen_ashlqi3,
    gen_ashrsi3,
    gen_ashrhi3,
    gen_ashrqi3,
    gen_lshlsi3,
    gen_lshlhi3,
    gen_lshlqi3,
    gen_lshrsi3,
    gen_lshrhi3,
    gen_lshrqi3,
    gen_rotlsi3,
    gen_rotlhi3,
    gen_rotlqi3,
    gen_rotrsi3,
    gen_rotrhi3,
    gen_rotrqi3,
    0,
    0,
    0,
    gen_extv,
    gen_extzv,
    0,
    0,
    0,
    gen_insv,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_seq,
    gen_sne,
    gen_sgt,
    gen_sgtu,
    gen_slt,
    gen_sltu,
    gen_sge,
    gen_sgeu,
    gen_sle,
    gen_sleu,
    gen_beq,
    gen_bne,
    gen_bgt,
    gen_bgtu,
    gen_blt,
    gen_bltu,
    gen_bge,
    gen_bgeu,
    gen_ble,
    gen_bleu,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_casesi_1,
    gen_casesi_2,
    gen_unused,
    0,
    gen_tablejump,
    gen_jump,
    0,
    0,
    0,
    gen_call,
    gen_call_value,
    gen_nop,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };

const int insn_n_operands[] =
  {
    2,
    2,
    1,
    1,
    1,
    1,
    2,
    1,
    1,
    2,
    1,
    2,
    2,
    2,
    2,
    3,
    2,
    2,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    2,
    2,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    2,
    3,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    3,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    4,
    4,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    4,
    4,
    4,
    4,
    4,
    4,
    3,
    3,
    4,
    4,
    4,
    3,
    3,
    4,
    3,
    3,
    3,
    3,
    3,
    3,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    4,
    2,
    5,
    1,
    1,
    0,
    1,
    1,
    1,
    2,
    3,
    0,
    2,
    2,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
  };

const int insn_n_dups[] =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    3,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };

char *const insn_operand_constraint[][MAX_RECOG_OPERANDS] =
  {
    { "=m", "ro<>fyF", },
    { "=m", "ro<>Fy", },
    { "rm", },
    { "rm", },
    { "dm", },
    { "", },
    { "xmdF", "d", },
    { "fdm", },
    { "", },
    { "xrmF", "d", },
    { "fm", },
    { "rKs,mr,>", "mr,Ksr,>", },
    { "rnm,d,n,m", "d,rnm,m,n", },
    { "dn,md,>", "dm,nd,>", },
    { "", "", },
    { "x,y", "xH,rmF", "d,d", },
    { "f,mG", "fmG,f", },
    { "", "", },
    { "x,y", "xH,rmF", "d,d", },
    { "f,mdG", "fmdG,f", },
    { "do", "di", },
    { "d", "di", },
    { "do", "d", },
    { "d", "d", },
    { "md", "i", },
    { "o,d", "i,i", },
    { "do", "i", },
    { "dm", },
    { "dm", "i", },
    { "=m", "J", },
    { "=g", },
    { "=g,da,y,!*x*r*m", "daymKs,i,g,*x*r*m", },
    { "=g", "g", },
    { "+dm", "rmn", },
    { "=d,*a,m,m,?*a", "dmi*a,d*a,dmi,?*a,m", },
    { "+dm", "dmn", },
    { "=rmf,x,y,rm,!x,!rm", "rmfF,xH,rmF,y,rm,x", },
    { "=rm,&rf,&rof<>,y,rm,x,!x,!rm", "rf,m,rofF<>,rmF,y,xH,rm,x", },
    { "=rm,&r,&ro<>,y,rm,!*x,!rm", "rF,m,roi<>F,rmiF,y,rmF,*x", },
    { "=m", "p", },
    { "=dm,d", "doJ,i", },
    { "=dm,d", "doJ,i", },
    { "=dm,d", "roJ,i", },
    { "", "", },
    { "", "", },
    { "", "", },
    { "=do<>", "rmn", },
    { "=do<>", "dmn", },
    { "=do<>", "dmn", },
    { "=*d,a", "0,rmn", },
    { "=d", "0", },
    { "=d", "0", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=*fdm,f", "f,dmF", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=f", "fmG", },
    { "=dm", "f", },
    { "", "", },
    { "=y,x", "rmi,x", },
    { "=f", "dmi", },
    { "", "", },
    { "=y,x", "rmi,x", },
    { "=f", "dmi", },
    { "=f", "dmn", },
    { "=f", "dmn", },
    { "=f", "dmn", },
    { "=f", "dmn", },
    { "=f", "fFm", },
    { "=f", "dfFm", },
    { "=dm", "f", },
    { "=dm", "f", },
    { "=dm", "f", },
    { "=dm", "f", },
    { "=dm", "f", },
    { "=dm", "f", },
    { "=x,y", "xH,rmF", },
    { "=x,y", "xH,rmF", },
    { "=m,r,!a,!a", "%0,0,a,rJK", "dIKLs,mrIKLs,rJK,a", },
    { "=a", "0", "rmn", },
    { "=m,r", "%0,0", "dn,rmn", },
    { "+m,d", "dn,rmn", },
    { "=m,d", "%0,0", "dn,dmn", },
    { "+m,d", "dn,dmn", },
    { "", "", "", },
    { "=x,y", "%xH,y", "xH,dmF", },
    { "=f", "%0", "fmG", },
    { "", "", "", },
    { "=x,y", "%xH,y", "xH,rmF", },
    { "=f", "%0", "fdmF", },
    { "=m,r,!a,?d", "0,0,a,mrIKs", "dIKs,mrIKs,J,0", },
    { "=a", "0", "rmn", },
    { "=m,r", "0,0", "dn,rmn", },
    { "+m,d", "dn,rmn", },
    { "=m,d", "0,0", "dn,dmn", },
    { "+m,d", "dn,dmn", },
    { "", "", "", },
    { "=x,y,y", "xH,y,dmF", "xH,dmF,0", },
    { "=f", "0", "fmG", },
    { "", "", "", },
    { "=x,y,y", "xH,y,rmF", "xH,rmF,0", },
    { "=f", "0", "fdmF", },
    { "=d", "%0", "dmn", },
    { "=d", "%0", "dmn", },
    { "=d", "%0", "dmsK", },
    { "=d", "%0", "dmn", },
    { "=d", "%0", "dmn", },
    { "=d", "%0", "dmsK", },
    { "", "", "", },
    { "=x,y", "%xH,y", "xH,rmF", },
    { "=f", "%0", "fmG", },
    { "", "", "", },
    { "=x,y", "%xH,y", "xH,rmF", },
    { "=f", "%0", "fdmF", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmsK", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmsK", },
    { "", "", "", },
    { "=x,y,y", "xH,y,rmF", "xH,rmF,0", },
    { "=f", "0", "fmG", },
    { "", "", "", },
    { "=x,y,y", "xH,y,rmF", "xH,rmF,0", },
    { "=f", "0", "fdmF", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmn", },
    { "=d", "0", "dmsK", "=d", },
    { "=d", "0", "dmsK", "=d", },
    { "=m,d", "%0,0", "dKs,dmKs", },
    { "=m,d", "%0,0", "dn,dmn", },
    { "=m,d", "%0,0", "dn,dmn", },
    { "=d", "dm", "0", },
    { "=d", "dm", "0", },
    { "=m,d", "%0,0", "dKs,dmKs", },
    { "=m,d", "%0,0", "dn,dmn", },
    { "=m,d", "%0,0", "dn,dmn", },
    { "=do,m", "%0,0", "di,dKs", },
    { "=dm", "%0", "dn", },
    { "=dm", "%0", "dn", },
    { "=dm", "0", },
    { "=dm", "0", },
    { "=dm", "0", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=f", "fdmF", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=f", "fmF", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=f", "fdmF", },
    { "", "", },
    { "=x,y", "xH,rmF", },
    { "=f", "fmF", },
    { "=dm", "0", },
    { "=dm", "0", },
    { "=dm", "0", },
    { "=d", "m", },
    { "=d", "m", },
    { "i", "m", },
    { "m", "i", },
    { "i", "m", },
    { "m", "i", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "=d", "0", "dI", },
    { "+do", "i", "i", "d", },
    { "=&d", "do", "i", "i", },
    { "=d", "do", "i", "i", },
    { "=d,d", "o,d", "di,di", "di,di", },
    { "=d,d", "o,d", "di,di", "di,di", },
    { "+o,d", "di,di", "di,di", "i,i", },
    { "+o,d", "di,di", "di,di", },
    { "+o,d", "di,di", "di,di", },
    { "+o,d", "di,di", "di,di", "d,d", },
    { "=d", "d", "di", "di", },
    { "=d", "d", "di", "di", },
    { "+d", "di", "di", },
    { "+d", "di", "di", },
    { "+d", "di", "di", "d", },
    { "o", "di", "di", },
    { "o", "di", "di", },
    { "o", "di", "di", },
    { "d", "di", "di", },
    { "d", "di", "di", },
    { "d", "di", "di", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { "=d", },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { "", "", "", "", },
    { "", "", },
    { "", "", "", "", "", },
    { "r", },
    { "a", },
    { 0 },
    { "g", },
    { "g", },
    { "g", },
    { "o", "g", },
    { "=rf", "o", "g", },
    { 0 },
    { "=a", "p", },
    { "f", "ad", },
    { "=x,y,y", "%x,dmF,y", "xH,y,y", "xH,y,dmF", },
    { "=x,y,y", "xH,y,dmF", "%x,dmF,y", "xH,y,y", },
    { "=x,y,y", "%x,ydmF,y", "xH,y,ydmF", "xH,ydmF,ydmF", },
    { "=x,y,y", "xH,ydmF,ydmF", "%x,ydmF,y", "xH,y,ydmF", },
    { "=x,y,y", "xH,rmF,y", "%xH,y,y", "x,y,rmF", },
    { "=x,y,y", "xH,rmF,yrmF", "%xH,rmF,y", "x,y,yrmF", },
    { "=x,y,y", "%xH,y,y", "x,y,rmF", "xH,rmF,y", },
    { "=x,y,y", "%xH,rmF,y", "x,y,yrmF", "xH,rmF,yrmF", },
    { "=x,y,y", "%xH,y,y", "x,y,rmF", "xH,rmF,y", },
    { "=x,y,y", "xH,rmF,y", "%xH,y,y", "x,y,rmF", },
    { "=x,y,y", "%xH,rmF,y", "x,y,yrmF", "xH,rmF,yrmF", },
    { "=x,y,y", "xH,rmF,yrmF", "%xH,rmF,y", "x,y,yrmF", },
    { "=x,y,y", "xH,y,y", "x,y,rmF", "xH,rmF,y", },
    { "=x,y,y", "xH,rmF,y", "xH,y,y", "x,y,rmF", },
    { "=x,y,y", "xH,rmF,y", "x,y,yrmF", "xH,rmF,yrmF", },
    { "=x,y,y", "xH,rmF,yrmF", "xH,rmF,y", "x,y,yrmF", },
  };

const enum machine_mode insn_operand_mode[][MAX_RECOG_OPERANDS] =
  {
    { DFmode, DFmode, },
    { DImode, DImode, },
    { SImode, },
    { HImode, },
    { QImode, },
    { SFmode, },
    { SFmode, SImode, },
    { SFmode, },
    { DFmode, },
    { DFmode, SImode, },
    { DFmode, },
    { SImode, SImode, },
    { HImode, HImode, },
    { QImode, QImode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, SImode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, SImode, },
    { SFmode, SFmode, },
    { QImode, SImode, },
    { SImode, SImode, },
    { QImode, SImode, },
    { SImode, SImode, },
    { QImode, SImode, },
    { HImode, SImode, },
    { SImode, SImode, },
    { QImode, },
    { QImode, SImode, },
    { SImode, SImode, },
    { SImode, },
    { SImode, SImode, },
    { HImode, HImode, },
    { HImode, HImode, },
    { QImode, QImode, },
    { QImode, QImode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { DImode, DImode, },
    { SImode, SImode, },
    { QImode, SImode, },
    { QImode, HImode, },
    { HImode, SImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { DFmode, SFmode, },
    { DFmode, SFmode, },
    { DFmode, SFmode, },
    { SFmode, DFmode, },
    { SFmode, DFmode, },
    { SFmode, DFmode, },
    { SFmode, DFmode, },
    { SFmode, SImode, },
    { SFmode, SImode, },
    { SFmode, SImode, },
    { DFmode, SImode, },
    { DFmode, SImode, },
    { DFmode, SImode, },
    { SFmode, HImode, },
    { DFmode, HImode, },
    { SFmode, QImode, },
    { DFmode, QImode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { QImode, SFmode, },
    { HImode, SFmode, },
    { SImode, SFmode, },
    { QImode, DFmode, },
    { HImode, DFmode, },
    { SImode, DFmode, },
    { SImode, SFmode, },
    { SImode, DFmode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, HImode, },
    { HImode, HImode, HImode, },
    { HImode, HImode, },
    { QImode, QImode, QImode, },
    { QImode, QImode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, HImode, },
    { HImode, HImode, HImode, },
    { HImode, HImode, },
    { QImode, QImode, QImode, },
    { QImode, QImode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { HImode, HImode, HImode, },
    { SImode, HImode, HImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { SImode, HImode, HImode, },
    { SImode, SImode, SImode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { HImode, HImode, HImode, },
    { HImode, SImode, HImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { HImode, SImode, HImode, },
    { SImode, SImode, SImode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, },
    { HImode, HImode, HImode, },
    { HImode, SImode, HImode, },
    { HImode, HImode, HImode, },
    { HImode, SImode, HImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, HImode, SImode, },
    { SImode, QImode, SImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, },
    { HImode, HImode, },
    { QImode, QImode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { SImode, SImode, },
    { HImode, HImode, },
    { QImode, QImode, },
    { SImode, SImode, },
    { SImode, SImode, },
    { QImode, SImode, },
    { SImode, QImode, },
    { QImode, SImode, },
    { SImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, },
    { HImode, HImode, HImode, },
    { QImode, QImode, QImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, QImode, SImode, SImode, },
    { SImode, QImode, SImode, SImode, },
    { QImode, SImode, SImode, VOIDmode, },
    { QImode, SImode, SImode, },
    { QImode, SImode, SImode, },
    { QImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, SImode, },
    { QImode, SImode, SImode, },
    { QImode, SImode, SImode, },
    { QImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { QImode, },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { SImode, SImode, SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, SImode, VOIDmode, VOIDmode, },
    { HImode, },
    { SImode, },
    { VOIDmode },
    { HImode, },
    { SImode, },
    { SImode, },
    { QImode, SImode, },
    { VOIDmode, QImode, SImode, },
    { VOIDmode },
    { SImode, QImode, },
    { VOIDmode, VOIDmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { DFmode, DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
    { SFmode, SFmode, SFmode, SFmode, },
  };

const char insn_operand_strict_low[][MAX_RECOG_OPERANDS] =
  {
    { 0, 0, },
    { 0, 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, 0, },
    { 0, },
    { 0, },
    { 0, 0, },
    { 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, },
    { 0, 0, },
    { 0, 0, },
    { 1, 0, },
    { 0, 0, },
    { 1, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 1, 0, },
    { 0, 0, 0, },
    { 1, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 1, 0, },
    { 0, 0, 0, },
    { 1, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, },
    { 0, },
    { 0 },
    { 0, },
    { 0, },
    { 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0 },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
  };

int (*const insn_operand_predicate[][MAX_RECOG_OPERANDS])() =
  {
    { push_operand, general_operand, },
    { push_operand, general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, general_operand, },
    { general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, general_operand, },
    { nonimmediate_operand, },
    { nonimmediate_operand, general_operand, },
    { push_operand, general_operand, },
    { general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { push_operand, address_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, general_operand, },
    { register_operand, general_operand, },
    { register_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, memory_operand, },
    { general_operand, memory_operand, },
    { general_operand, memory_operand, },
    { memory_operand, general_operand, },
    { general_operand, memory_operand, },
    { memory_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { general_operand, general_operand, general_operand, },
    { nonimmediate_operand, immediate_operand, immediate_operand, general_operand, },
    { general_operand, nonimmediate_operand, immediate_operand, immediate_operand, },
    { general_operand, nonimmediate_operand, immediate_operand, immediate_operand, },
    { general_operand, nonimmediate_operand, general_operand, general_operand, },
    { general_operand, nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, immediate_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, general_operand, },
    { general_operand, nonimmediate_operand, general_operand, general_operand, },
    { general_operand, nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, general_operand, },
    { memory_operand, general_operand, general_operand, },
    { memory_operand, general_operand, general_operand, },
    { memory_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { nonimmediate_operand, general_operand, general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { general_operand, immediate_operand, general_operand, general_operand, },
    { 0, 0, },
    { general_operand, immediate_operand, general_operand, 0, 0, },
    { general_operand, },
    { register_operand, },
    { 0 },
    { general_operand, },
    { general_operand, },
    { general_operand, },
    { general_operand, general_operand, },
    { 0, general_operand, general_operand, },
    { 0 },
    { general_operand, address_operand, },
    { 0, 0, },
    { register_operand, general_operand, general_operand, general_operand, },
    { register_operand, general_operand, general_operand, general_operand, },
    { register_operand, general_operand, general_operand, general_operand, },
    { register_operand, general_operand, general_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
    { register_operand, register_operand, general_operand, general_operand, },
    { register_operand, general_operand, register_operand, general_operand, },
  };

#ifndef DEFAULT_MACHINE_INFO
#define DEFAULT_MACHINE_INFO 0
#endif

const INSN_MACHINE_INFO insn_machine_info[] =
  {
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
     { DEFAULT_MACHINE_INFO },
  };

const int insn_n_alternatives[] =
  {
    1,
    1,
    1,
    1,
    1,
     0,
    1,
    1,
     0,
    1,
    1,
    3,
    4,
    3,
     0,
    2,
    2,
     0,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    2,
    1,
    1,
    1,
    1,
    1,
    4,
    1,
    1,
    5,
    1,
    6,
    8,
    7,
    1,
    2,
    2,
    2,
     0,
     0,
     0,
    1,
    1,
    1,
    2,
    1,
    1,
     0,
    2,
    2,
     0,
    2,
    1,
    1,
     0,
    2,
    1,
     0,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    4,
    1,
    2,
    2,
    2,
    2,
     0,
    2,
    1,
     0,
    2,
    1,
    4,
    1,
    2,
    2,
    2,
    2,
     0,
    3,
    1,
     0,
    3,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
     0,
    2,
    1,
     0,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
     0,
    3,
    1,
     0,
    3,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    1,
    1,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
     0,
    2,
    1,
     0,
    2,
    1,
     0,
    2,
    1,
     0,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
    1,
    1,
     0,
    1,
    1,
    1,
    1,
    1,
     0,
    1,
    1,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
  };
