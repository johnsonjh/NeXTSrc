/* Generated automatically by the program `genrecog'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"

/* `recog' contains a decision tree
   that recognizes whether the rtx X0 is a valid instruction.

   recog returns -1 if the rtx is not valid.
   If the rtx is valid, recog returns a nonnegative number
   which is the insn code number for the pattern that matched.
   This is the same as the order in the machine description of
   the entry that matched.  This number can be used as an index into
   insn_templates and insn_n_operands (found in insn-output.c)
   or as an argument to output_insn_hairy (also in insn-output.c).  */

rtx recog_operand[MAX_RECOG_OPERANDS];

rtx *recog_operand_loc[MAX_RECOG_OPERANDS];

rtx *recog_dup_loc[MAX_DUP_OPERANDS];

char recog_dup_num[MAX_DUP_OPERANDS];

extern rtx recog_addr_dummy;

#define operands recog_operand

int
recog_1 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L8:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[0] = x1; return 2; }
 L11:
  if (general_operand (x1, HImode))
    { recog_operand[0] = x1; return 3; }
 L14:
  if (general_operand (x1, QImode))
    { recog_operand[0] = x1; return 4; }
 L23:
  if (general_operand (x1, SFmode))
    { recog_operand[0] = x1; if (TARGET_68881) return 7; }
 L32:
  if (general_operand (x1, DFmode))
    { recog_operand[0] = x1; if (TARGET_68881) return 10; }
 L35:
  if (GET_CODE (x1) == COMPARE && 1)
    goto L36;
  if (GET_CODE (x1) == ZERO_EXTRACT && 1)
    goto L77;
  if (GET_CODE (x1) == SUBREG && GET_MODE (x1) == SImode && XINT (x1, 1) == 0 && 1)
    goto L131;
  if (GET_CODE (x1) == AND && GET_MODE (x1) == SImode && 1)
    goto L137;
 L886:
  if (GET_CODE (x1) == ZERO_EXTRACT && GET_MODE (x1) == SImode && 1)
    goto L887;
  if (GET_CODE (x1) != SUBREG)
    goto ret0;
  if (GET_MODE (x1) == QImode && XINT (x1, 1) == 0 && 1)
    goto L893;
  if (GET_MODE (x1) == HImode && XINT (x1, 1) == 0 && 1)
    goto L900;
  goto ret0;
 L36:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[0] = x2; goto L37; }
 L41:
  if (general_operand (x2, HImode))
    { recog_operand[0] = x2; goto L42; }
 L46:
  if (general_operand (x2, QImode))
    { recog_operand[0] = x2; goto L47; }
 L59:
  if (general_operand (x2, DFmode))
    { recog_operand[0] = x2; goto L60; }
 L72:
  if (general_operand (x2, SFmode))
    { recog_operand[0] = x2; goto L73; }
 L680:
  if (general_operand (x2, QImode))
    { recog_operand[0] = x2; goto L681; }
 L687:
  if (GET_CODE (x2) == LSHIFTRT && GET_MODE (x2) == SImode && 1)
    goto L688;
 L694:
  if (general_operand (x2, QImode))
    { recog_operand[0] = x2; goto L695; }
 L701:
  if (GET_CODE (x2) == ASHIFTRT && GET_MODE (x2) == SImode && 1)
    goto L702;
  goto ret0;
 L37:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; return 11; }
  x2 = XEXP (x1, 0);
  goto L41;
 L42:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 12; }
  x2 = XEXP (x1, 0);
  goto L46;
 L47:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 13; }
  x2 = XEXP (x1, 0);
  goto L59;
 L60:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 16; }
  x2 = XEXP (x1, 0);
  goto L72;
 L73:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 19; }
  x2 = XEXP (x1, 0);
  goto L680;
 L681:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LSHIFTRT && GET_MODE (x2) == SImode && 1)
    goto L682;
  x2 = XEXP (x1, 0);
  goto L687;
 L682:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, SImode))
    { recog_operand[1] = x3; goto L683; }
  x2 = XEXP (x1, 0);
  goto L687;
 L683:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 24 && 1)
    if ((GET_CODE (operands[0]) == CONST_INT
    && (INTVAL (operands[0]) & ~0xff) == 0)) return 164;
  x2 = XEXP (x1, 0);
  goto L687;
 L688:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, SImode))
    { recog_operand[0] = x3; goto L689; }
  goto L694;
 L689:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 24 && 1)
    goto L690;
  goto L694;
 L690:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; if ((GET_CODE (operands[1]) == CONST_INT
    && (INTVAL (operands[1]) & ~0xff) == 0)) return 165; }
  x2 = XEXP (x1, 0);
  goto L694;
 L695:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == ASHIFTRT && GET_MODE (x2) == SImode && 1)
    goto L696;
  x2 = XEXP (x1, 0);
  goto L701;
 L696:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, SImode))
    { recog_operand[1] = x3; goto L697; }
  x2 = XEXP (x1, 0);
  goto L701;
 L697:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 24 && 1)
    if ((GET_CODE (operands[0]) == CONST_INT
    && ((INTVAL (operands[0]) + 0x80) & ~0xff) == 0)) return 166;
  x2 = XEXP (x1, 0);
  goto L701;
 L702:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, SImode))
    { recog_operand[0] = x3; goto L703; }
  goto ret0;
 L703:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 24 && 1)
    goto L704;
  goto ret0;
 L704:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; if ((GET_CODE (operands[1]) == CONST_INT
    && ((INTVAL (operands[1]) + 0x80) & ~0xff) == 0)) return 167; }
  goto ret0;
 L77:
  x2 = XEXP (x1, 0);
  if (nonimmediate_operand (x2, QImode))
    { recog_operand[0] = x2; goto L78; }
 L119:
  if (nonimmediate_operand (x2, HImode))
    { recog_operand[0] = x2; goto L120; }
 L85:
  if (nonimmediate_operand (x2, SImode))
    { recog_operand[0] = x2; goto L86; }
  goto L886;
 L78:
  x2 = XEXP (x1, 1);
  if (x2 == const1_rtx && 1)
    goto L79;
  x2 = XEXP (x1, 0);
  goto L119;
 L79:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == SImode && 1)
    goto L80;
 L115:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (GET_CODE (operands[1]) == CONST_INT
   && (unsigned) INTVAL (operands[1]) < 8) return 24; }
  x2 = XEXP (x1, 0);
  goto L119;
 L80:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 7 && 1)
    goto L81;
  goto L115;
 L81:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; return 20; }
 L97:
  if (GET_CODE (x3) == AND && GET_MODE (x3) == SImode && 1)
    goto L98;
  goto L115;
 L98:
  x4 = XEXP (x3, 0);
  if (general_operand (x4, SImode))
    { recog_operand[1] = x4; goto L99; }
  goto L115;
 L99:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XINT (x4, 0) == 7 && 1)
    return 22;
  goto L115;
 L120:
  x2 = XEXP (x1, 1);
  if (x2 == const1_rtx && 1)
    goto L121;
  x2 = XEXP (x1, 0);
  goto L85;
 L121:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (GET_CODE (operands[1]) == CONST_INT) return 25; }
  x2 = XEXP (x1, 0);
  goto L85;
 L86:
  x2 = XEXP (x1, 1);
  if (x2 == const1_rtx && 1)
    goto L87;
  goto L886;
 L87:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == SImode && 1)
    goto L88;
 L127:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (GET_CODE (operands[1]) == CONST_INT) return 26; }
  goto L886;
 L88:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 31 && 1)
    goto L89;
  goto L127;
 L89:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; return 21; }
 L107:
  if (GET_CODE (x3) == AND && GET_MODE (x3) == SImode && 1)
    goto L108;
  goto L127;
 L108:
  x4 = XEXP (x3, 0);
  if (general_operand (x4, SImode))
    { recog_operand[1] = x4; goto L109; }
  goto L127;
 L109:
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) == CONST_INT && XINT (x4, 0) == 31 && 1)
    return 23;
  goto L127;
 L131:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == LSHIFTRT && GET_MODE (x2) == QImode && 1)
    goto L132;
  goto ret0;
 L132:
  x3 = XEXP (x2, 0);
  if (nonimmediate_operand (x3, QImode))
    { recog_operand[0] = x3; goto L133; }
  goto ret0;
 L133:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == 7 && 1)
    return 27;
  goto ret0;
 L137:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == SIGN_EXTEND && GET_MODE (x2) == SImode && 1)
    goto L138;
  goto ret0;
 L138:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == SIGN_EXTEND && GET_MODE (x3) == HImode && 1)
    goto L139;
  goto ret0;
 L139:
  x4 = XEXP (x3, 0);
  if (nonimmediate_operand (x4, QImode))
    { recog_operand[0] = x4; goto L140; }
  goto ret0;
 L140:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if ((GET_CODE (operands[1]) == CONST_INT
    && (unsigned) INTVAL (operands[1]) < 0x100
    && exact_log2 (INTVAL (operands[1])) >= 0)) return 28; }
  goto ret0;
 L887:
  x2 = XEXP (x1, 0);
  if (memory_operand (x2, QImode))
    { recog_operand[0] = x2; goto L888; }
 L907:
  if (nonimmediate_operand (x2, SImode))
    { recog_operand[0] = x2; goto L908; }
  goto ret0;
 L888:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L889; }
  x2 = XEXP (x1, 0);
  goto L907;
 L889:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 200; }
  x2 = XEXP (x1, 0);
  goto L907;
 L908:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L909; }
  goto ret0;
 L909:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 203; }
  goto ret0;
 L893:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == ZERO_EXTRACT && GET_MODE (x2) == SImode && 1)
    goto L894;
  goto ret0;
 L894:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, QImode))
    { recog_operand[0] = x3; goto L895; }
 L914:
  if (nonimmediate_operand (x3, SImode))
    { recog_operand[0] = x3; goto L915; }
  goto ret0;
 L895:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L896; }
  x3 = XEXP (x2, 0);
  goto L914;
 L896:
  x3 = XEXP (x2, 2);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 201; }
  x3 = XEXP (x2, 0);
  goto L914;
 L915:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L916; }
  goto ret0;
 L916:
  x3 = XEXP (x2, 2);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 204; }
  goto ret0;
 L900:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == ZERO_EXTRACT && GET_MODE (x2) == SImode && 1)
    goto L901;
  goto ret0;
 L901:
  x3 = XEXP (x2, 0);
  if (memory_operand (x3, QImode))
    { recog_operand[0] = x3; goto L902; }
 L921:
  if (nonimmediate_operand (x3, SImode))
    { recog_operand[0] = x3; goto L922; }
  goto ret0;
 L902:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L903; }
  x3 = XEXP (x2, 0);
  goto L921;
 L903:
  x3 = XEXP (x2, 2);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 202; }
  x3 = XEXP (x2, 0);
  goto L921;
 L922:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L923; }
  goto ret0;
 L923:
  x3 = XEXP (x2, 2);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT) return 205; }
  goto ret0;
 ret0: return -1;
}

int
recog_2 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L977:
  x1 = XEXP (x0, 1);
  x2 = XEXP (x1, 0);
 switch (GET_CODE (x2))
  {
  case EQ:
  if (1)
    goto L978;
  break;
  case NE:
  if (1)
    goto L987;
  break;
  case GT:
  if (1)
    goto L996;
  break;
  case GTU:
  if (1)
    goto L1005;
  break;
  case LT:
  if (1)
    goto L1014;
  break;
  case LTU:
  if (1)
    goto L1023;
  break;
  case GE:
  if (1)
    goto L1032;
  break;
  case GEU:
  if (1)
    goto L1041;
  break;
  case LE:
  if (1)
    goto L1050;
  break;
  case LEU:
  if (1)
    goto L1059;
  break;
  }
  goto ret0;
 L978:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L979;
  goto ret0;
 L979:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L980;
  goto ret0;
 L980:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L981;
  if (x2 == pc_rtx && 1)
    goto L1071;
  goto ret0;
 L981:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L982; }
  goto ret0;
 L982:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 216;
  goto ret0;
 L1071:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1072;
  goto ret0;
 L1072:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 226; }
  goto ret0;
 L987:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L988;
  goto ret0;
 L988:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L989;
  goto ret0;
 L989:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L990;
  if (x2 == pc_rtx && 1)
    goto L1080;
  goto ret0;
 L990:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L991; }
  goto ret0;
 L991:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 217;
  goto ret0;
 L1080:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1081;
  goto ret0;
 L1081:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 227; }
  goto ret0;
 L996:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L997;
  goto ret0;
 L997:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L998;
  goto ret0;
 L998:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L999;
  if (x2 == pc_rtx && 1)
    goto L1089;
  goto ret0;
 L999:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1000; }
  goto ret0;
 L1000:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 218;
  goto ret0;
 L1089:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1090;
  goto ret0;
 L1090:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 228; }
  goto ret0;
 L1005:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1006;
  goto ret0;
 L1006:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1007;
  goto ret0;
 L1007:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1008;
  if (x2 == pc_rtx && 1)
    goto L1098;
  goto ret0;
 L1008:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1009; }
  goto ret0;
 L1009:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 219;
  goto ret0;
 L1098:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1099;
  goto ret0;
 L1099:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 229; }
  goto ret0;
 L1014:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1015;
  goto ret0;
 L1015:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1016;
  goto ret0;
 L1016:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1017;
  if (x2 == pc_rtx && 1)
    goto L1107;
  goto ret0;
 L1017:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1018; }
  goto ret0;
 L1018:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 220;
  goto ret0;
 L1107:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1108;
  goto ret0;
 L1108:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 230; }
  goto ret0;
 L1023:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1024;
  goto ret0;
 L1024:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1025;
  goto ret0;
 L1025:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1026;
  if (x2 == pc_rtx && 1)
    goto L1116;
  goto ret0;
 L1026:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1027; }
  goto ret0;
 L1027:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 221;
  goto ret0;
 L1116:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1117;
  goto ret0;
 L1117:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 231; }
  goto ret0;
 L1032:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1033;
  goto ret0;
 L1033:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1034;
  goto ret0;
 L1034:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1035;
  if (x2 == pc_rtx && 1)
    goto L1125;
  goto ret0;
 L1035:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1036; }
  goto ret0;
 L1036:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 222;
  goto ret0;
 L1125:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1126;
  goto ret0;
 L1126:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 232; }
  goto ret0;
 L1041:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1042;
  goto ret0;
 L1042:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1043;
  goto ret0;
 L1043:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1044;
  if (x2 == pc_rtx && 1)
    goto L1134;
  goto ret0;
 L1044:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1045; }
  goto ret0;
 L1045:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 223;
  goto ret0;
 L1134:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1135;
  goto ret0;
 L1135:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 233; }
  goto ret0;
 L1050:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1051;
  goto ret0;
 L1051:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1052;
  goto ret0;
 L1052:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1053;
  if (x2 == pc_rtx && 1)
    goto L1143;
  goto ret0;
 L1053:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1054; }
  goto ret0;
 L1054:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 224;
  goto ret0;
 L1143:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1144;
  goto ret0;
 L1144:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 234; }
  goto ret0;
 L1059:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L1060;
  goto ret0;
 L1060:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L1061;
  goto ret0;
 L1061:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1062;
  if (x2 == pc_rtx && 1)
    goto L1152;
  goto ret0;
 L1062:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L1063; }
  goto ret0;
 L1063:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 225;
  goto ret0;
 L1152:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1153;
  goto ret0;
 L1153:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 235; }
  goto ret0;
 ret0: return -1;
}

int
recog_3 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L159:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, QImode))
    { recog_operand[1] = x1; return 34; }
 L178:
 switch (GET_CODE (x1))
  {
  case TRUNCATE:
  if (GET_MODE (x1) == QImode && 1)
    goto L179;
  break;
  case FIX:
  if (GET_MODE (x1) == QImode && 1)
    goto L275;
  break;
  case PLUS:
  if (GET_MODE (x1) == QImode && 1)
    goto L331;
  break;
  case MINUS:
  if (GET_MODE (x1) == QImode && 1)
    goto L384;
  break;
  case AND:
  if (GET_MODE (x1) == QImode && 1)
    goto L567;
  break;
  case IOR:
  if (GET_MODE (x1) == QImode && 1)
    goto L594;
  break;
  case XOR:
  if (GET_MODE (x1) == QImode && 1)
    goto L609;
  break;
  case NEG:
  if (GET_MODE (x1) == QImode && 1)
    goto L622;
  break;
  case NOT:
  if (GET_MODE (x1) == QImode && 1)
    goto L666;
  break;
  case ASHIFT:
  if (GET_MODE (x1) == QImode && 1)
    goto L718;
  break;
  case ASHIFTRT:
  if (GET_MODE (x1) == QImode && 1)
    goto L733;
  break;
  case LSHIFT:
  if (GET_MODE (x1) == QImode && 1)
    goto L748;
  break;
  case LSHIFTRT:
  if (GET_MODE (x1) == QImode && 1)
    goto L763;
  break;
  case ROTATE:
  if (GET_MODE (x1) == QImode && 1)
    goto L778;
  break;
  case ROTATERT:
  if (GET_MODE (x1) == QImode && 1)
    goto L793;
  break;
  case EQ:
  if (1)
    goto L927;
  break;
  case NE:
  if (1)
    goto L932;
  break;
  case GT:
  if (1)
    goto L937;
  break;
  case GTU:
  if (1)
    goto L942;
  break;
  case LT:
  if (1)
    goto L947;
  break;
  case LTU:
  if (1)
    goto L952;
  break;
  case GE:
  if (1)
    goto L957;
  break;
  case GEU:
  if (1)
    goto L962;
  break;
  case LE:
  if (1)
    goto L967;
  break;
  case LEU:
  if (1)
    goto L972;
  break;
  }
  goto ret0;
 L179:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; return 40; }
 L183:
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 41; }
  goto ret0;
 L275:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 71; }
 L287:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 74; }
  goto ret0;
 L331:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L332; }
  goto ret0;
 L332:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 83; }
  goto ret0;
 L384:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L385; }
  goto ret0;
 L385:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 95; }
  goto ret0;
 L567:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L568; }
  goto ret0;
 L568:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 135; }
  goto ret0;
 L594:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L595; }
  goto ret0;
 L595:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 140; }
  goto ret0;
 L609:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L610; }
  goto ret0;
 L610:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 143; }
  goto ret0;
 L622:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 146; }
  goto ret0;
 L666:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 161; }
  goto ret0;
 L718:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L719; }
  goto ret0;
 L719:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 170; }
  goto ret0;
 L733:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L734; }
  goto ret0;
 L734:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 173; }
  goto ret0;
 L748:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L749; }
  goto ret0;
 L749:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 176; }
  goto ret0;
 L763:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L764; }
  goto ret0;
 L764:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 179; }
  goto ret0;
 L778:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L779; }
  goto ret0;
 L779:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 182; }
  goto ret0;
 L793:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L794; }
  goto ret0;
 L794:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[2] = x2; return 185; }
  goto ret0;
 L927:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L928;
  goto ret0;
 L928:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 206;
  goto ret0;
 L932:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L933;
  goto ret0;
 L933:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 207;
  goto ret0;
 L937:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L938;
  goto ret0;
 L938:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 208;
  goto ret0;
 L942:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L943;
  goto ret0;
 L943:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 209;
  goto ret0;
 L947:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L948;
  goto ret0;
 L948:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 210;
  goto ret0;
 L952:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L953;
  goto ret0;
 L953:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 211;
  goto ret0;
 L957:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L958;
  goto ret0;
 L958:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 212;
  goto ret0;
 L962:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L963;
  goto ret0;
 L963:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 213;
  goto ret0;
 L967:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L968;
  goto ret0;
 L968:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 214;
  goto ret0;
 L972:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L973;
  goto ret0;
 L973:
  x2 = XEXP (x1, 1);
  if (x2 == const0_rtx && 1)
    return 215;
  goto ret0;
 ret0: return -1;
}

int
recog_4 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L152:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, HImode))
    { recog_operand[1] = x1; return 32; }
 L186:
  if (GET_MODE (x1) != HImode)
    goto ret0;
 switch (GET_CODE (x1))
  {
  case TRUNCATE:
  if (1)
    goto L187;
  break;
  case ZERO_EXTEND:
  if (1)
    goto L195;
  break;
  case SIGN_EXTEND:
  if (1)
    goto L207;
  break;
  case FIX:
  if (1)
    goto L279;
  break;
  case PLUS:
  if (1)
    goto L320;
  break;
  case MINUS:
  if (1)
    goto L373;
  break;
  case MULT:
  if (1)
    goto L415;
  break;
  case UMULT:
  if (1)
    goto L430;
  break;
  case DIV:
  if (1)
    goto L465;
  break;
  case UDIV:
  if (1)
    goto L480;
  break;
  case MOD:
  if (1)
    goto L515;
  break;
  case UMOD:
  if (1)
    goto L525;
  break;
  case AND:
  if (1)
    goto L562;
  break;
  case IOR:
  if (1)
    goto L589;
  break;
  case XOR:
  if (1)
    goto L604;
  break;
  case NEG:
  if (1)
    goto L618;
  break;
  case NOT:
  if (1)
    goto L662;
  break;
  case ASHIFT:
  if (1)
    goto L713;
  break;
  case ASHIFTRT:
  if (1)
    goto L728;
  break;
  case LSHIFT:
  if (1)
    goto L743;
  break;
  case LSHIFTRT:
  if (1)
    goto L758;
  break;
  case ROTATE:
  if (1)
    goto L773;
  break;
  case ROTATERT:
  if (1)
    goto L788;
  break;
  }
  goto ret0;
 L187:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; return 42; }
  goto ret0;
 L195:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 47; }
  goto ret0;
 L207:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 50; }
  goto ret0;
 L279:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 72; }
 L291:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 75; }
  goto ret0;
 L320:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L321; }
  goto ret0;
 L321:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 81; }
  goto ret0;
 L373:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L374; }
  goto ret0;
 L374:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 93; }
  goto ret0;
 L415:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L416; }
  goto ret0;
 L416:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 103; }
  goto ret0;
 L430:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L431; }
  goto ret0;
 L431:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 106; }
  goto ret0;
 L465:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L466; }
 L470:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L471; }
  goto ret0;
 L466:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 115; }
  x2 = XEXP (x1, 0);
  goto L470;
 L471:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 116; }
  goto ret0;
 L480:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L481; }
 L485:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L486; }
  goto ret0;
 L481:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 118; }
  x2 = XEXP (x1, 0);
  goto L485;
 L486:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 119; }
  goto ret0;
 L515:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L516; }
 L520:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L521; }
  goto ret0;
 L516:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 127; }
  x2 = XEXP (x1, 0);
  goto L520;
 L521:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 128; }
  goto ret0;
 L525:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L526; }
 L530:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L531; }
  goto ret0;
 L526:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 129; }
  x2 = XEXP (x1, 0);
  goto L530;
 L531:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 130; }
  goto ret0;
 L562:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L563; }
  goto ret0;
 L563:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 134; }
  goto ret0;
 L589:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L590; }
  goto ret0;
 L590:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 139; }
  goto ret0;
 L604:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L605; }
  goto ret0;
 L605:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 142; }
  goto ret0;
 L618:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 145; }
  goto ret0;
 L662:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 160; }
  goto ret0;
 L713:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L714; }
  goto ret0;
 L714:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 169; }
  goto ret0;
 L728:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L729; }
  goto ret0;
 L729:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 172; }
  goto ret0;
 L743:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L744; }
  goto ret0;
 L744:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 175; }
  goto ret0;
 L758:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L759; }
  goto ret0;
 L759:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 178; }
  goto ret0;
 L773:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L774; }
  goto ret0;
 L774:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 181; }
  goto ret0;
 L788:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L789; }
  goto ret0;
 L789:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 184; }
  goto ret0;
 ret0: return -1;
}

