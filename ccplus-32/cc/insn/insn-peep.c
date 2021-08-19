/* Generated automatically by the program `genpeep'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "regs.h"
#include "real.h"

extern rtx peep_operand[];

#define operands peep_operand

rtx
peephole (ins1)
     rtx ins1;
{
  rtx insn, x, pat;
  int i;

  if (NEXT_INSN (ins1)
      && GET_CODE (NEXT_INSN (ins1)) == BARRIER)
    return 0;

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L249;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != REG) goto L249;
  if (GET_MODE (x) != SImode) goto L249;
  if (XINT (x, 0) != 15) goto L249;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != PLUS) goto L249;
  if (GET_MODE (x) != SImode) goto L249;
  x = XEXP (XEXP (pat, 1), 0);
  if (GET_CODE (x) != REG) goto L249;
  if (GET_MODE (x) != SImode) goto L249;
  if (XINT (x, 0) != 15) goto L249;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L249;
  if (XINT (x, 0) != 4) goto L249;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L249; }
  while (GET_CODE (insn) == NOTE);
  if (GET_CODE (insn) == CODE_LABEL
      || GET_CODE (insn) == BARRIER)
    goto L249;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L249;
  x = XEXP (pat, 0);
  operands[0] = x;
  if (! register_operand (x, DFmode)) goto L249;
  x = XEXP (pat, 1);
  operands[1] = x;
  if (! register_operand (x, DFmode)) goto L249;
  if (! (FP_REG_P (operands[0]) && ! FP_REG_P (operands[1]))) goto L249;
  PATTERN (ins1) = gen_rtx (PARALLEL, VOIDmode, gen_rtvec_v (2, operands));
  INSN_CODE (ins1) = 249;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L249:

  return 0;
}

rtx peep_operand[2];
