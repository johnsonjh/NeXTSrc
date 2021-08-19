;; Machine description for Pyramid 90 Series for GNU C compiler
;; Copyright (C) 1989 Free Software Foundation, Inc.

;; This file is part of GNU CC.

;; GNU CC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 1, or (at your option)
;; any later version.

;; GNU CC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU CC; see the file COPYING.  If not, write to
;; the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

;; Instruction patterns.  When multiple patterns apply,
;; the first one in the file is chosen.
;;
;; See file "rtl.def" for documentation on define_insn, match_*, et. al.
;;
;; cpp macro #define NOTICE_UPDATE_CC in file tm.h handles condition code
;; updates for most instructions.

;; * Should we check for SUBREG as well as REG, in some places???
;; * Make the jump tables contain branches, not addresses!  This would
;;   save us one instruction.
;; * Could the compilcated scheme for compares be simplyfied, if we had
;;   no named cmpqi or cmphi patterns, and instead anonymous patterns for
;;   the less-than-word compare cases pyr can handle???
;; * The jump insn seems to accept more than just IR addressing.  Would
;;   we win by telling GCC?
;; * More DImode patterns.
;; * Expansion of "cmpsi" is not necessary.  Also, some define_insn could
;;   be taken away if constraints were used.
;; * Scan backwards in "zero_extendhisi2", "zero_extendqisi2" to find out
;;   if the extension can be omitted.  Also improve the scan (now in the sign
;;   extenstion patterns), to handle more cases than extension regx->regx.
;; * Enhance NOTICE_UPDATE_CC.  1) Should not be reset by insn that don't
;;   modify cc.  2) cc is preserved by CALL.
;; * "divmodsi" with Pyramid "ediv" insn.  Is it possible in rtl??
;; * Would "rcsp tmpreg; u?cmp[bh] op1_regdispl(tmpreg),op2" win in
;;   comparison with the two extensions and single test generated now?
;;   The rcsp insn could be expanded, and moved out of loops by the
;;   optimizer, making 1 (64 bit) insn of 3 (32 bit) insns in loops.
;;   The rcsp insn could be followed by an add insn, making non-displacement
;;   IR addressing sufficient.

;______________________________________________________________________
;
;	Test and Compare Patterns.
;______________________________________________________________________

(define_expand "cmpsi"
  [(set (cc0)
	(compare (match_operand:SI 0 "general_operand" "")
		 (match_operand:SI 1 "general_operand" "")))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  test_op1 = copy_rtx (operands[1]);
  DONE;
}")

(define_expand "tstsi"
  [(set (cc0)
	(match_operand:SI 0 "general_operand" ""))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  DONE;
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:SI 0 "memory_operand" "m")
		 (match_operand:SI 1 "memory_operand" "m")))]
  "weird_memory_memory (operands[0], operands[1])"
  "*
{
  rtx br_insn = NEXT_INSN (insn);
  RTX_CODE br_code;

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();
  br_code =  GET_CODE (XEXP (XEXP (PATTERN (br_insn), 1), 0));

  weird_memory_memory (operands[0], operands[1]);

  if (swap_operands == 0)
    return (br_code >= GEU && br_code <= LTU
	    ? \"ucmpw %1,%0\" : \"cmpw %1,%0\");
  else
    {
      cc_status.flags |= CC_REVERSED;
      return (br_code >= GEU && br_code <= LTU
	      ? \"ucmpw %0,%1\" : \"cmpw %0,%1\");
    }
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:SI 0 "general_operand" "r,g")
		 (match_operand:SI 1 "general_operand" "g,r")))]
  ""
  "*
{
  rtx br_insn = NEXT_INSN (insn);
  RTX_CODE br_code;

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();
  br_code =  GET_CODE (XEXP (XEXP (PATTERN (br_insn), 1), 0));

  if (which_alternative == 0)
    return (br_code >= GEU && br_code <= LTU)
      ? \"ucmpw %1,%0\" : \"cmpw %1,%0\";
  else
    {
      cc_status.flags |= CC_REVERSED;
      return (br_code >= GEU && br_code <= LTU)
	? \"ucmpw %0,%1\" : \"cmpw %0,%1\";
    }
}")

(define_insn ""
  [(set (cc0)
	(match_operand:SI 0 "register_operand" "r"))]
  ""
  "mtstw %0,%0")

(define_expand "cmphi"
  [(set (cc0)
	(compare (match_operand:HI 0 "general_operand" "")
		 (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  test_op1 = copy_rtx (operands[1]);
  DONE;
}")

(define_expand "tsthi"
  [(set (cc0)
	(match_operand:HI 0 "general_operand" ""))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  DONE;
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:HI 0 "memory_operand" "m")
		 (match_operand:HI 1 "memory_operand" "m")))]
  "weird_memory_memory (operands[0], operands[1])"
  "*
{
  rtx br_insn = NEXT_INSN (insn);

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();

  weird_memory_memory (operands[0], operands[1]);

  if (swap_operands == 0)
    return \"cmph %1,%0\";
  else
    {
      cc_status.flags |= CC_REVERSED;
      return \"cmph %0,%1\";
    }
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:HI 0 "nonimmediate_operand" "r,m")
		 (match_operand:HI 1 "nonimmediate_operand" "m,r")))]
  ""
  "*
{
  rtx br_insn = NEXT_INSN (insn);

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();

  if (which_alternative == 0)
    return \"cmph %1,%0\";
  else
    {
      cc_status.flags |= CC_REVERSED;
      return \"cmph %0,%1\";
    }
}")