int
recog_5 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L190:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SImode)
    goto ret0;
 switch (GET_CODE (x1))
  {
  case ZERO_EXTEND:
  if (1)
    goto L191;
  break;
  case SIGN_EXTEND:
  if (1)
    goto L203;
  break;
  case FIX:
  if (1)
    goto L283;
  break;
  case PLUS:
  if (1)
    goto L309;
  break;
  case MINUS:
  if (1)
    goto L362;
  break;
  case MULT:
  if (1)
    goto L420;
  break;
  case UMULT:
  if (1)
    goto L435;
  break;
  case DIV:
  if (1)
    goto L475;
  break;
  case UDIV:
  if (1)
    goto L490;
  break;
  case AND:
  if (1)
    goto L557;
  break;
  case IOR:
  if (1)
    goto L584;
  break;
  case XOR:
  if (1)
    goto L599;
  break;
  case NEG:
  if (1)
    goto L614;
  break;
  case NOT:
  if (1)
    goto L658;
  break;
  case ASHIFTRT:
  if (1)
    goto L670;
  break;
  case LSHIFTRT:
  if (1)
    goto L675;
  break;
  case ASHIFT:
  if (1)
    goto L708;
  break;
  case LSHIFT:
  if (1)
    goto L738;
  break;
  case ROTATE:
  if (1)
    goto L768;
  break;
  case ROTATERT:
  if (1)
    goto L783;
  break;
  }
  goto ret0;
 L191:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 46; }
 L199:
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 48; }
  goto ret0;
 L203:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 49; }
 L211:
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; if (TARGET_68020) return 51; }
  goto ret0;
 L283:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 73; }
 L295:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 76; }
 L299:
  if (GET_CODE (x2) != FIX)
    goto ret0;
  if (GET_MODE (x2) == SFmode && 1)
    goto L300;
  if (GET_MODE (x2) == DFmode && 1)
    goto L305;
  goto ret0;
 L300:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, SFmode))
    { recog_operand[1] = x3; if (TARGET_FPA) return 77; }
  goto ret0;
 L305:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, DFmode))
    { recog_operand[1] = x3; if (TARGET_FPA) return 78; }
  goto ret0;
 L309:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L310; }
  goto ret0;
 L310:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 79; }
 L315:
  if (GET_CODE (x2) == SIGN_EXTEND && GET_MODE (x2) == SImode && 1)
    goto L316;
  goto ret0;
 L316:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, HImode))
    { recog_operand[2] = x3; return 80; }
  goto ret0;
 L362:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L363; }
  goto ret0;
 L363:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 91; }
 L368:
  if (GET_CODE (x2) == SIGN_EXTEND && GET_MODE (x2) == SImode && 1)
    goto L369;
  goto ret0;
 L369:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, HImode))
    { recog_operand[2] = x3; return 92; }
  goto ret0;
 L420:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L421; }
 L425:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L426; }
  goto ret0;
 L421:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 104; }
  x2 = XEXP (x1, 0);
  goto L425;
 L426:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020) return 105; }
  goto ret0;
 L435:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; goto L436; }
 L440:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L441; }
  goto ret0;
 L436:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[2] = x2; return 107; }
  x2 = XEXP (x1, 0);
  goto L440;
 L441:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020) return 108; }
  goto ret0;
 L475:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L476; }
  goto ret0;
 L476:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020) return 117; }
  goto ret0;
 L490:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L491; }
  goto ret0;
 L491:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_68020) return 120; }
  goto ret0;
 L557:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L558; }
 L572:
  if (GET_CODE (x2) == ZERO_EXTEND && GET_MODE (x2) == SImode && 1)
    goto L573;
  goto ret0;
 L558:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 133; }
  x2 = XEXP (x1, 0);
  goto L572;
 L573:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, HImode))
    { recog_operand[1] = x3; goto L574; }
 L579:
  if (general_operand (x3, QImode))
    { recog_operand[1] = x3; goto L580; }
  goto ret0;
 L574:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT
   && (unsigned int) INTVAL (operands[2]) < (1 << GET_MODE_BITSIZE (HImode))) return 136; }
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  goto L579;
 L580:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT
   && (unsigned int) INTVAL (operands[2]) < (1 << GET_MODE_BITSIZE (QImode))) return 137; }
  goto ret0;
 L584:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L585; }
  goto ret0;
 L585:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 138; }
  goto ret0;
 L599:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L600; }
  goto ret0;
 L600:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 141; }
  goto ret0;
 L614:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; return 144; }
  goto ret0;
 L658:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; return 159; }
  goto ret0;
 L670:
  x2 = XEXP (x1, 0);
  if (memory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L671; }
 L723:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L724; }
  goto ret0;
 L671:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 24 && 1)
    if (GET_CODE (XEXP (operands[1], 0)) != POST_INC
   && GET_CODE (XEXP (operands[1], 0)) != PRE_DEC) return 162;
  x2 = XEXP (x1, 0);
  goto L723;
 L724:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 171; }
  goto ret0;
 L675:
  x2 = XEXP (x1, 0);
  if (memory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L676; }
 L753:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L754; }
  goto ret0;
 L676:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 24 && 1)
    if (GET_CODE (XEXP (operands[1], 0)) != POST_INC
   && GET_CODE (XEXP (operands[1], 0)) != PRE_DEC) return 163;
  x2 = XEXP (x1, 0);
  goto L753;
 L754:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 177; }
  goto ret0;
 L708:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L709; }
  goto ret0;
 L709:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 168; }
  goto ret0;
 L738:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L739; }
  goto ret0;
 L739:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 174; }
  goto ret0;
 L768:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L769; }
  goto ret0;
 L769:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 180; }
  goto ret0;
 L783:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L784; }
  goto ret0;
 L784:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 183; }
  goto ret0;
 ret0: return -1;
}

int
recog_6 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L7:
  x1 = XEXP (x0, 0);
  if (x1 == cc0_rtx && 1)
    goto L8;
  if (GET_CODE (x1) == STRICT_LOW_PART && 1)
    goto L162;
  if (x1 == pc_rtx && 1)
    goto L976;
 L1233:
  if (1)
    { recog_operand[0] = x1; goto L1234; }
 L158:
 switch (GET_MODE (x1))
  {
  case QImode:
  if (general_operand (x1, QImode))
    { recog_operand[0] = x1; goto L159; }
  break;
 L151:
  case HImode:
  if (general_operand (x1, HImode))
    { recog_operand[0] = x1; goto L152; }
  break;
  case SImode:
  if (push_operand (x1, SImode))
    { recog_operand[0] = x1; goto L143; }
 L145:
  if (general_operand (x1, SImode))
    { recog_operand[0] = x1; goto L146; }
 L174:
  if (push_operand (x1, SImode))
    { recog_operand[0] = x1; goto L175; }
 L189:
  if (general_operand (x1, SImode))
    { recog_operand[0] = x1; goto L190; }
 L796:
  if (GET_CODE (x1) == ZERO_EXTRACT && 1)
    goto L827;
 L802:
  if (general_operand (x1, SImode))
    { recog_operand[0] = x1; goto L803; }
  break;
  case DImode:
  if (push_operand (x1, DImode))
    { recog_operand[0] = x1; goto L5; }
 L171:
  if (general_operand (x1, DImode))
    { recog_operand[0] = x1; goto L172; }
  break;
  case SFmode:
  if (general_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L166; }
 L221:
  if (register_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L222; }
 L225:
  if (general_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L226; }
 L1256:
  if (register_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L1257; }
  break;
  case DFmode:
  if (push_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L2; }
 L168:
  if (general_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L169; }
 L241:
  if (register_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L242; }
 L245:
  if (general_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L246; }
 L1242:
  if (register_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L1243; }
  break;
  }
  goto ret0;
 L8:
  tem = recog_1 (x0, insn);
  if (tem >= 0) return tem;
  goto L1233;
 L162:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[0] = x2; goto L163; }
 L155:
  if (general_operand (x2, HImode))
    { recog_operand[0] = x2; goto L156; }
  goto L1233;
 L163:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, QImode))
    { recog_operand[1] = x1; return 35; }
 L336:
  if (GET_MODE (x1) != QImode)
    {
      x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
      goto L155;
    }
  if (GET_CODE (x1) == PLUS && 1)
    goto L337;
  if (GET_CODE (x1) == MINUS && 1)
    goto L390;
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L155;
 L337:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L338;
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L155;
 L338:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 84; }
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L155;
 L390:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L391;
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L155;
 L391:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; return 96; }
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L155;
 L156:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, HImode))
    { recog_operand[1] = x1; return 33; }
 L325:
  if (GET_MODE (x1) != HImode)
    {
      x1 = XEXP (x0, 0);
      goto L1233;
    }
  if (GET_CODE (x1) == PLUS && 1)
    goto L326;
  if (GET_CODE (x1) == MINUS && 1)
    goto L379;
  x1 = XEXP (x0, 0);
  goto L1233;
 L326:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L327;
  x1 = XEXP (x0, 0);
  goto L1233;
 L327:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 82; }
  x1 = XEXP (x0, 0);
  goto L1233;
 L379:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L380;
  x1 = XEXP (x0, 0);
  goto L1233;
 L380:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; return 94; }
  x1 = XEXP (x0, 0);
  goto L1233;
 L976:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == IF_THEN_ELSE && 1)
    goto L977;
  if (GET_CODE (x1) == LABEL_REF && 1)
    goto L1173;
  x1 = XEXP (x0, 0);
  goto L1233;
 L977:
  tem = recog_2 (x0, insn);
  if (tem >= 0) return tem;
  x1 = XEXP (x0, 0);
  goto L1233;
 L1173:
  x2 = XEXP (x1, 0);
  if (1)
    { recog_operand[0] = x2; return 241; }
  x1 = XEXP (x0, 0);
  goto L1233;
 L1234:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == CALL && 1)
    goto L1235;
  x1 = XEXP (x0, 0);
  goto L158;
 L1235:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; goto L1236; }
  x1 = XEXP (x0, 0);
  goto L158;
 L1236:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 246; }
  x1 = XEXP (x0, 0);
  goto L158;
 L159:
  tem = recog_3 (x0, insn);
  if (tem >= 0) return tem;
  goto L151;
 L152:
  return recog_4 (x0, insn);
 L143:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; if (GET_CODE (operands[1]) == CONST_INT
   && INTVAL (operands[1]) >= -0x8000
   && INTVAL (operands[1]) < 0x8000) return 29; }
  x1 = XEXP (x0, 0);
  goto L145;
 L146:
  x1 = XEXP (x0, 1);
  if (x1 == const0_rtx && 1)
    return 30;
 L149:
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; return 31; }
  x1 = XEXP (x0, 0);
  goto L174;
 L175:
  x1 = XEXP (x0, 1);
  if (address_operand (x1, SImode))
    { recog_operand[1] = x1; return 39; }
  x1 = XEXP (x0, 0);
  goto L189;
 L190:
  tem = recog_5 (x0, insn);
  if (tem >= 0) return tem;
  goto L796;
 L827:
  x2 = XEXP (x1, 0);
  if (nonimmediate_operand (x2, QImode))
    { recog_operand[0] = x2; goto L828; }
 L797:
  if (nonimmediate_operand (x2, SImode))
    { recog_operand[0] = x2; goto L798; }
  goto L802;
 L828:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L829; }
  x2 = XEXP (x1, 0);
  goto L797;
 L829:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L830; }
  x2 = XEXP (x1, 0);
  goto L797;
 L830:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == XOR && GET_MODE (x1) == SImode && 1)
    goto L831;
  if (x1 == const0_rtx && 1)
    if (TARGET_68020 && TARGET_BITFIELD) return 192;
 L847:
  if (GET_CODE (x1) == CONST_INT && XINT (x1, 0) == -1 && 1)
    if (TARGET_68020 && TARGET_BITFIELD) return 193;
 L853:
  if (general_operand (x1, SImode))
    { recog_operand[3] = x1; if (TARGET_68020 && TARGET_BITFIELD) return 194; }
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L797;
 L831:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == ZERO_EXTRACT && GET_MODE (x2) == SImode && 1)
    goto L832;
  goto L853;
 L832:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[0]) && 1)
    goto L833;
  goto L853;
 L833:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, recog_operand[1]) && 1)
    goto L834;
  goto L853;
 L834:
  x3 = XEXP (x2, 2);
  if (rtx_equal_p (x3, recog_operand[2]) && 1)
    goto L835;
  goto L853;
 L835:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, VOIDmode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[3]) == CONST_INT
   && (INTVAL (operands[3]) == -1
       || (GET_CODE (operands[1]) == CONST_INT
           && (~ INTVAL (operands[3]) & ((1 << INTVAL (operands[1]))- 1)) == 0))) return 191; }
  goto L853;
 L798:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[1] = x2; goto L799; }
 L869:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; goto L870; }
  goto L802;
 L799:
  x2 = XEXP (x1, 2);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; goto L800; }
  x2 = XEXP (x1, 1);
  goto L869;
 L800:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[3] = x1; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[1]) == CONST_INT
   && (INTVAL (operands[1]) == 8 || INTVAL (operands[1]) == 16)
   && GET_CODE (operands[2]) == CONST_INT
   && INTVAL (operands[2]) % INTVAL (operands[1]) == 0
   && (GET_CODE (operands[0]) == REG
       || ! mode_dependent_address_p (XEXP (operands[0], 0)))) return 186; }
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 1);
  goto L869;
 L870:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L871; }
  goto L802;
 L871:
  x1 = XEXP (x0, 1);
  if (x1 == const0_rtx && 1)
    if (TARGET_68020 && TARGET_BITFIELD) return 197;
 L877:
  if (GET_CODE (x1) == CONST_INT && XINT (x1, 0) == -1 && 1)
    if (TARGET_68020 && TARGET_BITFIELD) return 198;
 L883:
  if (general_operand (x1, SImode))
    { recog_operand[3] = x1; if (TARGET_68020 && TARGET_BITFIELD) return 199; }
  x1 = XEXP (x0, 0);
  goto L802;
 L803:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == ZERO_EXTRACT && GET_MODE (x1) == SImode && 1)
    goto L822;
  if (GET_CODE (x1) == SIGN_EXTRACT && GET_MODE (x1) == SImode && 1)
    goto L816;
 L1240:
  if (address_operand (x1, QImode))
    { recog_operand[1] = x1; return 248; }
  goto ret0;
 L822:
  x2 = XEXP (x1, 0);
  if (nonimmediate_operand (x2, QImode))
    { recog_operand[1] = x2; goto L823; }
 L804:
  if (nonimmediate_operand (x2, SImode))
    { recog_operand[1] = x2; goto L805; }
  goto L1240;
 L823:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L824; }
  x2 = XEXP (x1, 0);
  goto L804;
 L824:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD) return 190; }
  x2 = XEXP (x1, 0);
  goto L804;
 L805:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; goto L806; }
 L864:
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L865; }
  goto L1240;
 L806:
  x2 = XEXP (x1, 2);
  if (immediate_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[2]) == CONST_INT
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && GET_CODE (operands[3]) == CONST_INT
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0
   && (GET_CODE (operands[1]) == REG
       || ! mode_dependent_address_p (XEXP (operands[1], 0)))) return 187; }
  x2 = XEXP (x1, 1);
  goto L864;
 L865:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD) return 196; }
  goto L1240;
 L816:
  x2 = XEXP (x1, 0);
  if (nonimmediate_operand (x2, QImode))
    { recog_operand[1] = x2; goto L817; }
 L810:
  if (nonimmediate_operand (x2, SImode))
    { recog_operand[1] = x2; goto L811; }
  goto L1240;
 L817:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L818; }
  x2 = XEXP (x1, 0);
  goto L810;
 L818:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD) return 189; }
  x2 = XEXP (x1, 0);
  goto L810;
 L811:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; goto L812; }
 L858:
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; goto L859; }
  goto L1240;
 L812:
  x2 = XEXP (x1, 2);
  if (immediate_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD
   && GET_CODE (operands[2]) == CONST_INT
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && GET_CODE (operands[3]) == CONST_INT
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0
   && (GET_CODE (operands[1]) == REG
       || ! mode_dependent_address_p (XEXP (operands[1], 0)))) return 188; }
  x2 = XEXP (x1, 1);
  goto L858;
 L859:
  x2 = XEXP (x1, 2);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; if (TARGET_68020 && TARGET_BITFIELD) return 195; }
  goto L1240;
 L5:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DImode))
    { recog_operand[1] = x1; return 1; }
  x1 = XEXP (x0, 0);
  goto L171;
 L172:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DImode))
    { recog_operand[1] = x1; return 38; }
  goto ret0;
 L166:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SFmode))
    { recog_operand[1] = x1; return 36; }
  x1 = XEXP (x0, 0);
  goto L221;
 L222:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == FLOAT_TRUNCATE && GET_MODE (x1) == SFmode && 1)
    goto L223;
  x1 = XEXP (x0, 0);
  goto L225;
 L223:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 56; }
  x1 = XEXP (x0, 0);
  goto L225;
 L226:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SFmode)
    {
      x1 = XEXP (x0, 0);
      goto L1256;
    }
 switch (GET_CODE (x1))
  {
  case FLOAT_TRUNCATE:
  if (1)
    goto L227;
  break;
  case FLOAT:
  if (1)
    goto L235;
  break;
  case FIX:
  if (1)
    goto L271;
  break;
  case PLUS:
  if (1)
    goto L352;
  break;
  case MINUS:
  if (1)
    goto L405;
  break;
  case MULT:
  if (1)
    goto L455;
  break;
  case DIV:
  if (1)
    goto L505;
  break;
  case NEG:
  if (1)
    goto L626;
  break;
  case ABS:
  if (1)
    goto L642;
  break;
  }
  x1 = XEXP (x0, 0);
  goto L1256;
 L227:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881 && TARGET_IEEE_MATH) return 57; }
 L231:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 58; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L235:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 60; }
 L239:
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 61; }
 L251:
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 65; }
 L259:
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 67; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L271:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881 && !TARGET_68040) return 70; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L352:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L353; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L353:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 89; }
 L358:
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 90; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L405:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L406; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L406:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 101; }
 L411:
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 102; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L455:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L456; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L456:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 113; }
 L461:
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 114; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L505:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L506; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L506:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 125; }
 L511:
  if (general_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 126; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L626:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 148; }
 L630:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 149; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L642:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 154; }
 L646:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 155; }
  x1 = XEXP (x0, 0);
  goto L1256;
 L1257:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SFmode)
    goto ret0;
  if (GET_CODE (x1) == PLUS && 1)
    goto L1258;
  if (GET_CODE (x1) == MINUS && 1)
    goto L1279;
  if (GET_CODE (x1) == MULT && 1)
    goto L1314;
  goto ret0;
 L1258:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == SFmode && 1)
    goto L1259;
 L1265:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L1266; }
  goto ret0;
 L1259:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, SFmode))
    { recog_operand[1] = x3; goto L1260; }
  goto L1265;
 L1260:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1261; }
  goto L1265;
 L1261:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 252; }
  x2 = XEXP (x1, 0);
  goto L1265;
 L1266:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == SFmode && 1)
    goto L1267;
  goto ret0;
 L1267:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1268; }
  goto ret0;
 L1268:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 253; }
  goto ret0;
 L1279:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L1280; }
 L1293:
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == SFmode && 1)
    goto L1294;
  goto ret0;
 L1280:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == SFmode && 1)
    goto L1281;
  x2 = XEXP (x1, 0);
  goto L1293;
 L1281:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1282; }
  x2 = XEXP (x1, 0);
  goto L1293;
 L1282:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 255; }
  x2 = XEXP (x1, 0);
  goto L1293;
 L1294:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[1] = x3; goto L1295; }
  goto ret0;
 L1295:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1296; }
  goto ret0;
 L1296:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 257; }
  goto ret0;
 L1314:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SFmode && 1)
    goto L1315;
 L1321:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L1322; }
 L1342:
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == SFmode && 1)
    goto L1343;
 L1349:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L1350; }
  goto ret0;
 L1315:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[1] = x3; goto L1316; }
  goto L1321;
 L1316:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1317; }
  goto L1321;
 L1317:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 260; }
  x2 = XEXP (x1, 0);
  goto L1321;
 L1322:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SFmode && 1)
    goto L1323;
  x2 = XEXP (x1, 0);
  goto L1342;
 L1323:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1324; }
  x2 = XEXP (x1, 0);
  goto L1342;
 L1324:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 261; }
  x2 = XEXP (x1, 0);
  goto L1342;
 L1343:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[1] = x3; goto L1344; }
  goto L1349;
 L1344:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1345; }
  goto L1349;
 L1345:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 264; }
  x2 = XEXP (x1, 0);
  goto L1349;
 L1350:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == SFmode && 1)
    goto L1351;
  goto ret0;
 L1351:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SFmode))
    { recog_operand[2] = x3; goto L1352; }
  goto ret0;
 L1352:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 265; }
  goto ret0;
 L2:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DFmode))
    { recog_operand[1] = x1; return 0; }
  x1 = XEXP (x0, 0);
  goto L168;
 L169:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DFmode))
    { recog_operand[1] = x1; return 37; }
 L214:
  if (GET_CODE (x1) == FLOAT_EXTEND && GET_MODE (x1) == DFmode && 1)
    goto L215;
  x1 = XEXP (x0, 0);
  goto L241;
 L215:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 53; }
 L219:
  if (general_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 54; }
  x1 = XEXP (x0, 0);
  goto L241;
 L242:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == FLOAT && GET_MODE (x1) == DFmode && 1)
    goto L243;
  x1 = XEXP (x0, 0);
  goto L245;
 L243:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 63; }
  x1 = XEXP (x0, 0);
  goto L245;
 L246:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != DFmode)
    {
      x1 = XEXP (x0, 0);
      goto L1242;
    }
 switch (GET_CODE (x1))
  {
  case FLOAT:
  if (1)
    goto L247;
  break;
  case FIX:
  if (1)
    goto L267;
  break;
  case PLUS:
  if (1)
    goto L342;
  break;
  case MINUS:
  if (1)
    goto L395;
  break;
  case MULT:
  if (1)
    goto L445;
  break;
  case DIV:
  if (1)
    goto L495;
  break;
  case NEG:
  if (1)
    goto L634;
  break;
  case ABS:
  if (1)
    goto L650;
  break;
  }
  x1 = XEXP (x0, 0);
  goto L1242;
 L247:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 64; }
 L255:
  if (general_operand (x2, HImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 66; }
 L263:
  if (general_operand (x2, QImode))
    { recog_operand[1] = x2; if (TARGET_68881) return 68; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L267:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881 && !TARGET_68040) return 69; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L342:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L343; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L343:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 86; }
 L348:
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 87; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L395:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L396; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L396:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 98; }
 L401:
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 99; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L445:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L446; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L446:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 110; }
 L451:
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 111; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L495:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L496; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L496:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 122; }
 L501:
  if (general_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_68881) return 123; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L634:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 151; }
 L638:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 152; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L650:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 157; }
 L654:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_68881) return 158; }
  x1 = XEXP (x0, 0);
  goto L1242;
 L1243:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != DFmode)
    goto ret0;
  if (GET_CODE (x1) == PLUS && 1)
    goto L1244;
  if (GET_CODE (x1) == MINUS && 1)
    goto L1272;
  if (GET_CODE (x1) == MULT && 1)
    goto L1300;
  goto ret0;
 L1244:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == DFmode && 1)
    goto L1245;
 L1251:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L1252; }
  goto ret0;
 L1245:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, DFmode))
    { recog_operand[1] = x3; goto L1246; }
  goto L1251;
 L1246:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1247; }
  goto L1251;
 L1247:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 250; }
  x2 = XEXP (x1, 0);
  goto L1251;
 L1252:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == DFmode && 1)
    goto L1253;
  goto ret0;
 L1253:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1254; }
  goto ret0;
 L1254:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 251; }
  goto ret0;
 L1272:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L1273; }
 L1286:
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == DFmode && 1)
    goto L1287;
  goto ret0;
 L1273:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MULT && GET_MODE (x2) == DFmode && 1)
    goto L1274;
  x2 = XEXP (x1, 0);
  goto L1286;
 L1274:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1275; }
  x2 = XEXP (x1, 0);
  goto L1286;
 L1275:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 254; }
  x2 = XEXP (x1, 0);
  goto L1286;
 L1287:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[1] = x3; goto L1288; }
  goto ret0;
 L1288:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1289; }
  goto ret0;
 L1289:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 256; }
  goto ret0;
 L1300:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == DFmode && 1)
    goto L1301;
 L1307:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L1308; }
 L1328:
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == DFmode && 1)
    goto L1329;
 L1335:
  if (general_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L1336; }
  goto ret0;
 L1301:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[1] = x3; goto L1302; }
  goto L1307;
 L1302:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1303; }
  goto L1307;
 L1303:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 258; }
  x2 = XEXP (x1, 0);
  goto L1307;
 L1308:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == DFmode && 1)
    goto L1309;
  x2 = XEXP (x1, 0);
  goto L1328;
 L1309:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1310; }
  x2 = XEXP (x1, 0);
  goto L1328;
 L1310:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 259; }
  x2 = XEXP (x1, 0);
  goto L1328;
 L1329:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[1] = x3; goto L1330; }
  goto L1335;
 L1330:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1331; }
  goto L1335;
 L1331:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, DFmode))
    { recog_operand[3] = x2; if (TARGET_FPA) return 262; }
  x2 = XEXP (x1, 0);
  goto L1335;
 L1336:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MINUS && GET_MODE (x2) == DFmode && 1)
    goto L1337;
  goto ret0;
 L1337:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, DFmode))
    { recog_operand[2] = x3; goto L1338; }
  goto ret0;
 L1338:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[3] = x3; if (TARGET_FPA) return 263; }
  goto ret0;
 ret0: return -1;
}

int
recog_7 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L1157:
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SImode && 1)
    goto L1158;
 L1166:
  if (register_operand (x2, SImode))
    { recog_operand[0] = x2; goto L1167; }
 L1177:
  if (GET_CODE (x2) == IF_THEN_ELSE && 1)
    goto L1178;
  goto ret0;
 L1158:
  x3 = XEXP (x2, 0);
  if (x3 == pc_rtx && 1)
    goto L1159;
  goto L1166;
 L1159:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, HImode))
    { recog_operand[0] = x3; goto L1160; }
  goto L1166;
 L1160:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L1161;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1166;
 L1161:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1162;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1166;
 L1162:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[1] = x3; return 239; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1166;
 L1167:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L1168;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1177;
 L1168:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L1169;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1177;
 L1169:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[1] = x3; return 240; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L1177;
 L1178:
  x3 = XEXP (x2, 0);
  if (GET_CODE (x3) == NE && 1)
    goto L1179;
  if (GET_CODE (x3) == GE && 1)
    goto L1217;
  goto ret0;
 L1179:
  x4 = XEXP (x3, 0);
  if (GET_CODE (x4) == COMPARE && 1)
    goto L1180;
  goto ret0;
 L1180:
  x5 = XEXP (x4, 0);
  if (GET_CODE (x5) != PLUS)
    goto ret0;
  if (GET_MODE (x5) == HImode && 1)
    goto L1181;
  if (GET_MODE (x5) == SImode && 1)
    goto L1200;
  goto ret0;
 L1181:
  x6 = XEXP (x5, 0);
  if (general_operand (x6, HImode))
    { recog_operand[0] = x6; goto L1182; }
  goto ret0;
 L1182:
  x6 = XEXP (x5, 1);
  if (GET_CODE (x6) == CONST_INT && XINT (x6, 0) == -1 && 1)
    goto L1183;
  goto ret0;
 L1183:
  x5 = XEXP (x4, 1);
  if (GET_CODE (x5) == CONST_INT && XINT (x5, 0) == -1 && 1)
    goto L1184;
  goto ret0;
 L1184:
  x4 = XEXP (x3, 1);
  if (x4 == const0_rtx && 1)
    goto L1185;
  goto ret0;
 L1185:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1186;
  goto ret0;
 L1186:
  x4 = XEXP (x3, 0);
  if (1)
    { recog_operand[1] = x4; goto L1187; }
  goto ret0;
 L1187:
  x3 = XEXP (x2, 2);
  if (x3 == pc_rtx && 1)
    goto L1188;
  goto ret0;
 L1188:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1189;
  goto ret0;
 L1189:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L1190;
  goto ret0;
 L1190:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == HImode && 1)
    goto L1191;
  goto ret0;
 L1191:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[0]) && 1)
    goto L1192;
  goto ret0;
 L1192:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == -1 && 1)
    return 242;
  goto ret0;
 L1200:
  x6 = XEXP (x5, 0);
  if (general_operand (x6, SImode))
    { recog_operand[0] = x6; goto L1201; }
  goto ret0;
 L1201:
  x6 = XEXP (x5, 1);
  if (GET_CODE (x6) == CONST_INT && XINT (x6, 0) == -1 && 1)
    goto L1202;
  goto ret0;
 L1202:
  x5 = XEXP (x4, 1);
  if (GET_CODE (x5) == CONST_INT && XINT (x5, 0) == -1 && 1)
    goto L1203;
  goto ret0;
 L1203:
  x4 = XEXP (x3, 1);
  if (x4 == const0_rtx && 1)
    goto L1204;
  goto ret0;
 L1204:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1205;
  goto ret0;
 L1205:
  x4 = XEXP (x3, 0);
  if (1)
    { recog_operand[1] = x4; goto L1206; }
  goto ret0;
 L1206:
  x3 = XEXP (x2, 2);
  if (x3 == pc_rtx && 1)
    goto L1207;
  goto ret0;
 L1207:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1208;
  goto ret0;
 L1208:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L1209;
  goto ret0;
 L1209:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SImode && 1)
    goto L1210;
  goto ret0;
 L1210:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[0]) && 1)
    goto L1211;
  goto ret0;
 L1211:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == -1 && 1)
    return 243;
  goto ret0;
 L1217:
  x4 = XEXP (x3, 0);
  if (GET_CODE (x4) == PLUS && GET_MODE (x4) == SImode && 1)
    goto L1218;
  goto ret0;
 L1218:
  x5 = XEXP (x4, 0);
  if (general_operand (x5, SImode))
    { recog_operand[0] = x5; goto L1219; }
  goto ret0;
 L1219:
  x5 = XEXP (x4, 1);
  if (GET_CODE (x5) == CONST_INT && XINT (x5, 0) == -1 && 1)
    goto L1220;
  goto ret0;
 L1220:
  x4 = XEXP (x3, 1);
  if (x4 == const0_rtx && 1)
    goto L1221;
  goto ret0;
 L1221:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == LABEL_REF && 1)
    goto L1222;
  goto ret0;
 L1222:
  x4 = XEXP (x3, 0);
  if (1)
    { recog_operand[1] = x4; goto L1223; }
  goto ret0;
 L1223:
  x3 = XEXP (x2, 2);
  if (x3 == pc_rtx && 1)
    goto L1224;
  goto ret0;
 L1224:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L1225;
  goto ret0;
 L1225:
  x2 = XEXP (x1, 0);
  if (rtx_equal_p (x2, recog_operand[0]) && 1)
    goto L1226;
  goto ret0;
 L1226:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SImode && 1)
    goto L1227;
  goto ret0;
 L1227:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[0]) && 1)
    goto L1228;
  goto ret0;
 L1228:
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT && XINT (x3, 0) == -1 && 1)
    if (find_reg_note (insn, REG_NONNEG, 0)) return 244;
  goto ret0;
 ret0: return -1;
}

int
recog (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L0:
 switch (GET_CODE (x0))
  {
  case SET:
  if (1)
    goto L7;
  break;
  case PARALLEL:
  if (XVECLEN (x0, 0) == 2 && 1)
    goto L16;
  break;
  case CALL:
  if (1)
    goto L1230;
  break;
  case CONST_INT:
  if (x0 == const0_rtx && 1)
    return 247;
  break;
  }
  goto ret0;
 L7:
  return recog_6 (x0, insn);
 L16:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == SET && 1)
    goto L17;
  goto ret0;
 L17:
  x2 = XEXP (x1, 0);
  if (x2 == cc0_rtx && 1)
    goto L18;
  if (x2 == pc_rtx && 1)
    goto L1157;
 L534:
  if (general_operand (x2, SImode))
    { recog_operand[0] = x2; goto L535; }
  goto ret0;
 L18:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SFmode))
    { recog_operand[0] = x2; goto L19; }
 L27:
  if (general_operand (x2, DFmode))
    { recog_operand[0] = x2; goto L28; }
 L51:
  if (GET_CODE (x2) == COMPARE && 1)
    goto L52;
  x2 = XEXP (x1, 0);
  goto L534;
 L19:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L20;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L27;
 L20:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 6; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L27;
 L28:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L29;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L51;
 L29:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[1] = x2; if (TARGET_FPA) return 9; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  goto L51;
 L52:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, DFmode))
    { recog_operand[0] = x3; goto L53; }
 L65:
  if (general_operand (x3, SFmode))
    { recog_operand[0] = x3; goto L66; }
  x2 = XEXP (x1, 0);
  goto L534;
 L53:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, DFmode))
    { recog_operand[1] = x3; goto L54; }
  x3 = XEXP (x2, 0);
  goto L65;
 L54:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L55;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L65;
 L55:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 15; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  goto L65;
 L66:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SFmode))
    { recog_operand[1] = x3; goto L67; }
  x2 = XEXP (x1, 0);
  goto L534;
 L67:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == CLOBBER && 1)
    goto L68;
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L534;
 L68:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (TARGET_FPA) return 18; }
  x1 = XVECEXP (x0, 0, 0);
  x2 = XEXP (x1, 0);
  goto L534;
 L1157:
  tem = recog_7 (x0, insn);
  if (tem >= 0) return tem;
  goto L534;
 L535:
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != SImode)
    goto ret0;
  if (GET_CODE (x2) == DIV && 1)
    goto L536;
  if (GET_CODE (x2) == UDIV && 1)
    goto L547;
  goto ret0;
 L536:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L537; }
  goto ret0;
 L537:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; goto L538; }
  goto ret0;
 L538:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L539;
  goto ret0;
 L539:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; goto L540; }
  goto ret0;
 L540:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == MOD && GET_MODE (x2) == SImode && 1)
    goto L541;
  goto ret0;
 L541:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[1]) && 1)
    goto L542;
  goto ret0;
 L542:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, recog_operand[2]) && 1)
    if (TARGET_68020) return 131;
  goto ret0;
 L547:
  x3 = XEXP (x2, 0);
  if (general_operand (x3, SImode))
    { recog_operand[1] = x3; goto L548; }
  goto ret0;
 L548:
  x3 = XEXP (x2, 1);
  if (general_operand (x3, SImode))
    { recog_operand[2] = x3; goto L549; }
  goto ret0;
 L549:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == SET && 1)
    goto L550;
  goto ret0;
 L550:
  x2 = XEXP (x1, 0);
  if (general_operand (x2, SImode))
    { recog_operand[3] = x2; goto L551; }
  goto ret0;
 L551:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == UMOD && GET_MODE (x2) == SImode && 1)
    goto L552;
  goto ret0;
 L552:
  x3 = XEXP (x2, 0);
  if (rtx_equal_p (x3, recog_operand[1]) && 1)
    goto L553;
  goto ret0;
 L553:
  x3 = XEXP (x2, 1);
  if (rtx_equal_p (x3, recog_operand[2]) && 1)
    if (TARGET_68020) return 132;
  goto ret0;
 L1230:
  x1 = XEXP (x0, 0);
  if (general_operand (x1, QImode))
    { recog_operand[0] = x1; goto L1231; }
  goto ret0;
 L1231:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; return 245; }
  goto ret0;
 ret0: return -1;
}