(define_expand "cmpqi"
  [(set (cc0)
	(compare (match_operand:QI 0 "general_operand" "")
		 (match_operand:QI 1 "general_operand" "")))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  test_op1 = copy_rtx (operands[1]);
  DONE;
}")

(define_expand "tstqi"
  [(set (cc0)
	(match_operand:QI 0 "general_operand" ""))]
  ""
  "
{
  extern rtx test_op0, test_op1;
  test_op0 = copy_rtx (operands[0]);
  DONE;
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:QI 0 "memory_operand" "m")
		 (match_operand:QI 1 "memory_operand" "m")))]
  "weird_memory_memory (operands[0], operands[1])"
  "*
{
  rtx br_insn = NEXT_INSN (insn);
  RTX_CODE br_code;

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();
  br_code =  GET_CODE (XEXP (XEXP (PATTERN (br_insn), 1), 0));

  weird_memory_memory (operands[0], operands[1]);

  if (swap_operands == 0)
    return (br_code >= GEU && br_code <= LTU
	    ? \"ucmpb %1,%0\" : \"cmpb %1,%0\");
  else
    {
      cc_status.flags |= CC_REVERSED;
      return (br_code >= GEU && br_code <= LTU
	      ? \"ucmpb %0,%1\" : \"cmpb %0,%1\");
    }
}")

(define_insn ""
  [(set (cc0)
	(compare (match_operand:QI 0 "nonimmediate_operand" "r,m")
		 (match_operand:QI 1 "nonimmediate_operand" "m,r")))]
  ""
  "*
{
  rtx br_insn = NEXT_INSN (insn);
  RTX_CODE br_code;

  if (GET_CODE (br_insn) != JUMP_INSN)
    abort();
  br_code =  GET_CODE (XEXP (XEXP (PATTERN (br_insn), 1), 0));

  if (which_alternative == 0)
    return (br_code >= GEU && br_code <= LTU)
      ? \"ucmpb %1,%0\" : \"cmpb %1,%0\";
  else
    {
      cc_status.flags |= CC_REVERSED;
      return (br_code >= GEU && br_code <= LTU)
	? \"ucmpb %0,%1\" : \"cmpb %0,%1\";
    }
}")

(define_expand "bgt"
  [(set (pc) (if_then_else (gt (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "blt"
  [(set (pc) (if_then_else (lt (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "bge"
  [(set (pc) (if_then_else (ge (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "ble"
  [(set (pc) (if_then_else (le (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "beq"
  [(set (pc) (if_then_else (eq (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "bne"
  [(set (pc) (if_then_else (ne (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (SIGN_EXTEND);")

(define_expand "bgtu"
  [(set (pc) (if_then_else (gtu (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (ZERO_EXTEND);")

(define_expand "bltu"
  [(set (pc) (if_then_else (ltu (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (ZERO_EXTEND);")

(define_expand "bgeu"
  [(set (pc) (if_then_else (geu (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (ZERO_EXTEND);")

(define_expand "bleu"
  [(set (pc) (if_then_else (leu (cc0) (const_int 0))
			   (label_ref (match_operand 0 "" "")) (pc)))]
  "" "extend_and_branch (ZERO_EXTEND);")

(define_insn "cmpdf"
  [(set (cc0)
	(compare (match_operand:DF 0 "register_operand" "r")
		 (match_operand:DF 1 "register_operand" "r")))]
  ""
  "cmpd %1,%0")

(define_insn "cmpsf"
  [(set (cc0)
	(compare (match_operand:SF 0 "register_operand" "r")
		 (match_operand:SF 1 "register_operand" "r")))]
  ""
  "cmpf %1,%0")

(define_insn "tstdf"
  [(set (cc0)
       	(match_operand:DF 0 "register_operand" "r"))]
  ""
  "mtstd %0,%0")

(define_insn "tstsf"
  [(set (cc0)
       	(match_operand:SF 0 "register_operand" "r"))]
  ""
  "mtstf %0,%0")

;______________________________________________________________________
;
;	Fixed-point Arithmetic.
;______________________________________________________________________

(define_insn "addsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,!r")
	(plus:SI (match_operand:SI 1 "register_operand" "%0,r")
		 (match_operand:SI 2 "general_operand" "g,rJ")))]
  ""
  "*
{
  if (which_alternative == 0)
    return \"addw %2,%0\";
  else
    {
      CC_STATUS_INIT;
      if (REG_P (operands[2]))
	return \"mova (%2)[%1*1],%0\";
      else
	return \"mova %a2[%1*1],%0\";
    }
}")

(define_insn "subsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(minus:SI (match_operand:SI 1 "general_operand" "0,g")
		  (match_operand:SI 2 "general_operand" "g,0")))]
  ""
  "*
{
  if (which_alternative == 0)
    return \"subw %2,%0\";
  else
    return \"rsubw %1,%0\";
}")

(define_insn "mulsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(mult:SI (match_operand:SI 1 "register_operand" "%0")
		 (match_operand:SI 2 "general_operand" "g")))]
  ""
  "mulw %2,%0")

(define_insn "umulsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(umult:SI (match_operand:SI 1 "register_operand" "%0")
		  (match_operand:SI 2 "general_operand" "g")))]
  ""
  "umulw %2,%0")

(define_insn "divsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(div:SI (match_operand:SI 1 "general_operand" "0,g")
		(match_operand:SI 2 "general_operand" "g,0")))]
  ""
  "*
{
  if (which_alternative == 0)
    return \"divw %2,%0\";
  else
    return \"rdivw %1,%0\";
}")

(define_insn "udivsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(udiv:SI (match_operand:SI 1 "register_operand" "0")
		 (match_operand:SI 2 "general_operand" "g")))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"udivw %2,%0\";
}")

(define_insn "modsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(mod:SI (match_operand:SI 1 "register_operand" "0")
		(match_operand:SI 2 "general_operand" "g")))]
  ""
  "modw %2,%0")

(define_insn "umodsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(umod:SI (match_operand:SI 1 "register_operand" "0")
		 (match_operand:SI 2 "general_operand" "g")))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"umodw %2,%0\";
}")

(define_insn "negsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(neg:SI (match_operand:SI 1 "nonimmediate_operand" "rm")))]
  ""
  "mnegw %1,%0")

(define_insn "one_cmplsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(not:SI (match_operand:SI 1 "nonimmediate_operand" "rm")))]
  ""
  "mcomw %1,%0")

(define_insn "abssi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(abs:SI (match_operand:SI 1 "nonimmediate_operand" "rm")))]
  ""
  "mabsw %1,%0")

;______________________________________________________________________
;
;	Floating-point Arithmetic.
;______________________________________________________________________

(define_insn "adddf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(plus:DF (match_operand:DF 1 "register_operand" "%0")
		 (match_operand:DF 2 "register_operand" "r")))]
  ""
  "addd %2,%0")

(define_insn "addsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(plus:SF (match_operand:SF 1 "register_operand" "%0")
		 (match_operand:SF 2 "register_operand" "r")))]
  ""
  "addf %2,%0")

(define_insn "subdf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(minus:DF (match_operand:DF 1 "register_operand" "0")
		  (match_operand:DF 2 "register_operand" "r")))]
  ""
  "subd %2,%0")

(define_insn "subsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(minus:SF (match_operand:SF 1 "register_operand" "0")
		  (match_operand:SF 2 "register_operand" "r")))]
  ""
  "subf %2,%0")

(define_insn "muldf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(mult:DF (match_operand:DF 1 "register_operand" "%0")
		 (match_operand:DF 2 "register_operand" "r")))]
  ""
  "muld %2,%0")

(define_insn "mulsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(mult:SF (match_operand:SF 1 "register_operand" "%0")
		 (match_operand:SF 2 "register_operand" "r")))]
  ""
  "mulf %2,%0")

(define_insn "divdf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(div:DF (match_operand:DF 1 "register_operand" "0")
		(match_operand:DF 2 "register_operand" "r")))]
  ""
  "divd %2,%0")

(define_insn "divsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(div:SF (match_operand:SF 1 "register_operand" "0")
		(match_operand:SF 2 "register_operand" "r")))]
  ""
  "divf %2,%0")

(define_insn "negdf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(neg:DF (match_operand:DF 1 "register_operand" "r")))]
  ""
  "mnegd %1,%0")

(define_insn "negsf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(neg:SF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "mnegf %1,%0")

(define_insn "absdf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
	(abs:DF (match_operand:DF 1 "register_operand" "r")))]
  ""
  "mabsd %1,%0")

(define_insn "abssf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
	(abs:SF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "mabsf %1,%0")

;______________________________________________________________________
;
;	Logical and Shift Instructions.
;______________________________________________________________________

(define_insn ""
  [(set (cc0)
	(and:SI (match_operand:SI 0 "register_operand" "%r")
		(match_operand:SI 1 "general_operand" "g")))]
  ""
  "bitw %1,%0");

(define_insn "andsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(and:SI (match_operand:SI 1 "register_operand" "%0,r")
		(match_operand:SI 2 "general_operand" "g,K")))]
  ""
  "*
{
  if (which_alternative == 0)
    return \"andw %2,%0\";
  else
    {
      CC_STATUS_INIT;
      return (INTVAL (operands[2]) == 255
	      ? \"movzbw %1,%0\" : \"movzhw %1,%0\");
    }
}")

(define_insn "andcbsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(and:SI (match_operand:SI 1 "register_operand" "0")
		(not:SI (match_operand:SI 2 "general_operand" "g"))))]
  ""
  "bicw %2,%0")

(define_insn ""
  [(set (match_operand:SI 0 "register_operand" "=r")
	(and:SI (not:SI (match_operand:SI 1 "general_operand" "g"))
		(match_operand:SI 2 "register_operand" "0")))]
  ""
  "bicw %1,%0")

(define_insn "iorsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ior:SI (match_operand:SI 1 "register_operand" "%0")
		(match_operand:SI 2 "general_operand" "g")))]
  ""
  "orw %2,%0")

(define_insn "xorsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(xor:SI (match_operand:SI 1 "register_operand" "%0")
		(match_operand:SI 2 "general_operand" "g")))]
  ""
  "xorw %2,%0")

; Some patterns using De Morgan law.
;
;(define_insn ""
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(not:SI (and:SI (not:SI (match_operand:SI 1 "register_operand" "%0"))
;			(not:SI (match_operand:SI 2 "general_operand" "g")))))]
;  ""
;  "orw %2,%0")
;
;(define_insn ""
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(not:SI (ior:SI (not:SI (match_operand:SI 1 "register_operand" "%0"))
;			(not:SI (match_operand:SI 2 "general_operand" "g")))))]
;  ""
;  "andw %2,%0")
;
;(define_insn ""
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(not:SI (and:SI (not:SI (match_operand:SI 1 "register_operand" "0"))
;			(match_operand:SI 2 "immediate_operand" "n"))))]
;  ""
;  "*
;  operands[2] = gen_rtx (CONST_INT, VOIDmode, ~INTVAL (operands[2]));
;  return \"orw %2,%0\";
;")
;
;(define_insn ""
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(not:SI (ior:SI (not:SI (match_operand:SI 1 "register_operand" "0"))
;			(match_operand:SI 2 "immediate_operand" "n"))))]
;  ""
;  "*
;  operands[2] = gen_rtx (CONST_INT, VOIDmode, ~INTVAL (operands[2]));
;  return \"andw %2,%0\";
;")

(define_insn "ashlsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ashift:SI (match_operand:SI 1 "register_operand" "0")
		   (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 32;
      if (cnt == 0)
	  return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  /* Use lshlw, not ashlw, since arithmetic shifts work strangely on pyr.  */
  return \"lshlw %2,%0\";
}")

; The arithmetic left shift instructions work strange on pyramids.
; They fail to modify the sign bit.
;(define_insn "ashldi3"
;  [(set (match_operand:DI 0 "register_operand" "=r")
;	(ashift:DI (match_operand:DI 1 "register_operand" "0")
;		   (match_operand:SI 2 "general_operand" "rnm")))]
;  ""
;  "*
;{
;  if (GET_CODE (operands[2]) == CONST_INT)
;    {
;      int cnt = INTVAL (operands[2]) % 64;
;      if (cnt == 0)
;	  return \"\";
;      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
;    }
;  return \"ashll %2,%0\";
;}")

(define_insn "ashrsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ashiftrt:SI (match_operand:SI 1 "register_operand" "0")
		     (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 32;
      if (cnt == 0)
	  return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  return \"ashrw %2,%0\";
}")

(define_insn "ashrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(ashiftrt:DI (match_operand:DI 1 "register_operand" "0")
		     (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 64;
      if (cnt == 0)
	return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  return \"ashrl %2,%0\";
}")

(define_insn "lshrsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(lshiftrt:SI (match_operand:SI 1 "register_operand" "0")
		     (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 32;
      if (cnt == 0)
	  return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  return \"lshrw %2,%0\";
}")

(define_insn "rotlsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(rotate:SI (match_operand:SI 1 "register_operand" "0")
		   (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 32;
      if (cnt == 0)
	  return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  return \"rotlw %2,%0\";
}")

(define_insn "rotrsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(rotatert:SI (match_operand:SI 1 "register_operand" "0")
		     (match_operand:SI 2 "general_operand" "rnm")))]
  ""
  "*
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      int cnt = INTVAL (operands[2]) % 32;
      if (cnt == 0)
	  return \"\";
      operands[2] = gen_rtx (CONST_INT, VOIDmode, cnt);
    }
  return \"rotrw %2,%0\";
}")

;______________________________________________________________________
;
;	Fixed and Floating Moves.
;______________________________________________________________________

;; If the destination is a memory address, indexed source operands are
;; disallowed.  Big DImode constants are always loaded into a reg pair,
;; although offsetable memory addresses really could be dealt with.

(define_insn ""
  [(set (match_operand:DI 0 "memory_operand" "=m")
	(match_operand:DI 1 "nonindexed_operand" "gF"))]
  "(GET_CODE (operands[1]) == CONST_DOUBLE
    ? CONST_DOUBLE_HIGH (operands[1]) == 0
    : 1)"
  "*
{
  CC_STATUS_INIT;
  if (GET_CODE (operands[1]) == CONST_DOUBLE)
    operands[1] = gen_rtx (CONST_INT, VOIDmode,
				      CONST_DOUBLE_LOW (operands[1]));
  return \"movl %1,%0\";
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movdi"
  [(set (match_operand:DI 0 "general_operand" "=r")
	(match_operand:DI 1 "general_operand" "gF"))]
  ""
  "* return output_move_double (operands); ")

;; If the destination is a memory address, indexed operands are disallowed.

(define_insn ""
  [(set (match_operand:SI 0 "memory_operand" "=m")
	(match_operand:SI 1 "nonindexed_operand" "g"))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movw %1,%0\";
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movsi"
  [(set (match_operand:SI 0 "general_operand" "=r")
	(match_operand:SI 1 "general_operand" "g"))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movw %1,%0\";
}")

;; If the destination is a memory address, indexed operands are disallowed.

(define_insn ""
  [(set (match_operand:HI 0 "memory_operand" "=m")
	(match_operand:HI 1 "nonindexed_operand" "g"))]
  ""
  "*
{
  if (REG_P (operands[1]))
    return \"cvtwh %1,%0\";		/* reg -> mem */
  else
    {
      CC_STATUS_INIT;
      return \"movh %1,%0\";		/* mem imm -> mem */
    }
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movhi"
  [(set (match_operand:HI 0 "general_operand" "=r")
	(match_operand:HI 1 "general_operand" "g"))]
  ""
  "*
{
  if (GET_CODE (operands[1]) != MEM)
    {
      CC_STATUS_INIT;
      return \"movw %1,%0\";
    }
  return \"cvthw %1,%0\";
}")

;; If the destination is a memory address, indexed operands are disallowed.

(define_insn ""
  [(set (match_operand:QI 0 "memory_operand" "=m")
	(match_operand:QI 1 "nonindexed_operand" "g"))]
  ""
  "*
{
  if (REG_P (operands[1]))
    return \"cvtwb %1,%0\";		/* reg -> mem */
  else
    {
      CC_STATUS_INIT;
      return \"movb %1,%0\";		/* mem imm -> mem */
    }
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movqi"
  [(set (match_operand:QI 0 "general_operand" "=r")
	(match_operand:QI 1 "general_operand" "g"))]
  ""
  "*
{
  if (GET_CODE (operands[1]) != MEM)
    {
      CC_STATUS_INIT;
      return \"movw %1,%0\";
    }
  return \"cvtbw %1,%0\";
}")

;; If the destination is a memory address, indexed operands are disallowed, and
;; so are immediate operands.  (Constants are always loaded into a reg pair,
;; although offsetable memory addresses really doesn't need that.)

(define_insn ""
  [(set (match_operand:DF 0 "memory_operand" "=m")
	(match_operand:DF 1 "nonindexed_operand" "g"))]
  "GET_CODE (operands[1]) != CONST_DOUBLE"
  "*
{
  CC_STATUS_INIT;
  return \"movl %1,%0\";
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movdf"
  [(set (match_operand:DF 0 "general_operand" "=r")
	(match_operand:DF 1 "general_operand" "gF"))]
  ""
  "* return output_move_double (operands); ")

;; If the destination is a memory address, indexed operands are disallowed.

(define_insn ""
  [(set (match_operand:SF 0 "memory_operand" "=m")
	(match_operand:SF 1 "nonindexed_operand" "g"))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movw %1,%0\";
}")

;; Force the destination to a register, so all source operands are allowed.

(define_insn "movsf"
  [(set (match_operand:SF 0 "general_operand" "=r")
	(match_operand:SF 1 "general_operand" "g"))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movw %1,%0\";
}")

(define_insn ""
  [(set (match_operand:SI 0 "register_operand" "=r")
	(match_operand:QI 1 "address_operand" "p"))]
  ""
  "*
  CC_STATUS_INIT;
  return \"mova %a1,%0\";
")

;______________________________________________________________________
;
;	Conversion patterns.
;______________________________________________________________________

;; The trunc patterns are used only when non compile-time constants are used.

(define_insn "truncsiqi2"
  [(set (match_operand:QI 0 "general_operand" "=r,m")
	(truncate:QI (match_operand:SI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "*
{
  CC_STATUS_INIT;
  if (REGNO (operands[0]) == REGNO (operands[1]))
    return \"\";
  else
    return \"movw %1,%0\";
}")

(define_insn "truncsihi2"
  [(set (match_operand:HI 0 "general_operand" "=r,m")
	(truncate:HI (match_operand:SI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "*
{
  CC_STATUS_INIT;
  if (REGNO (operands[0]) == REGNO (operands[1]))
    return \"\";
  else
    return \"movw %1,%0\";
}")

(define_insn "extendhisi2"
  [(set (match_operand:SI 0 "general_operand" "=r,m")
	(sign_extend:SI (match_operand:HI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "*
{
  extern int optimize;
  if (optimize && REG_P (operands[0]) && REG_P (operands[1])
      && REGNO (operands[0]) == REGNO (operands[1])
      && already_sign_extended (insn, HImode, operands[0]))
    {
      CC_STATUS_INIT;
      return \"\";
    }
  return \"cvthw %1,%0\";
}")

(define_insn "extendqisi2"
  [(set (match_operand:SI 0 "general_operand" "=r,m")
	(sign_extend:SI (match_operand:QI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "*
{
  extern int optimize;
  if (optimize && REG_P (operands[0]) && REG_P (operands[1])
      && REGNO (operands[0]) == REGNO (operands[1])
      && already_sign_extended (insn, QImode, operands[0]))
    {
      CC_STATUS_INIT;
      return \"\";
    }
  return \"cvtbw %1,%0\";
}")

; Pyramid doesn't have insns *called* "cvtbh" or "movzbh".
; But we can cvtbw/movzbw into a register, where there is no distinction
; between words and halfwords.
(define_insn "extendqihi2"
  [(set (match_operand:HI 0 "register_operand" "=r")
	(sign_extend:HI (match_operand:QI 1 "nonimmediate_operand" "rm")))]
  ""
  "cvtbw %1,%0")

(define_insn "zero_extendhisi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(zero_extend:SI (match_operand:HI 1 "nonimmediate_operand" "rm")))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movzhw %1,%0\";
}")

(define_insn "zero_extendqisi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(zero_extend:SI (match_operand:QI 1 "nonimmediate_operand" "rm")))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movzbw %1,%0\";
}")

(define_insn "zero_extendqihi2"
  [(set (match_operand:HI 0 "register_operand" "=r")
	(zero_extend:HI (match_operand:QI 1 "nonimmediate_operand" "rm")))]
  ""
  "*
{
  CC_STATUS_INIT;
  return \"movzbw %1,%0\";
}")

(define_insn "extendsfdf2"
  [(set (match_operand:DF 0 "general_operand" "=r,m")
	(float_extend:DF (match_operand:SF 1 "nonimmediate_operand" "rm,r")))]
  ""
  "cvtfd %1,%0")

(define_insn "truncdfsf2"
  [(set (match_operand:SF 0 "general_operand" "=r,m")
	(float_truncate:SF (match_operand:DF 1 "nonimmediate_operand" "rm,r")))]
  ""
  "cvtdf %1,%0")

;;----------------------------------------------------------------------------
;;
;; Fix-to-Float and Float-to-Fix Conversion Patterns.
;;
;; Note that the ones that start with SImode come first.
;; That is so that an operand that is a CONST_INT
;; (and therefore lacks a specific machine mode).
;; will be recognized as SImode (which is always valid)
;; rather than as QImode or HImode.
;;
;;----------------------------------------------------------------------------

(define_insn "floatsisf2"
  [(set (match_operand:SF 0 "general_operand" "=r,m")
	(float:SF (match_operand:SI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "cvtwf %1,%0")

(define_insn "floatsidf2"
  [(set (match_operand:DF 0 "general_operand" "=r,m")
	(float:DF (match_operand:SI 1 "nonimmediate_operand" "rm,r")))]
  ""
  "cvtwd %1,%0")

(define_insn "fix_truncsfsi2"
  [(set (match_operand:SI 0 "general_operand" "=r,m")
	(fix:SI (fix:SF (match_operand:SF 1 "nonimmediate_operand" "rm,r"))))]
  ""
  "cvtfw %1,%0")

(define_insn "fix_truncdfsi2"
  [(set (match_operand:SI 0 "general_operand" "=r,m")
	(fix:SI (fix:DF (match_operand:DF 1 "nonimmediate_operand" "rm,r"))))]
  ""
  "cvtdw %1,%0")

;______________________________________________________________________
;
;	Flow Control Patterns.
;______________________________________________________________________

;; Prefer "br" to "jump" for unconditional jumps, since it's faster.
;; (The assembler can manage with out-of-range branches.)

(define_insn "jump"
  [(set (pc)
	(label_ref (match_operand 0 "" "")))]
  ""
  "br %l0")

(define_insn ""
  [(set (pc)
	(if_then_else (match_operator 0 "relop" [(cc0) (const_int 0)])
		      (label_ref (match_operand 1 "" ""))
		      (pc)))]
  ""
  "b%N0 %l1")

(define_insn ""
  [(set (pc)
	(if_then_else (match_operator 0 "relop" [(cc0) (const_int 0)])
		      (pc)
		      (label_ref (match_operand 1 "" ""))))]
  ""
  "b%C0 %l1")

(define_insn "call"
  [(call (match_operand:QI 0 "memory_operand" "m")
	 (match_operand:QI 1 "immediate_operand" "n"))]
  ""
  "call %0")

(define_insn "call_value"
  [(set (match_operand 0 "" "=r")
	(call (match_operand:QI 1 "memory_operand" "m")
	      (match_operand:SI 2 "immediate_operand" "n")))]
  ;; Operand 2 not really used on Pyramid architecture.
  ""
  "call %1")

(define_insn ""
  [(return)]
  ""
  "ret")

(define_insn "tablejump"
  [(set (pc) (match_operand:SI 0 "register_operand" "r"))
   (use (label_ref (match_operand 1 "" "")))]
  ""
  "jump (%0)")

;______________________________________________________________________
;
;	Peep-hole Optimization Patterns.
;______________________________________________________________________

;; Optimize fullword move followed by a test of the moved value.

;(define_peephole
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(match_operand:SI 1 "nonimmediate_operand" "rm"))
;   (set (cc0) (match_operand:SI 2 "nonimmediate_operand" "rm"))]
;  "rtx_equal_p (operands[2], operands[0])
;   || rtx_equal_p (operands[2], operands[1])"
;  "mtstw %1,%0")
;
;;; Optimize loops with a incremented/decremented variable.
;
;(define_peephole
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(plus:SI (match_dup 0)
;		 (const_int -1)))
;   (set (cc0)
;	(compare (match_operand:SI 1 "register_operand" "r")
;		 (match_operand:SI 2 "nonmemory_operand" "ri")))
;   (set (pc)
;	(if_then_else (match_operator:SI 3 "signed_comparison"
;			 [(cc0) (const_int 0)])
;		      (label_ref (match_operand 4 "" ""))
;		      (pc)))]
;  "rtx_equal_p (operands[0], operands[1])
;     || rtx_equal_p (operands[0], operands[2])"
;  "*
;  if (rtx_equal_p (operands[0], operands[1]))
;    {
;      output_asm_insn (\"dcmpw %2,%0\", operands);
;      return output_branch (GET_CODE (operands[3]));
;    }
;  else
;    {
;      output_asm_insn (\"dcmpw %1,%0\", operands);
;      return output_inv_branch (GET_CODE (operands[3]));
;    }
;")
;
;(define_peephole
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(plus:SI (match_dup 0)
;		 (const_int 1)))
;   (set (cc0)
;	(compare (match_operand:SI 1 "register_operand" "r")
;		 (match_operand:SI 2 "nonmemory_operand" "ri")))
;   (set (pc)
;	(if_then_else (match_operator:SI 3 "signed_comparison"
;			 [(cc0) (const_int 0)])
;		      (label_ref (match_operand 4 "" ""))
;		      (pc)))]
;  "rtx_equal_p (operands[0], operands[1])
;     || rtx_equal_p (operands[0], operands[2])"
;  "*
;  if (rtx_equal_p (operands[0], operands[1]))
;    {
;      output_asm_insn (\"icmpw %2,%0\", operands);
;      return output_branch (GET_CODE (operands[3]));
;    }
;  else
;    {
;      output_asm_insn (\"icmpw %1,%0\", operands);
;      return output_inv_branch (GET_CODE (operands[3]));
;    }
;")
;
;(define_peephole
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(plus:SI (match_dup 0) (const_int -1)))
;   (set (cc0) (match_dup 0))
;   (set (pc) (if_then_else (match_operator:SI 1 "signed_comparison"
;					      [(cc0) (const_int 0)])
;			   (label_ref (match_operand 4 "" ""))
;			   (pc)))]
;  ""
;  "*
;    output_asm_insn (\"dcmpw $0,%0\", operands);
;    return output_branch (GET_CODE (operands[1]));
;")
;
;(define_peephole
;  [(set (match_operand:SI 0 "register_operand" "=r")
;	(plus:SI (match_dup 0) (const_int 1)))
;   (set (cc0) (match_dup 0))
;   (set (pc) (if_then_else (match_operator:SI 1 "signed_comparison"
;					      [(cc0) (const_int 0)])
;			   (label_ref (match_operand 4 "" ""))
;			   (pc)))]
;  ""
;  "*
;    output_asm_insn (\"icmpw $0,%0\", operands);
;    return output_branch (GET_CODE (operands[1]));
;")
;
;;; Combine word moves with consequtive operands into a long move.
;;; Also combines immediate moves, if the high-order destination operand
;;; is loaded with zero.
;
;(define_peephole
;  [(set (match_operand:SI 0 "general_operand" "=g")
;	(match_operand:SI 1 "general_operand" "g"))
;   (set (match_operand:SI 2 "general_operand" "=g")
;	(match_operand:SI 3 "general_operand" "g"))]
;  "movdi_possible (operands)"
;  "*
;  CC_STATUS_INIT;
;  movdi_possible (operands);
;  if (CONSTANT_P (operands[1]))
;    /* Also operand 3 is guarranteed to be CONSTANT by movdi_possible.  */
;    return (swap_operands) ? \"movl %3,%0\" : \"movl %1,%2\";
;
;  return (swap_operands) ? \"movl %1,%0\" : \"movl %3,%2\";
;")
;
;;; Optimize certain tests after memory stores.
;
;(define_peephole
;  [(set (match_operand 0 "memory_operand" "=m")
;	(match_operand 1 "register_operand" "r"))
;   (set (match_operand:SI 2 "register_operand" "=r")
;	(sign_extend:SI (match_dup 1)))
;   (set (cc0)
;	(match_dup 2))]
;  "dead_or_set_p (insn, operands[2])"
;  "*
;  if (GET_MODE (operands[0]) == QImode)
;    return \"cvtwb %1,%0\";
;  else
;    return \"cvtwh %1,%0\";
;")

;______________________________________________________________________
;
;	DImode Patterns.
;______________________________________________________________________

(define_expand "extendsidi2"
  [(set (subreg:SI (match_operand:DI 0 "register_operand" "=r") 1)
	(match_operand:SI 1 "register_operand" "r"))
   (set (subreg:SI (match_dup 0) 0)
	(subreg:SI (match_dup 0) 1))
   (set (subreg:SI (match_dup 0) 0)
	(ashiftrt:SI (subreg:SI (match_dup 0) 0)
		     (const_int 31)))]
  ""
  "")

;; I need to debug these.

;(define_expand "cmpdi"
;  [(set (cc0)
;	(compare (subreg:SI (match_operand:DI 0 "register_operand" "r") 0)
;		 (subreg:SI (match_operand:DI 1 "register_operand" "r") 0)))
;   (set (pc) (if_then_else (ne (cc0) (const_int 0))
;			   (label_ref (match_dup 2))
;			   (pc)))
;   (set (cc0)
;	(compare (subreg:SI (match_dup 0) 1)
;		 (subreg:SI (match_dup 1) 1)))
;   (match_dup 2)]
;  ""
;  "operands[2] = gen_label_rtx ();")
;
;(define_expand "tstdi"
;  [(set (cc0) (subreg:SI (match_operand:DI 0 "register_operand" "r") 0))
;   (set (pc) (if_then_else (ne (cc0) (const_int 0))
;			   (label_ref (match_dup 1))
;			   (pc)))
;   (set (cc0) (subreg:SI (match_dup 0) 1))
;   (match_dup 1)]
;  ""
;  "operands[1] = gen_label_rtx ();")

(define_insn "adddi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(plus:DI (match_operand:DI 1 "register_operand" "%0")
		 (match_operand:DI 2 "nonmemory_operand" "rF")))]
  ""
  "*
{
  rtx xoperands[2];
  CC_STATUS_INIT;
  xoperands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[2]))
    xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[2]) + 1);
  else
    {
      xoperands[1] = gen_rtx (CONST_INT, VOIDmode,
			      CONST_DOUBLE_LOW (operands[2]));
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     CONST_DOUBLE_HIGH (operands[2]));
    }
  output_asm_insn (\"addw %1,%0\", xoperands);
  return \"addwc %2,%0\";
}")

(define_insn "subdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(minus:DI (match_operand:DI 1 "register_operand" "0")
		  (match_operand:DI 2 "nonmemory_operand" "rF")))]
  ""
  "*
{
  rtx xoperands[2];
  CC_STATUS_INIT;
  xoperands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[2]))
    xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[2]) + 1);
  else
    {
      xoperands[1] = gen_rtx (CONST_INT, VOIDmode,
			      CONST_DOUBLE_LOW (operands[2]));
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     CONST_DOUBLE_HIGH (operands[2]));
    }
  output_asm_insn (\"subw %1,%0\", xoperands);
  return \"subwb %2,%0\";
}")

(define_insn "iordi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(ior:DI (match_operand:DI 1 "register_operand" "%0")
		(match_operand:DI 2 "nonmemory_operand" "rF")))]
  ""
  "*
{
  rtx xoperands[2];
  CC_STATUS_INIT;
  xoperands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[2]))
    xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[2]) + 1);
  else
    {
      xoperands[1] = gen_rtx (CONST_INT, VOIDmode,
			      CONST_DOUBLE_LOW (operands[2]));
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     CONST_DOUBLE_HIGH (operands[2]));
    }
  output_asm_insn (\"orw %1,%0\", xoperands);
  return \"orw %2,%0\";
}")

(define_insn "anddi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(and:DI (match_operand:DI 1 "register_operand" "%0")
		(match_operand:DI 2 "nonmemory_operand" "rF")))]
  ""
  "*
{
  rtx xoperands[2];
  CC_STATUS_INIT;
  xoperands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[2]))
    xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[2]) + 1);
  else
    {
      xoperands[1] = gen_rtx (CONST_INT, VOIDmode,
			      CONST_DOUBLE_LOW (operands[2]));
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     CONST_DOUBLE_HIGH (operands[2]));
    }
  output_asm_insn (\"andw %1,%0\", xoperands);
  return \"andw %2,%0\";
}")

(define_insn "xordi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(xor:DI (match_operand:DI 1 "register_operand" "%0")
		(match_operand:DI 2 "nonmemory_operand" "rF")))]
  ""
  "*
{
  rtx xoperands[2];
  CC_STATUS_INIT;
  xoperands[0] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[2]))
    xoperands[1] = gen_rtx (REG, SImode, REGNO (operands[2]) + 1);
  else
    {
      xoperands[1] = gen_rtx (CONST_INT, VOIDmode,
			      CONST_DOUBLE_LOW (operands[2]));
      operands[2] = gen_rtx (CONST_INT, VOIDmode,
			     CONST_DOUBLE_HIGH (operands[2]));
    }
  output_asm_insn (\"xorw %1,%0\", xoperands);
  return \"xorw %2,%0\";
}")

(define_insn "nop"
  [(const_int 0)]
  ""
  "movw gr0,gr0  # nop")

;;- Local variables:
;;- mode:emacs-lisp
;;- comment-start: ";;- "
;;- eval: (set-syntax-table (copy-sequence (syntax-table)))
;;- eval: (modify-syntax-entry ?] ")[")
;;- eval: (modify-syntax-entry ?{ "(}")
;;- eval: (modify-syntax-entry ?} "){")
;;- End:
