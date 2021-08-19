/* Opcode table for m68000/m68020 and m68881.  */

struct m68k_opcode
{
  char *name;
  unsigned long opcode;
  unsigned long  match;
  char *args;
  char *cpus;
};

/* We store four bytes of opcode for all opcodes because that
   is the most any of them need.  The actual length of an instruction
   is always at least 2 bytes, and is as much longer as necessary to
   hold the operands it has.

   The match component is a mask saying which bits must match
   particular opcode in order for an instruction to be an instance
   of that opcode.

   The args component is a string containing two characters
   for each operand of the instruction.  The first specifies
   the kind of operand; the second, the place it is stored.  */

/* Kinds of operands:
   D  data register only.  Stored as 3 bits.
   A  address register only.  Stored as 3 bits.
   R  either kind of register.  Stored as 4 bits.
   F  floating point coprocessor register only.   Stored as 3 bits.
   O  an offset (or width): immediate data 0-31 or data register.
      Stored as 6 bits in special format for BF... insns.
   +  autoincrement only.  Stored as 3 bits (number of the address register).
   -  autodecrement only.  Stored as 3 bits (number of the address register).
   0  indirect only.  Stored as 3 bits (number of the address register).
   Q  quick immediate data.  Stored as 3 bits.
      This matches an immediate operand only when value is in range 1 .. 8.
   M  moveq immediate data.  Stored as 8 bits.
      This matches an immediate operand only when value is in range -128..127
   T  trap vector immediate data.  Stored as 4 bits.

   k  K-factor for fmove.p instruction.   Stored as a 7-bit constant or
      a three bit register offset, depending on the field type.

   #  immediate data.  Stored in special places (b, w or l)
      which say how many bits to store.
   ^  immediate data for floating point instructions.   Special places
      are offset by 2 bytes from '#'...
   B  pc-relative address, converted to an offset
      that is treated as immediate data.
   d  displacement and register.  Stores the register as 3 bits
      and stores the displacement in the entire second word.

   C  the CCR.  No need to store it; this is just for filtering validity.
   S  the SR.  No need to store, just as with CCR.
   U  the USP.  No need to store, just as with CCR.

   I  Coprocessor ID.   Not printed if 1.   The Coprocessor ID is always
      extracted from the 'd' field of word one, which means that an extended
      coprocessor opcode can be skipped using the 'i' place, if needed.

   s  System Control register for the floating point coprocessor.
   S  List of system control registers for floating point coprocessor.

   J  Misc register for movec instruction, stored in 'j' format.
	Possible values:
	000	SFC	Source Function Code reg
	001	DFC	Data Function Code reg
	002	CACR	Cache Control Register
	800	USP	User Stack Pointer
	801	VBR	Vector Base reg
	802	CAAR	Cache Address Register
	803	MSP	Master Stack Pointer
	804	ISP	Interrupt Stack Pointer

    L  Register list of the type d0-d7/a0-a7 etc.
       (New!  Improved!  Can also hold fp0-fp7, as well!)
       The assembler tries to see if the registers match the insn by
       looking at where the insn wants them stored.

    l  Register list like L, but with all the bits reversed.
       Used for going the other way. . .

 They are all stored as 6 bits using an address mode and a register number;
 they differ in which addressing modes they match.

   *  all					(modes 0-6,7.*)
   ~  alterable memory				(modes 2-6,7.0,7.1)(not 0,1,7.~)
   %  alterable					(modes 0-6,7.0,7.1)(not 7.~)
   ;  data					(modes 0,2-6,7.*)(not 1)
   @  data, but not immediate			(modes 0,2-6,7.???)(not 1,7.?)  This may really be ;, the 68020 book says it is
   !  control					(modes 2,5,6,7.*-)(not 0,1,3,4,7.4)
   &  alterable control				(modes 2,5,6,7.0,7.1)(not 0,1,7.???)
   $  alterable data				(modes 0,2-6,7.0,7.1)(not 1,7.~)
   ?  alterable control, or data register	(modes 0,2,5,6,7.0,7.1)(not 1,3,4,7.~)
   /  control, or data register			(modes 0,2,5,6,7.0,7.1,7.2,7.3)(not 1,3,4,7.4)
*/

/* Places to put an operand, for non-general operands:
   s  source, low bits of first word.
   d  dest, shifted 9 in first word
   1  second word, shifted 12
   2  second word, shifted 6
   3  second word, shifted 0
   4  third word, shifted 12
   5  third word, shifted 6
   6  third word, shifted 0
   7  second word, shifted 7
   8  second word, shifted 10
   D  store in both place 1 and place 3; for divul and divsl.
   b  second word, low byte
   w  second word (entire)
   l  second and third word (entire)
   g  branch offset for bra and similar instructions.
      The place to store depends on the magnitude of offset.
   t  store in both place 7 and place 8; for floating point operations
   c  branch offset for cpBcc operations.
      The place to store is word two if bit six of word one is zero,
      and words two and three if bit six of word one is one.
   i  Increment by two, to skip over coprocessor extended operands.   Only
      works with the 'I' format.
   k  Dynamic K-factor field.   Bits 6-4 of word 2, used as a register number.
      Also used for dynamic fmovem instruction.
   C  floating point coprocessor constant - 7 bits.  Also used for static
      K-factors...
   j  Movec register #, stored in 12 low bits of second word.

 Places to put operand, for general operands:
   d  destination, shifted 6 bits in first word
   b  source, at low bit of first word, and immediate uses one byte
   w  source, at low bit of first word, and immediate uses two bytes
   l  source, at low bit of first word, and immediate uses four bytes
   s  source, at low bit of first word.
      Used sometimes in contexts where immediate is not allowed anyway.
   f  single precision float, low bit of 1st word, immediate uses 4 bytes
   F  double precision float, low bit of 1st word, immediate uses 8 bytes
   x  extended precision float, low bit of 1st word, immediate uses 12 bytes
   p  packed float, low bit of 1st word, immediate uses 12 bytes
*/

/* The cpus string indicates which members of the 68k processor family
   implement the instruction.  It also indicates which instructions are
   emulated in software on the 68040, and which instructions are privileged.
   
   Here are the possible values:
   
   *  Instruction is available on entire 68k family
   0  Instruction is available on the MC68000/8
   1  Instruction is available on the MC68010
   2  Instruction is available on the MC68020
   3  Instruction is available on the MC68030
   4  Instruction is available on the MC68040
   8  Instruction is available on the MC68881/2
   e  Instruction is emulated in software on the MC68040
   p  Instruction is privileged
*/

#define one(x) ((x) << 16)
#define two(x, y) (((x) << 16) + y)

/*
   The assembler requires that all instances of the same mnemonic must be
   consecutive.  If they aren't, the assembler will bomb at runtime
 */
struct m68k_opcode m68k_opcodes[] =
{
{"abcd",	one(0140400),		one(0170770),		"DsDd", 	"*"},
{"abcd",	one(0140410),		one(0170770),		"-s-d", 	"*"},

		/* Add instructions */
{"addal",	one(0150700),		one(0170700),		"*lAd", 	"*"},
{"addaw",	one(0150300),		one(0170700),		"*wAd", 	"*"},
{"addib",	one(0003000),		one(0177700),		"#b$b", 	"*"},
{"addil",	one(0003200),		one(0177700),		"#l$l", 	"*"},
{"addiw",	one(0003100),		one(0177700),		"#w$w", 	"*"},
{"addqb",	one(0050000),		one(0170700),		"Qd$b", 	"*"},
{"addql",	one(0050200),		one(0170700),		"Qd%l", 	"*"},
{"addqw",	one(0050100),		one(0170700),		"Qd%w", 	"*"},

{"addb",	one(0050000),		one(0170700),		"Qd$b", 	"*"},	/* addq written as add */
{"addb",	one(0003000),		one(0177700),		"#b$b", 	"*"},	/* addi written as add */
{"addb",	one(0150000),		one(0170700),		";bDd", 	"*"},	/* addb <ea>,	Dd */
{"addb",	one(0150400),		one(0170700),		"Dd~b", 	"*"},	/* addb Dd,	<ea> */

{"addw",	one(0050100),		one(0170700),		"Qd%w", 	"*"},	/* addq written as add */
{"addw",	one(0003100),		one(0177700),		"#w$w", 	"*"},	/* addi written as add */
{"addw",	one(0150300),		one(0170700),		"*wAd", 	"*"},	/* adda written as add */
{"addw",	one(0150100),		one(0170700),		"*wDd", 	"*"},	/* addw <ea>,	Dd */
{"addw",	one(0150500),		one(0170700),		"Dd~w", 	"*"},	/* addw Dd,	<ea> */

{"addl",	one(0050200),		one(0170700),		"Qd%l", 	"*"},	/* addq written as add */
{"addl",	one(0003200),		one(0177700),		"#l$l", 	"*"},	/* addi written as add */
{"addl",	one(0150700),		one(0170700),		"*lAd", 	"*"},	/* adda written as add */
{"addl",	one(0150200),		one(0170700),		"*lDd", 	"*"},	/* addl <ea>,	Dd */
{"addl",	one(0150600),		one(0170700),		"Dd~l", 	"*"},	/* addl Dd,	<ea> */

{"addxb",	one(0150400),		one(0170770),		"DsDd", 	"*"},
{"addxb",	one(0150410),		one(0170770),		"-s-d", 	"*"},
{"addxl",	one(0150600),		one(0170770),		"DsDd", 	"*"},
{"addxl",	one(0150610),		one(0170770),		"-s-d", 	"*"},
{"addxw",	one(0150500),		one(0170770),		"DsDd", 	"*"},
{"addxw",	one(0150510),		one(0170770),		"-s-d", 	"*"},

{"andib",	one(0001000),		one(0177700),		"#b$b", 	"*"},
{"andib",	one(0001074),		one(0177777),		"#bCb", 	"*"},	/* andi to ccr */
{"andiw",	one(0001100),		one(0177700),		"#w$w", 	"*"},
{"andiw",	one(0001174),		one(0177777),		"#wSw", 	"*p"},	/* andi to sr */
{"andil",	one(0001200),		one(0177700),		"#l$l", 	"*"},

{"andb",	one(0001000),		one(0177700),		"#b$b", 	"*"},	/* andi written as or */
{"andb",	one(0001074),		one(0177777),		"#bCb", 	"*"},	/* andi to ccr */
{"andb",	one(0140000),		one(0170700),		";bDd", 	"*"},	/* memory to register */
{"andb",	one(0140400),		one(0170700),		"Dd~b", 	"*"},	/* register to memory */
{"andw",	one(0001100),		one(0177700),		"#w$w", 	"*"},	/* andi written as or */
{"andw",	one(0001174),		one(0177777),		"#wSw", 	"*"},	/* andi to sr */
{"andw",	one(0140100),		one(0170700),		";wDd", 	"*"},	/* memory to register */
{"andw",	one(0140500),		one(0170700),		"Dd~w", 	"*"},	/* register to memory */
{"andl",	one(0001200),		one(0177700),		"#l$l", 	"*"},	/* andi written as or */
{"andl",	one(0140200),		one(0170700),		";lDd", 	"*"},	/* memory to register */
{"andl",	one(0140600),		one(0170700),		"Dd~l", 	"*"},	/* register to memory */

{"aslb",	one(0160400),		one(0170770),		"QdDs", 	"*"},
{"aslb",	one(0160440),		one(0170770),		"DdDs", 	"*"},
{"asll",	one(0160600),		one(0170770),		"QdDs", 	"*"},
{"asll",	one(0160640),		one(0170770),		"DdDs", 	"*"},
{"aslw",	one(0160500),		one(0170770),		"QdDs", 	"*"},
{"aslw",	one(0160540),		one(0170770),		"DdDs", 	"*"},
{"aslw",	one(0160700),		one(0177700),		"~s", 		"*"},	/* Shift memory */
{"asrb",	one(0160000),		one(0170770),		"QdDs", 	"*"},
{"asrb",	one(0160040),		one(0170770),		"DdDs", 	"*"},
{"asrl",	one(0160200),		one(0170770),		"QdDs", 	"*"},
{"asrl",	one(0160240),		one(0170770),		"DdDs", 	"*"},
{"asrw",	one(0160100),		one(0170770),		"QdDs", 	"*"},
{"asrw",	one(0160140),		one(0170770),		"DdDs", 	"*"},
{"asrw",	one(0160300),		one(0177700),		"~s", 		"*"},	/* Shift memory */

{"bhi",		one(0061000),		one(0177400),		"Bg", 		"*"},
{"bls",		one(0061400),		one(0177400),		"Bg", 		"*"},
{"bcc",		one(0062000),		one(0177400),		"Bg", 		"*"},
{"bcs",		one(0062400),		one(0177400),		"Bg", 		"*"},
{"bne",		one(0063000),		one(0177400),		"Bg", 		"*"},
{"beq",		one(0063400),		one(0177400),		"Bg", 		"*"},
{"bvc",		one(0064000),		one(0177400),		"Bg", 		"*"},
{"bvs",		one(0064400),		one(0177400),		"Bg", 		"*"},
{"bpl",		one(0065000),		one(0177400),		"Bg", 		"*"},
{"bmi",		one(0065400),		one(0177400),		"Bg", 		"*"},
{"bge",		one(0066000),		one(0177400),		"Bg", 		"*"},
{"blt",		one(0066400),		one(0177400),		"Bg", 		"*"},
{"bgt",		one(0067000),		one(0177400),		"Bg", 		"*"},
{"ble",		one(0067400),		one(0177400),		"Bg", 		"*"},

{"bchg",	one(0000500),		one(0170700),		"Dd$s", 	"*"},
{"bchg",	one(0004100),		one(0177700),		"#b$s", 	"*"},
{"bclr",	one(0000600),		one(0170700),		"Dd$s", 	"*"},
{"bclr",	one(0004200),		one(0177700),		"#b$s", 	"*"},
{"bfchg",	two(0165300, 0),	two(0177700, 0170000),	"?sO2O3", 	"234"},
{"bfclr",	two(0166300, 0),	two(0177700, 0170000),	"?sO2O3", 	"234"},
{"bfexts",	two(0165700, 0),	two(0177700, 0100000),	"/sO2O3D1", 	"234"},
{"bfextu",	two(0164700, 0),	two(0177700, 0100000),	"/sO2O3D1", 	"234"},
{"bfffo",	two(0166700, 0),	two(0177700, 0100000),	"/sO2O3D1", 	"234"},
{"bfins",	two(0167700, 0),	two(0177700, 0100000),	"D1?sO2O3", 	"234"},
{"bfset",	two(0167300, 0),	two(0177700, 0170000),	"?sO2O3", 	"234"},
{"bftst",	two(0164300, 0),	two(0177700, 0170000),	"/sO2O3", 	"234"},
{"bset",	one(0000700),		one(0170700),		"Dd$s", 	"*"},
{"bset",	one(0004300),		one(0177700),		"#b$s", 	"*"},
{"btst",	one(0000400),		one(0170700),		"Dd@s", 	"*"},
{"btst",	one(0004000),		one(0177700),		"#b@s", 	"*"},

{"bkpt",	one(0044110),		one(0177770),		"Qs", 		"1234"},
{"bra",		one(0060000),		one(0177400),		"Bg", 		"*"},
{"bras",	one(0060000),		one(0177400),		"Bg", 		"*"},
{"bsr",		one(0060400),		one(0177400),		"Bg", 		"*"},
{"bsrs",	one(0060400),		one(0177400),		"Bg", 		"*"},

{"callm",	one(0003300),		one(0177700),		"#b!s", 	"2"},
{"cas2l",	two(0007374, 0),	two(0177777, 0107070),	"D3D6D2D5R1R4", "234"},	/* JF FOO this is really a 3 word ins */
{"cas2w",	two(0006374, 0),	two(0177777, 0107070),	"D3D6D2D5R1R4", "234"},	/* JF ditto */
{"casb",	two(0005300, 0),	two(0177700, 0177070),	"D3D2~s", 	"234"},
{"casl",	two(0007300, 0),	two(0177700, 0177070),	"D3D2~s", 	"234"},
{"casw",	two(0006300, 0),	two(0177700, 0177070),	"D3D2~s", 	"234"},

/*  {"chk",	one(0040600),		one(0170700),		";wDd", 	"*"}, JF FOO this looks wrong */
{"chk2b",	two(0000300, 0004000),	two(0177700, 07777),	"!sR1", 	"234"},
{"chk2l",	two(0002300, 0004000),	two(0177700, 07777),	"!sR1", 	"234"},
{"chk2w",	two(0001300, 0004000),	two(0177700, 07777),	"!sR1", 	"234"},
{"chkl",	one(0040400),		one(0170700),		";lDd", 	"*"},
{"chkw",	one(0040600),		one(0170700),		";wDd", 	"*"},
{"clrb",	one(0041000),		one(0177700),		"$s", 		"*"},
{"clrl",	one(0041200),		one(0177700),		"$s", 		"*"},
{"clrw",	one(0041100),		one(0177700),		"$s", 		"*"},

{"cmp2b",	two(0000300, 0),	two(0177700, 07777),	"!sR1", 	"234"},
{"cmp2l",	two(0002300, 0),	two(0177700, 07777),	"!sR1", 	"234"},
{"cmp2w",	two(0001300, 0),	two(0177700, 07777),	"!sR1", 	"234"},
{"cmpal",	one(0130700),		one(0170700),		"*lAd", 	"*"},
{"cmpaw",	one(0130300),		one(0170700),		"*wAd", 	"*"},
{"cmpib",	one(0006000),		one(0177700),		"#b;b", 	"*"},
{"cmpil",	one(0006200),		one(0177700),		"#l;l", 	"*"},
{"cmpiw",	one(0006100),		one(0177700),		"#w;w", 	"*"},
{"cmpb",	one(0006000),		one(0177700),		"#b;b", 	"*"},	/* cmpi written as cmp */
{"cmpb",	one(0130000),		one(0170700),		";bDd", 	"*"},
{"cmpw",	one(0006100),		one(0177700),		"#w;w", 	"*"},
{"cmpw",	one(0130100),		one(0170700),		"*wDd", 	"*"},
{"cmpw",	one(0130300),		one(0170700),		"*wAd", 	"*"},	/* cmpa written as cmp */
{"cmpl",	one(0006200),		one(0177700),		"#l;l", 	"*"},
{"cmpl",	one(0130200),		one(0170700),		"*lDd", 	"*"},
{"cmpl",	one(0130700),		one(0170700),		"*lAd", 	"*"},
{"cmpmb",	one(0130410),		one(0170770),		"+s+d", 	"*"},
{"cmpml",	one(0130610),		one(0170770),		"+s+d", 	"*"},
{"cmpmw",	one(0130510),		one(0170770),		"+s+d", 	"*"},

{"dbcc",	one(0052310),		one(0177770),		"DsBw", 	"*"},
{"dbcs",	one(0052710),		one(0177770),		"DsBw", 	"*"},
{"dbeq",	one(0053710),		one(0177770),		"DsBw", 	"*"},
{"dbf",		one(0050710),		one(0177770),		"DsBw", 	"*"},
{"dbge",	one(0056310),		one(0177770),		"DsBw", 	"*"},
{"dbgt",	one(0057310),		one(0177770),		"DsBw", 	"*"},
{"dbhi",	one(0051310),		one(0177770),		"DsBw", 	"*"},
{"dble",	one(0057710),		one(0177770),		"DsBw", 	"*"},
{"dbls",	one(0051710),		one(0177770),		"DsBw", 	"*"},
{"dblt",	one(0056710),		one(0177770),		"DsBw", 	"*"},
{"dbmi",	one(0055710),		one(0177770),		"DsBw", 	"*"},
{"dbne",	one(0053310),		one(0177770),		"DsBw", 	"*"},
{"dbpl",	one(0055310),		one(0177770),		"DsBw", 	"*"},
{"dbra",	one(0050710),		one(0177770),		"DsBw", 	"*"},
{"dbt",		one(0050310),		one(0177770),		"DsBw", 	"*"},
{"dbvc",	one(0054310),		one(0177770),		"DsBw", 	"*"},
{"dbvs",	one(0054710),		one(0177770),		"DsBw", 	"*"},

{"divsl",	two(0046100, 0006000),	two(0177700, 0107770),	";lD3D1", 	"*"},
{"divsl",	two(0046100, 0004000),	two(0177700, 0107770),	";lDD", 	"*"},
{"divsll",	two(0046100, 0004000),	two(0177700, 0107770),	";lD3D1", 	"234"},
{"divsw",	one(0100700),		one(0170700),		";wDd", 	"*"},
{"divs",	one(0100700),		one(0170700),		";wDd", 	"*"},
{"divul",	two(0046100, 0002000),	two(0177700, 0107770),	";lD3D1", 	"*"},
{"divul",	two(0046100, 0000000),	two(0177700, 0107770),	";lDD", 	"*"},
{"divull",	two(0046100, 0000000),	two(0177700, 0107770),	";lD3D1", 	"234"},
{"divuw",	one(0100300),		one(0170700),		";wDd", 	"*"},
{"divu",	one(0100300),		one(0170700),		";wDd", 	"*"},
{"eorb",	one(0005000),		one(0177700),		"#b$s", 	"*"},	/* eori written as eor */
{"eorb",	one(0005074),		one(0177777),		"#bCs", 	"*"},	/* eori to ccr */
{"eorb",	one(0130400),		one(0170700),		"Dd$s", 	"*"},	/* register to memory */
{"eorib",	one(0005000),		one(0177700),		"#b$s", 	"*"},
{"eorib",	one(0005074),		one(0177777),		"#bCs", 	"*"},	/* eori to ccr */
{"eoril",	one(0005200),		one(0177700),		"#l$s", 	"*"},
{"eoriw",	one(0005100),		one(0177700),		"#w$s", 	"*"},
{"eoriw",	one(0005174),		one(0177777),		"#wSs", 	"*"},	/* eori to sr */
{"eorl",	one(0005200),		one(0177700),		"#l$s", 	"*"},
{"eorl",	one(0130600),		one(0170700),		"Dd$s", 	"*"},
{"eorw",	one(0005100),		one(0177700),		"#w$s", 	"*"},
{"eorw",	one(0005174),		one(0177777),		"#wSs", 	"*p"},	/* eori to sr */
{"eorw",	one(0130500),		one(0170700),		"Dd$s", 	"*"},

{"exg",		one(0140500),		one(0170770),		"DdDs", 	"*"},
{"exg",		one(0140510),		one(0170770),		"AdAs", 	"*"},
{"exg",		one(0140610),		one(0170770),		"DdAs", 	"*"},
{"exg",		one(0140610),		one(0170770),		"AsDd", 	"*"},

{"extw",	one(0044200),		one(0177770),		"Ds", 		"*"},
{"extl",	one(0044300),		one(0177770),		"Ds", 		"*"},
{"extbl",	one(0044700),		one(0177770),		"Ds", 		"234"},
{"extb.l",	one(0044700),		one(0177770),		"Ds", 		"234"},	/* Not sure we should support this one*/

{"illegal",	one(0045374),		one(0177777),		"", 		"*"},
{"jmp",		one(0047300),		one(0177700),		"!s", 		"*"},
{"jsr",		one(0047200),		one(0177700),		"!s", 		"*"},
{"lea",		one(0040700),		one(0170700),		"!sAd", 	"*"},
{"linkw",	one(0047120),		one(0177770),		"As#w", 	"*"},
{"linkl",	one(0044010),		one(0177770),		"As#l", 	"*"},
{"link",	one(0047120),		one(0177770),		"As#w", 	"*"},
{"link",	one(0044010),		one(0177770),		"As#l", 	"*"},

{"lslb",	one(0160410),		one(0170770),		"QdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lslb",	one(0160450),		one(0170770),		"DdDs", 	"*"},	/* lsrb Dd,	Ds */
{"lslw",	one(0160510),		one(0170770),		"QdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lslw",	one(0160550),		one(0170770),		"DdDs", 	"*"},	/* lsrb Dd,	Ds */
{"lslw",	one(0161700),		one(0177700),		"~s", 		"*"},	/* Shift memory */
{"lsll",	one(0160610),		one(0170770),		"QdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lsll",	one(0160650),		one(0170770),		"DdDs", 	"*"},	/* lsrb Dd,	Ds */

{"lsrb",	one(0160010),		one(0170770),		"QdDs", 	"*"}, 	/* lsrb #Q,	Ds */
{"lsrb",	one(0160050),		one(0170770),		"DdDs", 	"*"},	/* lsrb Dd,	Ds */
{"lsrl",	one(0160210),		one(0170770),		"QdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lsrl",	one(0160250),		one(0170770),		"DdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lsrw",	one(0160110),		one(0170770),		"QdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lsrw",	one(0160150),		one(0170770),		"DdDs", 	"*"},	/* lsrb #Q,	Ds */
{"lsrw",	one(0161300),		one(0177700),		"~s", 		"*"},	/* Shift memory */

{"moveal",	one(0020100),		one(0170700),		"*lAd", 	"*"},
{"moveaw",	one(0030100),		one(0170700),		"*wAd", 	"*"},
{"moveb",	one(0010000),		one(0170000),		";b$d", 	"*"},	/* move */
{"movel",	one(0070000),		one(0170400),		"MsDd", 	"*"},	/* moveq written as move */
{"movel",	one(0020000),		one(0170000),		"*l$d", 	"*"},
{"movel",	one(0020100),		one(0170700),		"*lAd", 	"*"},
{"movel",	one(0047140),		one(0177770),		"AsUd", 	"*p"},	/* move to USP */
{"movel",	one(0047150),		one(0177770),		"UdAs", 	"*p"},	/* move from USP */

{"movec",	one(0047173),		one(0177777),		"R1Jj", 	"234p"},
{"movec",	one(0047173),		one(0177777),		"R1#j", 	"234p"},
{"movec",	one(0047172),		one(0177777),		"JjR1", 	"234p"},
{"movec",	one(0047172),		one(0177777),		"#jR1", 	"234p"},

{"moveml",	one(0044300),		one(0177700),		"#w&s", 	"*"},	/* movem reg to mem. */
{"moveml",	one(0044340),		one(0177770),		"#w-s", 	"*"},	/* movem reg to autodecrement. */
{"moveml",	one(0046300),		one(0177700),		"!s#w", 	"*"},	/* movem mem to reg. */
{"moveml",	one(0046330),		one(0177770),		"+s#w", 	"*"},	/* movem autoinc to reg. */
/* JF added these next four for the assembler */
{"moveml",	one(0044300),		one(0177700),		"Lw&s", 	"*"},	/* movem reg to mem. */
{"moveml",	one(0044340),		one(0177770),		"lw-s", 	"*"},	/* movem reg to autodecrement. */
{"moveml",	one(0046300),		one(0177700),		"!sLw", 	"*"},	/* movem mem to reg. */
{"moveml",	one(0046330),		one(0177770),		"+sLw", 	"*"},	/* movem autoinc to reg. */

{"movemw",	one(0044200),		one(0177700),		"#w&s", 	"*"},	/* movem reg to mem. */
{"movemw",	one(0044240),		one(0177770),		"#w-s", 	"*"},	/* movem reg to autodecrement. */
{"movemw",	one(0046200),		one(0177700),		"!s#w", 	"*"},	/* movem mem to reg. */
{"movemw",	one(0046230),		one(0177770),		"+s#w", 	"*"},	/* movem autoinc to reg. */
/* JF added these next four for the assembler */
{"movemw",	one(0044200),		one(0177700),		"Lw&s", 	"*"},	/* movem reg to mem. */
{"movemw",	one(0044240),		one(0177770),		"lw-s", 	"*"},	/* movem reg to autodecrement. */
{"movemw",	one(0046200),		one(0177700),		"!sLw", 	"*"},	/* movem mem to reg. */
{"movemw",	one(0046230),		one(0177770),		"+sLw", 	"*"},	/* movem autoinc to reg. */

{"movepl",	one(0000510),		one(0170770),		"dsDd", 	"*"},	/* memory to register */
{"movepl",	one(0000710),		one(0170770),		"Ddds", 	"*"},	/* register to memory */
{"movepw",	one(0000410),		one(0170770),		"dsDd", 	"*"},	/* memory to register */
{"movepw",	one(0000610),		one(0170770),		"Ddds", 	"*"},	/* register to memory */
{"moveq",	one(0070000),		one(0170400),		"MsDd", 	"*"},
{"movew",	one(0030000),		one(0170000),		"*w$d", 	"*"},
{"movew",	one(0030100),		one(0170700),		"*wAd", 	"*"},	/* movea, written as move */
{"movew",	one(0040300),		one(0177700),		"Ss$s", 	"*p"},	/* Move from sr */
{"movew",	one(0041300),		one(0177700),		"Cs$s", 	"1234"},/* Move from ccr */
{"movew",	one(0042300),		one(0177700),		";wCd", 	"*"},	/* move to ccr */
{"movew",	one(0043300),		one(0177700),		";wSd", 	"*p"},	/* move to sr */

{"movesb",	two(0007000, 0),	two(0177700, 07777),	"~sR1", 	"234p"}, /* moves from memory */
{"movesb",	two(0007000, 04000),	two(0177700, 07777),	"R1~s", 	"234p"}, /* moves to memory */
{"movesl",	two(0007200, 0),	two(0177700, 07777),	"~sR1", 	"234p"}, /* moves from memory */
{"movesl",	two(0007200, 04000),	two(0177700, 07777),	"R1~s", 	"234p"}, /* moves to memory */
{"movesw",	two(0007100, 0),	two(0177700, 07777),	"~sR1", 	"234p"}, /* moves from memory */
{"movesw",	two(0007100, 04000),	two(0177700, 07777),	"R1~s", 	"234p"}, /* moves to memory */

{"move16",	one(0173000),		one(0177770),		"+s#l",		"4"},	/* (An)+,xxx.L */
{"move16",	one(0173010),		one(0177770),		"#l+s",		"4"},	/* xxx.L,(An)+ */
{"move16",	one(0173020),		one(0177770),		"0s#l",		"4"},	/* (An), xxx.L */
{"move16",	one(0173030),		one(0177770),		"#l0s" 		"4"},	/* xxx.L,(An) */
{"move16",	two(0173040, 0100000),	two(0177770, 0107777),	"+s+1", 	"4"},	/* (Ax)+,(Ay)+ */

{"mulsl",	two(0046000, 004000),	two(0177700, 0107770),	";lD1", 	"*"},
{"mulsl",	two(0046000, 006000),	two(0177700, 0107770),	";lD3D1", 	"*"},
{"mulsw",	one(0140700),		one(0170700),		";wDd", 	"*"},
{"muls",	one(0140700),		one(0170700),		";wDd", 	"*"},
{"mulul",	two(0046000, 000000),	two(0177700, 0107770),	";lD1", 	"*"},
{"mulul",	two(0046000, 002000),	two(0177700, 0107770),	";lD3D1", 	"*"},
{"muluw",	one(0140300),		one(0170700),		";wDd", 	"*"},
{"mulu",	one(0140300),		one(0170700),		";wDd", 	"*"},
{"nbcd",	one(0044000),		one(0177700),		"$s", 		"*"},
{"negb",	one(0042000),		one(0177700),		"$s", 		"*"},
{"negl",	one(0042200),		one(0177700),		"$s", 		"*"},
{"negw",	one(0042100),		one(0177700),		"$s", 		"*"},
{"negxb",	one(0040000),		one(0177700),		"$s", 		"*"},
{"negxl",	one(0040200),		one(0177700),		"$s", 		"*"},
{"negxw",	one(0040100),		one(0177700),		"$s", 		"*"},
{"nop",		one(0047161),		one(0177777),		"", 		"*"},
{"notb",	one(0043000),		one(0177700),		"$s", 		"*"},
{"notl",	one(0043200),		one(0177700),		"$s", 		"*"},
{"notw",	one(0043100),		one(0177700),		"$s", 		"*"},

{"orb",		one(0000000),		one(0177700),		"#b$s", 	"*"},	/* ori written as or */
{"orb",		one(0000074),		one(0177777),		"#bCs", 	"*"},	/* ori to ccr */
{"orb",		one(0100000),		one(0170700),		";bDd", 	"*"},	/* memory to register */
{"orb",		one(0100400),		one(0170700),		"Dd~s", 	"*"},	/* register to memory */
{"orib",	one(0000000),		one(0177700),		"#b$s", 	"*"},
{"orib",	one(0000074),		one(0177777),		"#bCs", 	"*"},	/* ori to ccr */
{"oril",	one(0000200),		one(0177700),		"#l$s", 	"*"},
{"oriw",	one(0000100),		one(0177700),		"#w$s", 	"*"},
{"oriw",	one(0000174),		one(0177777),		"#wSs", 	"*p"},	/* ori to sr */
{"orl",		one(0000200),		one(0177700),		"#l$s", 	"*"},
{"orl",		one(0100200),		one(0170700),		";lDd", 	"*"},	/* memory to register */
{"orl",		one(0100600),		one(0170700),		"Dd~s", 	"*"},	/* register to memory */
{"orw",		one(0000100),		one(0177700),		"#w$s", 	"*"},
{"orw",		one(0000174),		one(0177777),		"#wSs", 	"*"},	/* ori to sr */
{"orw",		one(0100100),		one(0170700),		";wDd", 	"*"},	/* memory to register */
{"orw",		one(0100500),		one(0170700),		"Dd~s", 	"*"},	/* register to memory */

{"pack",	one(0100500),		one(0170770),		"DsDd#w", 	"234"},	/* pack Ds,	Dd,	#w */
{"pack",	one(0100510),		one(0170770),		"-s-d#w", 	"234"},	/* pack -(As),	-(Ad),	#w */
{"pea",		one(0044100),		one(0177700),		"!s", 		"*"},
{"reset",	one(0047160),		one(0177777),		"", 		"*p"},

{"rolb",	one(0160430),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"rolb",	one(0160470),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"roll",	one(0160630),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"roll",	one(0160670),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"rolw",	one(0160530),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"rolw",	one(0160570),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"rolw",	one(0163700),		one(0177700),		"~s", 		"*"},	/* Rotate memory */
{"rorb",	one(0160030),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"rorb",	one(0160070),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"rorl",	one(0160230),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"rorl",	one(0160270),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"rorw",	one(0160130),		one(0170770),		"QdDs", 	"*"},	/* rorb #Q,	Ds */
{"rorw",	one(0160170),		one(0170770),		"DdDs", 	"*"},	/* rorb Dd,	Ds */
{"rorw",	one(0163300),		one(0177700),		"~s", 		"*"},	/* Rotate memory */

{"roxlb",	one(0160420),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxlb",	one(0160460),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxll",	one(0160620),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxll",	one(0160660),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxlw",	one(0160520),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxlw",	one(0160560),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxlw",	one(0162700),		one(0177700),		"~s", 		"*"},	/* Rotate memory */
{"roxrb",	one(0160020),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxrb",	one(0160060),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxrl",	one(0160220),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxrl",	one(0160260),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxrw",	one(0160120),		one(0170770),		"QdDs", 	"*"},	/* roxrb #Q,	Ds */
{"roxrw",	one(0160160),		one(0170770),		"DdDs", 	"*"},	/* roxrb Dd,	Ds */
{"roxrw",	one(0162300),		one(0177700),		"~s", 		"*"},	/* Rotate memory */

{"rtd",		one(0047164),		one(0177777),		"#w", 		"1234"},
{"rte",		one(0047163),		one(0177777),		"", 		"*p"},
{"rtm",		one(0003300),		one(0177760),		"Rs", 		"2"},
{"rtr",		one(0047167),		one(0177777),		"", 		"*"},
{"rts",		one(0047165),		one(0177777),		"", 		"*"},

{"scc",		one(0052300),		one(0177700),		"$s", 		"*"},
{"scs",		one(0052700),		one(0177700),		"$s", 		"*"},
{"seq",		one(0053700),		one(0177700),		"$s", 		"*"},
{"sf",		one(0050700),		one(0177700),		"$s", 		"*"},
{"sge",		one(0056300),		one(0177700),		"$s", 		"*"},
{"sgt",		one(0057300),		one(0177700),		"$s", 		"*"},
{"shi",		one(0051300),		one(0177700),		"$s", 		"*"},
{"sle",		one(0057700),		one(0177700),		"$s", 		"*"},
{"sls",		one(0051700),		one(0177700),		"$s", 		"*"},
{"slt",		one(0056700),		one(0177700),		"$s", 		"*"},
{"smi",		one(0055700),		one(0177700),		"$s", 		"*"},
{"sne",		one(0053300),		one(0177700),		"$s", 		"*"},
{"spl",		one(0055300),		one(0177700),		"$s", 		"*"},
{"st",		one(0050300),		one(0177700),		"$s", 		"*"},
{"svc",		one(0054300),		one(0177700),		"$s", 		"*"},
{"svs",		one(0054700),		one(0177700),		"$s", 		"*"},

{"sbcd",	one(0100400),		one(0170770),		"DsDd", 	"*"},
{"sbcd",	one(0100410),		one(0170770),		"-s-d", 	"*"},
{"stop",	one(0047162),		one(0177777),		"#w", 		"*p"},

{"subal",	one(0110700),		one(0170700),		"*lAd", 	"*"},
{"subaw",	one(0110300),		one(0170700),		"*wAd", 	"*"},
{"subb",	one(0050400),		one(0170700),		"Qd%s", 	"*"},	/* subq written as sub */
{"subb",	one(0002000),		one(0177700),		"#b$s", 	"*"},	/* subi written as sub */
{"subb",	one(0110000),		one(0170700),		";bDd", 	"*"},	/* subb ??,	Dd */
{"subb",	one(0110400),		one(0170700),		"Dd~s", 	"*"},	/* subb Dd,	?? */
{"subib",	one(0002000),		one(0177700),		"#b$s", 	"*"},
{"subil",	one(0002200),		one(0177700),		"#l$s", 	"*"},
{"subiw",	one(0002100),		one(0177700),		"#w$s", 	"*"},

{"subl",	one(0050600),		one(0170700),		"Qd%s", 	"*"},
{"subl",	one(0002200),		one(0177700),		"#l$s", 	"*"},
{"subl",	one(0110700),		one(0170700),		"*lAd", 	"*"},
{"subl",	one(0110200),		one(0170700),		"*lDd", 	"*"},
{"subl",	one(0110600),		one(0170700),		"Dd~s", 	"*"},

{"subqb",	one(0050400),		one(0170700),		"Qd%s", 	"*"},
{"subql",	one(0050600),		one(0170700),		"Qd%s", 	"*"},
{"subqw",	one(0050500),		one(0170700),		"Qd%s", 	"*"},
{"subw",	one(0050500),		one(0170700),		"Qd%s", 	"*"},
{"subw",	one(0002100),		one(0177700),		"#w$s", 	"*"},
{"subw",	one(0110100),		one(0170700),		"*wDd", 	"*"},
{"subw",	one(0110300),		one(0170700),		"*wAd", 	"*"},	/* suba written as sub */
{"subw",	one(0110500),		one(0170700),		"Dd~s", 	"*"},

{"subxb",	one(0110400),		one(0170770),		"DsDd", 	"*"},	/* subxb Ds,	Dd */
{"subxb",	one(0110410),		one(0170770),		"-s-d", 	"*"},	/* subxb -(As),	-(Ad) */
{"subxl",	one(0110600),		one(0170770),		"DsDd", 	"*"},
{"subxl",	one(0110610),		one(0170770),		"-s-d", 	"*"},
{"subxw",	one(0110500),		one(0170770),		"DsDd", 	"*"},
{"subxw",	one(0110510),		one(0170770),		"-s-d", 	"*"},

{"swap",	one(0044100),		one(0177770),		"Ds", 		"*"},
	
{"tas",		one(0045300),		one(0177700),		"$s", 		"*"},
{"trap",	one(0047100),		one(0177760),		"Ts", 		"*"},

{"trapcc",	one(0052374),		one(0177777),		"", 		"234"},
{"trapcs",	one(0052774),		one(0177777),		"", 		"234"},
{"trapeq",	one(0053774),		one(0177777),		"", 		"234"},
{"trapf",	one(0050774),		one(0177777),		"", 		"234"},
{"trapge",	one(0056374),		one(0177777),		"", 		"234"},
{"trapgt",	one(0057374),		one(0177777),		"", 		"234"},
{"traphi",	one(0051374),		one(0177777),		"", 		"234"},
{"traple",	one(0057774),		one(0177777),		"", 		"234"},
{"trapls",	one(0051774),		one(0177777),		"", 		"234"},
{"traplt",	one(0056774),		one(0177777),		"", 		"234"},
{"trapmi",	one(0055774),		one(0177777),		"", 		"234"},
{"trapne",	one(0053374),		one(0177777),		"", 		"234"},
{"trappl",	one(0055374),		one(0177777),		"", 		"234"},
{"trapt",	one(0050374),		one(0177777),		"", 		"234"},
{"trapvc",	one(0054374),		one(0177777),		"", 		"234"},
{"trapvs",	one(0054774),		one(0177777),		"", 		"234"},

{"trapcc.w",	one(0052372),		one(0177777),		"", 		"234"},
{"trapcs.w",	one(0052772),		one(0177777),		"", 		"234"},
{"trapeq.w",	one(0053772),		one(0177777),		"", 		"234"},
{"trapf.w",	one(0050772),		one(0177777),		"", 		"234"},
{"trapge.w",	one(0056372),		one(0177777),		"", 		"234"},
{"trapgt.w",	one(0057372),		one(0177777),		"", 		"234"},
{"traphi.w",	one(0051372),		one(0177777),		"", 		"234"},
{"traple.w",	one(0057772),		one(0177777),		"", 		"234"},
{"trapls.w",	one(0051772),		one(0177777),		"", 		"234"},
{"traplt.w",	one(0056772),		one(0177777),		"", 		"234"},
{"trapmi.w",	one(0055772),		one(0177777),		"", 		"234"},
{"trapne.w",	one(0053372),		one(0177777),		"", 		"234"},
{"trappl.w",	one(0055372),		one(0177777),		"", 		"234"},
{"trapt.w",	one(0050372),		one(0177777),		"", 		"234"},
{"trapvc.w",	one(0054372),		one(0177777),		"", 		"234"},
{"trapvs.w",	one(0054772),		one(0177777),		"", 		"234"},

{"trapcc.l",	one(0052373),		one(0177777),		"", 		"234"},
{"trapcs.l",	one(0052773),		one(0177777),		"", 		"234"},
{"trapeq.l",	one(0053773),		one(0177777),		"", 		"234"},
{"trapf.l",	one(0050773),		one(0177777),		"", 		"234"},
{"trapge.l",	one(0056373),		one(0177777),		"", 		"234"},
{"trapgt.l",	one(0057373),		one(0177777),		"", 		"234"},
{"traphi.l",	one(0051373),		one(0177777),		"", 		"234"},
{"traple.l",	one(0057773),		one(0177777),		"", 		"234"},
{"trapls.l",	one(0051773),		one(0177777),		"", 		"234"},
{"traplt.l",	one(0056773),		one(0177777),		"", 		"234"},
{"trapmi.l",	one(0055773),		one(0177777),		"", 		"234"},
{"trapne.l",	one(0053373),		one(0177777),		"", 		"234"},
{"trappl.l",	one(0055373),		one(0177777),		"", 		"234"},
{"trapt.l",	one(0050373),		one(0177777),		"", 		"234"},
{"trapvc.l",	one(0054373),		one(0177777),		"", 		"234"},
{"trapvs.l",	one(0054773),		one(0177777),		"", 		"234"},

{"trapv",	one(0047166),		one(0177777),		"", 		"*"},

{"tstb",	one(0045000),		one(0177700),		";b", 		"*"},
{"tstw",	one(0045100),		one(0177700),		"*w", 		"*"},
{"tstl",	one(0045200),		one(0177700),		"*l", 		"*"},

{"unlk",	one(0047130),		one(0177770),		"As", 		"*"},
{"unpk",	one(0100600),		one(0170770),		"DsDd#w", 	"234"},
{"unpk",	one(0100610),		one(0170770),		"-s-d#w", 	"234"},

	/* JF floating pt stuff moved down here */

{"fabsb",	two(0xF000, 0x5818),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fabsd",	two(0xF000, 0x5418),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fabsl",	two(0xF000, 0x4018),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fabsp",	two(0xF000, 0x4C18),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fabss",	two(0xF000, 0x4418),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fabsw",	two(0xF000, 0x5018),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fabsx",	two(0xF000, 0x0018),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fabsx",	two(0xF000, 0x4818),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fabsx",	two(0xF000, 0x0018),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fsabsb",	two(0xF000, 0x5858),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsabsd",	two(0xF000, 0x5458),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsabsl",	two(0xF000, 0x4058),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsabsp",	two(0xF000, 0x4C58),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsabss",	two(0xF000, 0x4458),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsabsw",	two(0xF000, 0x5058),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsabsx",	two(0xF000, 0x0058),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsabsx",	two(0xF000, 0x4858),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fsabsx",	two(0xF000, 0x0058),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fdabsb",	two(0xF000, 0x585C),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdabsd",	two(0xF000, 0x545C),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdabsl",	two(0xF000, 0x405C),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdabsp",	two(0xF000, 0x4C5C),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdabss",	two(0xF000, 0x445C),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdabsw",	two(0xF000, 0x505C),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdabsx",	two(0xF000, 0x005C),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdabsx",	two(0xF000, 0x485C),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fdabsx",	two(0xF000, 0x005C),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"facosb",	two(0xF000, 0x581C),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"facosd",	two(0xF000, 0x541C),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"facosl",	two(0xF000, 0x401C),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"facosp",	two(0xF000, 0x4C1C),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"facoss",	two(0xF000, 0x441C),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"facosw",	two(0xF000, 0x501C),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"facosx",	two(0xF000, 0x001C),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"facosx",	two(0xF000, 0x481C),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"facosx",	two(0xF000, 0x001C),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"faddb",	two(0xF000, 0x5822),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"faddd",	two(0xF000, 0x5422),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"faddl",	two(0xF000, 0x4022),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"faddp",	two(0xF000, 0x4C22),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fadds",	two(0xF000, 0x4422),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"faddw",	two(0xF000, 0x5022),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"faddx",	two(0xF000, 0x0022),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"faddx",	two(0xF000, 0x4822),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
/* {"faddx",	two(0xF000, 0x0022),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"}, JF removed */

{"fsaddb",	two(0xF000, 0x5862),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsaddd",	two(0xF000, 0x5462),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsaddl",	two(0xF000, 0x4062),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsaddp",	two(0xF000, 0x4C62),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsadds",	two(0xF000, 0x4462),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsaddw",	two(0xF000, 0x5062),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsaddx",	two(0xF000, 0x0062),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsaddx",	two(0xF000, 0x4862),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fdaddb",	two(0xF000, 0x5866),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdaddd",	two(0xF000, 0x5466),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdaddl",	two(0xF000, 0x4066),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdaddp",	two(0xF000, 0x4C66),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdadds",	two(0xF000, 0x4466),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdaddw",	two(0xF000, 0x5066),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdaddx",	two(0xF000, 0x0066),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdaddx",	two(0xF000, 0x4866),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fasinb",	two(0xF000, 0x580C),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fasind",	two(0xF000, 0x540C),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fasinl",	two(0xF000, 0x400C),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48ee"},
{"fasinp",	two(0xF000, 0x4C0C),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fasins",	two(0xF000, 0x440C),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fasinw",	two(0xF000, 0x500C),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fasinx",	two(0xF000, 0x000C),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fasinx",	two(0xF000, 0x480C),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fasinx",	two(0xF000, 0x000C),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fatanb",	two(0xF000, 0x580A),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fatand",	two(0xF000, 0x540A),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fatanl",	two(0xF000, 0x400A),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fatanp",	two(0xF000, 0x4C0A),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fatans",	two(0xF000, 0x440A),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fatanw",	two(0xF000, 0x500A),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fatanx",	two(0xF000, 0x000A),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fatanx",	two(0xF000, 0x480A),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fatanx",	two(0xF000, 0x000A),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fatanhb",	two(0xF000, 0x580D),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fatanhd",	two(0xF000, 0x540D),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fatanhl",	two(0xF000, 0x400D),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fatanhp",	two(0xF000, 0x4C0D),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fatanhs",	two(0xF000, 0x440D),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fatanhw",	two(0xF000, 0x500D),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fatanhx",	two(0xF000, 0x000D),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fatanhx",	two(0xF000, 0x480D),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fatanhx",	two(0xF000, 0x000D),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fbeq",	one(0xF081),		one(0xF1BF),		"IdBc", 	"48"},
{"fbf",		one(0xF080),		one(0xF1BF),		"IdBc", 	"48"},
{"fbge",	one(0xF093),		one(0xF1BF),		"IdBc", 	"48"},
{"fbgl",	one(0xF096),		one(0xF1BF),		"IdBc", 	"48"},
{"fbgle",	one(0xF097),		one(0xF1BF),		"IdBc", 	"48"},
{"fbgt",	one(0xF092),		one(0xF1BF),		"IdBc", 	"48"},
{"fble",	one(0xF095),		one(0xF1BF),		"IdBc", 	"48"},
{"fblt",	one(0xF094),		one(0xF1BF),		"IdBc", 	"48"},
{"fbne",	one(0xF08E),		one(0xF1BF),		"IdBc", 	"48"},
{"fbnge",	one(0xF09C),		one(0xF1BF),		"IdBc", 	"48"},
{"fbngl",	one(0xF099),		one(0xF1BF),		"IdBc", 	"48"},
{"fbngle",	one(0xF098),		one(0xF1BF),		"IdBc", 	"48"},
{"fbngt",	one(0xF09D),		one(0xF1BF),		"IdBc", 	"48"},
{"fbnle",	one(0xF09A),		one(0xF1BF),		"IdBc", 	"48"},
{"fbnlt",	one(0xF09B),		one(0xF1BF),		"IdBc", 	"48"},
{"fboge",	one(0xF083),		one(0xF1BF),		"IdBc", 	"48"},
{"fbogl",	one(0xF086),		one(0xF1BF),		"IdBc", 	"48"},
{"fbogt",	one(0xF082),		one(0xF1BF),		"IdBc", 	"48"},
{"fbole",	one(0xF085),		one(0xF1BF),		"IdBc", 	"48"},
{"fbolt",	one(0xF084),		one(0xF1BF),		"IdBc", 	"48"},
{"fbor",	one(0xF087),		one(0xF1BF),		"IdBc", 	"48"},
{"fbseq",	one(0xF091),		one(0xF1BF),		"IdBc", 	"48"},
{"fbsf",	one(0xF090),		one(0xF1BF),		"IdBc", 	"48"},
{"fbsne",	one(0xF09E),		one(0xF1BF),		"IdBc", 	"48"},
{"fbst",	one(0xF09F),		one(0xF1BF),		"IdBc", 	"48"},
{"fbt",		one(0xF08F),		one(0xF1BF),		"IdBc", 	"48"},
{"fbueq",	one(0xF089),		one(0xF1BF),		"IdBc", 	"48"},
{"fbuge",	one(0xF08B),		one(0xF1BF),		"IdBc", 	"48"},
{"fbugt",	one(0xF08A),		one(0xF1BF),		"IdBc", 	"48"},
{"fbule",	one(0xF08D),		one(0xF1BF),		"IdBc", 	"48"},
{"fbult",	one(0xF08C),		one(0xF1BF),		"IdBc", 	"48"},
{"fbun",	one(0xF088),		one(0xF1BF),		"IdBc", 	"48"},

{"fcmpb",	two(0xF000, 0x5838),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fcmpd",	two(0xF000, 0x5438),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fcmpl",	two(0xF000, 0x4038),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fcmpp",	two(0xF000, 0x4C38),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fcmps",	two(0xF000, 0x4438),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fcmpw",	two(0xF000, 0x5038),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fcmpx",	two(0xF000, 0x0038),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fcmpx",	two(0xF000, 0x4838),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
/* {"fcmpx",	two(0xF000, 0x0038),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"}, JF removed */

{"fcosb",	two(0xF000, 0x581D),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fcosd",	two(0xF000, 0x541D),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fcosl",	two(0xF000, 0x401D),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fcosp",	two(0xF000, 0x4C1D),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fcoss",	two(0xF000, 0x441D),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fcosw",	two(0xF000, 0x501D),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fcosx",	two(0xF000, 0x001D),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fcosx",	two(0xF000, 0x481D),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fcosx",	two(0xF000, 0x001D),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fcoshb",	two(0xF000, 0x5819),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fcoshd",	two(0xF000, 0x5419),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fcoshl",	two(0xF000, 0x4019),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fcoshp",	two(0xF000, 0x4C19),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fcoshs",	two(0xF000, 0x4419),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fcoshw",	two(0xF000, 0x5019),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fcoshx",	two(0xF000, 0x0019),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fcoshx",	two(0xF000, 0x4819),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fcoshx",	two(0xF000, 0x0019),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fdbeq",	two(0xF048, 0x0001),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbf",	two(0xF048, 0x0000),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbge",	two(0xF048, 0x0013),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbgl",	two(0xF048, 0x0016),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbgle",	two(0xF048, 0x0017),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbgt",	two(0xF048, 0x0012),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdble",	two(0xF048, 0x0015),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdblt",	two(0xF048, 0x0014),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbne",	two(0xF048, 0x000E),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbnge",	two(0xF048, 0x001C),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbngl",	two(0xF048, 0x0019),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbngle",	two(0xF048, 0x0018),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbngt",	two(0xF048, 0x001D),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbnle",	two(0xF048, 0x001A),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbnlt",	two(0xF048, 0x001B),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdboge",	two(0xF048, 0x0003),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbogl",	two(0xF048, 0x0006),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbogt",	two(0xF048, 0x0002),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbole",	two(0xF048, 0x0005),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbolt",	two(0xF048, 0x0004),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbor",	two(0xF048, 0x0007),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbseq",	two(0xF048, 0x0011),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbsf",	two(0xF048, 0x0010),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbsne",	two(0xF048, 0x001E),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbst",	two(0xF048, 0x001F),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbt",	two(0xF048, 0x000F),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbueq",	two(0xF048, 0x0009),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbuge",	two(0xF048, 0x000B),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbugt",	two(0xF048, 0x000A),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbule",	two(0xF048, 0x000D),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbult",	two(0xF048, 0x000C),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},
{"fdbun",	two(0xF048, 0x0008),	two(0xF1F8, 0xFFFF),	"IiDsBw", 	"48"},

{"fdivb",	two(0xF000, 0x5820),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fdivd",	two(0xF000, 0x5420),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fdivl",	two(0xF000, 0x4020),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fdivp",	two(0xF000, 0x4C20),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fdivs",	two(0xF000, 0x4420),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fdivw",	two(0xF000, 0x5020),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fdivx",	two(0xF000, 0x0020),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fdivx",	two(0xF000, 0x4820),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
/* {"fdivx",	two(0xF000, 0x0020),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"}, JF */

{"fsdivb",	two(0xF000, 0x5860),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsdivd",	two(0xF000, 0x5460),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsdivl",	two(0xF000, 0x4060),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsdivp",	two(0xF000, 0x4C60),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsdivs",	two(0xF000, 0x4460),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsdivw",	two(0xF000, 0x5060),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsdivx",	two(0xF000, 0x0060),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsdivx",	two(0xF000, 0x4860),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fddivb",	two(0xF000, 0x5864),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fddivd",	two(0xF000, 0x5464),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fddivl",	two(0xF000, 0x4064),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fddivp",	two(0xF000, 0x4C64),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fddivs",	two(0xF000, 0x4464),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fddivw",	two(0xF000, 0x5064),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fddivx",	two(0xF000, 0x0064),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fddivx",	two(0xF000, 0x4864),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fetoxb",	two(0xF000, 0x5810),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fetoxd",	two(0xF000, 0x5410),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fetoxl",	two(0xF000, 0x4010),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fetoxp",	two(0xF000, 0x4C10),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fetoxs",	two(0xF000, 0x4410),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fetoxw",	two(0xF000, 0x5010),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fetoxx",	two(0xF000, 0x0010),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fetoxx",	two(0xF000, 0x4810),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fetoxx",	two(0xF000, 0x0010),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fetoxm1b",	two(0xF000, 0x5808),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fetoxm1d",	two(0xF000, 0x5408),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fetoxm1l",	two(0xF000, 0x4008),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fetoxm1p",	two(0xF000, 0x4C08),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fetoxm1s",	two(0xF000, 0x4408),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fetoxm1w",	two(0xF000, 0x5008),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fetoxm1x",	two(0xF000, 0x0008),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fetoxm1x",	two(0xF000, 0x4808),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fetoxm1x",	two(0xF000, 0x0008),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fgetexpb",	two(0xF000, 0x581E),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fgetexpd",	two(0xF000, 0x541E),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fgetexpl",	two(0xF000, 0x401E),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fgetexpp",	two(0xF000, 0x4C1E),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fgetexps",	two(0xF000, 0x441E),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fgetexpw",	two(0xF000, 0x501E),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fgetexpx",	two(0xF000, 0x001E),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fgetexpx",	two(0xF000, 0x481E),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fgetexpx",	two(0xF000, 0x001E),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fgetmanb",	two(0xF000, 0x581F),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fgetmand",	two(0xF000, 0x541F),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fgetmanl",	two(0xF000, 0x401F),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fgetmanp",	two(0xF000, 0x4C1F),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fgetmans",	two(0xF000, 0x441F),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fgetmanw",	two(0xF000, 0x501F),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fgetmanx",	two(0xF000, 0x001F),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fgetmanx",	two(0xF000, 0x481F),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fgetmanx",	two(0xF000, 0x001F),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fintb",	two(0xF000, 0x5801),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fintd",	two(0xF000, 0x5401),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fintl",	two(0xF000, 0x4001),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fintp",	two(0xF000, 0x4C01),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fints",	two(0xF000, 0x4401),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fintw",	two(0xF000, 0x5001),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fintx",	two(0xF000, 0x0001),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fintx",	two(0xF000, 0x4801),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fintx",	two(0xF000, 0x0001),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fintrzb",	two(0xF000, 0x5803),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fintrzd",	two(0xF000, 0x5403),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fintrzl",	two(0xF000, 0x4003),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fintrzp",	two(0xF000, 0x4C03),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fintrzs",	two(0xF000, 0x4403),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fintrzw",	two(0xF000, 0x5003),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fintrzx",	two(0xF000, 0x0003),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fintrzx",	two(0xF000, 0x4803),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fintrzx",	two(0xF000, 0x0003),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"flog10b",	two(0xF000, 0x5815),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"flog10d",	two(0xF000, 0x5415),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"flog10l",	two(0xF000, 0x4015),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"flog10p",	two(0xF000, 0x4C15),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"flog10s",	two(0xF000, 0x4415),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"flog10w",	two(0xF000, 0x5015),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"flog10x",	two(0xF000, 0x0015),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"flog10x",	two(0xF000, 0x4815),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"flog10x",	two(0xF000, 0x0015),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"flog2b",	two(0xF000, 0x5816),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"flog2d",	two(0xF000, 0x5416),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"flog2l",	two(0xF000, 0x4016),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"flog2p",	two(0xF000, 0x4C16),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"flog2s",	two(0xF000, 0x4416),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"flog2w",	two(0xF000, 0x5016),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"flog2x",	two(0xF000, 0x0016),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"flog2x",	two(0xF000, 0x4816),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"flog2x",	two(0xF000, 0x0016),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"flognb",	two(0xF000, 0x5814),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"flognd",	two(0xF000, 0x5414),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"flognl",	two(0xF000, 0x4014),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"flognp",	two(0xF000, 0x4C14),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"flogns",	two(0xF000, 0x4414),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"flognw",	two(0xF000, 0x5014),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"flognx",	two(0xF000, 0x0014),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"flognx",	two(0xF000, 0x4814),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"flognx",	two(0xF000, 0x0014),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"flognp1b",	two(0xF000, 0x5806),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"flognp1d",	two(0xF000, 0x5406),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"flognp1l",	two(0xF000, 0x4006),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"flognp1p",	two(0xF000, 0x4C06),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"flognp1s",	two(0xF000, 0x4406),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"flognp1w",	two(0xF000, 0x5006),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"flognp1x",	two(0xF000, 0x0006),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"flognp1x",	two(0xF000, 0x4806),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"flognp1x",	two(0xF000, 0x0006),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fmodb",	two(0xF000, 0x5821),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fmodd",	two(0xF000, 0x5421),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fmodl",	two(0xF000, 0x4021),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fmodp",	two(0xF000, 0x4C21),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fmods",	two(0xF000, 0x4421),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fmodw",	two(0xF000, 0x5021),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fmodx",	two(0xF000, 0x0021),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fmodx",	two(0xF000, 0x4821),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
/* {"fmodx",	two(0xF000, 0x0021),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"}, JF */

{"fmoveb",	two(0xF000, 0x5800),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmoveb",	two(0xF000, 0x7800),	two(0xF1C0, 0xFC7F),	"IiF7@b", 	"48"},	/* fmove from fp<n> to <ea> */
{"fmoved",	two(0xF000, 0x5400),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmoved",	two(0xF000, 0x7400),	two(0xF1C0, 0xFC7F),	"IiF7@F", 	"48"},	/* fmove from fp<n> to <ea> */
{"fmovel",	two(0xF000, 0x4000),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmovel",	two(0xF000, 0x6000),	two(0xF1C0, 0xFC7F),	"IiF7@l", 	"48"},	/* fmove from fp<n> to <ea> */
/* Warning:  The addressing modes on these are probably not right:
   esp, Areg direct is only allowed for FPI */
		/* fmove.l from/to system control registers: */
{"fmovel",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"Iis8@s", 	"48"},
{"fmovel",	two(0xF000, 0x8000),	two(0xF1C0, 0xE3FF),	"Ii*ls8", 	"48"},

/* {"fmovel",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"Iis8@s", 	"48"},
{"fmovel",	two(0xF000, 0x8000),	two(0xF2C0, 0xE3FF),	"Ii*ss8", 	"48"}, */

{"fmovep",	two(0xF000, 0x4C00),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmovep",	two(0xF000, 0x6C00),	two(0xF1C0, 0xFC00),	"IiF7@pkC", 	"48"},	/* fmove.p with k-factors: */
{"fmovep",	two(0xF000, 0x7C00),	two(0xF1C0, 0xFC0F),	"IiF7@pDk", 	"48"},	/* fmove.p with k-factors: */

{"fmoves",	two(0xF000, 0x4400),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmoves",	two(0xF000, 0x6400),	two(0xF1C0, 0xFC7F),	"IiF7@f", 	"48"},	/* fmove from fp<n> to <ea> */
{"fmovew",	two(0xF000, 0x5000),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmovew",	two(0xF000, 0x7000),	two(0xF1C0, 0xFC7F),	"IiF7@w", 	"48"},	/* fmove from fp<n> to <ea> */
{"fmovex",	two(0xF000, 0x0000),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmovex",	two(0xF000, 0x4800),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},	/* fmove from <ea> to fp<n> */
{"fmovex",	two(0xF000, 0x6800),	two(0xF1C0, 0xFC7F),	"IiF7@x", 	"48"},	/* fmove from fp<n> to <ea> */
/* JF removed {"fmovex",	two(0xF000, 0x0000),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},	/* fmove from <ea> to fp<n> */

{"fsmoveb",	two(0xF000, 0x5840),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsmoved",	two(0xF000, 0x5440),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsmovel",	two(0xF000, 0x4040),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsmovep",	two(0xF000, 0x4C40),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsmoves",	two(0xF000, 0x4440),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsmovew",	two(0xF000, 0x5040),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsmovex",	two(0xF000, 0x0040),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsmovex",	two(0xF000, 0x4840),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fdmoveb",	two(0xF000, 0x5844),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdmoved",	two(0xF000, 0x5444),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdmovel",	two(0xF000, 0x4044),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdmovep",	two(0xF000, 0x4C44),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdmoves",	two(0xF000, 0x4444),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdmovew",	two(0xF000, 0x5044),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdmovex",	two(0xF000, 0x0044),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdmovex",	two(0xF000, 0x4844),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fmovecrx",	two(0xF000, 0x5C00),	two(0xF1FF, 0xFC00),	"Ii#CF7", 	"48e"},	/* fmovecr.x #ccc,	FPn */
{"fmovecr",	two(0xF000, 0x5C00),	two(0xF1FF, 0xFC00),	"Ii#CF7", 	"48e"},

{"fmovemx",	two(0xF020, 0xE000),	two(0xF1F8, 0xFF00),	"Id#3-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */
{"fmovemx",	two(0xF020, 0xE800),	two(0xF1F8, 0xFF8F),	"IiDk-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */

{"fmovemx",	two(0xF000, 0xF000),	two(0xF1C0, 0xFF00),	"Id#3&s", 	"48"},	/* fmovem.x to control,	static and dynamic: */
{"fmovemx",	two(0xF000, 0xF800),	two(0xF1C0, 0xFF8F),	"IiDk&s", 	"48"},	/* fmovem.x to control,	static and dynamic: */

{"fmovemx",	two(0xF018, 0xD000),	two(0xF1F8, 0xFF00),	"Id+s#3", 	"48"},	/* fmovem.x from autoincrement, static and dynamic: */
{"fmovemx",	two(0xF018, 0xD800),	two(0xF1F8, 0xFF8F),	"Ii+sDk", 	"48"},	/* fmovem.x from autoincrement, static and dynamic: */
  
{"fmovemx",	two(0xF000, 0xD000),	two(0xF1C0, 0xFF00),	"Id&s#3", 	"48"},	/* fmovem.x from control, static and dynamic: */
{"fmovemx",	two(0xF000, 0xD800),	two(0xF1C0, 0xFF8F),	"Ii&sDk", 	"48"},	/* fmovem.x from control, static and dynamic: */

{"fmovemx",	two(0xF020, 0xE000),	two(0xF1F8, 0xFF00),	"IdL3-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */
{"fmovemx",	two(0xF000, 0xF000),	two(0xF1C0, 0xFF00),	"Idl3&s", 	"48"},	/* fmovem.x to control,	static and dynamic: */
{"fmovemx",	two(0xF018, 0xD000),	two(0xF1F8, 0xFF00),	"Id+sl3", 	"48"},	/* fmovem.x from autoincrement,	static and dynamic: */
{"fmovemx",	two(0xF000, 0xD000),	two(0xF1C0, 0xFF00),	"Id&sl3", 	"48"},	/* fmovem.x from control, static and dynamic: */

{"fmoveml",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"Ii#8@s", 	"48"},
{"fmoveml",	two(0xF000, 0x8000),	two(0xF1C0, 0xE3FF),	"Ii*s#8", 	"48"},

{"fmoveml",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"Iis8@s", 	"48"},
{"fmoveml",	two(0xF000, 0x8000),	two(0xF1C0, 0xE3FF),	"Ii*ss8", 	"48"},

{"fmoveml",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"IiL8@s", 	"48"},
{"fmoveml",	two(0xF000, 0x8000),	two(0xF2C0, 0xE3FF),	"Ii*sL8", 	"48"},

	/* Alternate mnemonics for GNU CC */
{"fmovem",	two(0xF020, 0xE000),	two(0xF1F8, 0xFF00),	"Id#3-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */
{"fmovem",	two(0xF020, 0xE800),	two(0xF1F8, 0xFF8F),	"IiDk-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */

{"fmovem",	two(0xF000, 0xF000),	two(0xF1C0, 0xFF00),	"Id#3&s", 	"48"},	/* fmovem.x to control, static and dynamic: */
{"fmovem",	two(0xF000, 0xF800),	two(0xF1C0, 0xFF8F),	"IiDk&s", 	"48"},	/* fmovem.x to control, static and dynamic: */

{"fmovem",	two(0xF018, 0xD000),	two(0xF1F8, 0xFF00),	"Id+s#3", 	"48"},	/* fmovem.x from autoincrement, static and dynamic: */
{"fmovem",	two(0xF018, 0xD800),	two(0xF1F8, 0xFF8F),	"Ii+sDk", 	"48"},	/* fmovem.x from autoincrement, static and dynamic: */
  
{"fmovem",	two(0xF000, 0xD000),	two(0xF1C0, 0xFF00),	"Id&s#3", 	"48"},	/* fmovem.x from control, static and dynamic: */
{"fmovem",	two(0xF000, 0xD800),	two(0xF1C0, 0xFF8F),	"Ii&sDk", 	"48"},	/* fmovem.x from control, static and dynamic: */

/* fmovemx with register lists */
{"fmovem",	two(0xF020, 0xE000),	two(0xF1F8, 0xFF00),	"IdL3-s", 	"48"},	/* fmovem.x to autodecrement, static and dynamic */
{"fmovem",	two(0xF000, 0xF000),	two(0xF1C0, 0xFF00),	"Idl3&s", 	"48"},	/* fmovem.x to control, static and dynamic: */
{"fmovem",	two(0xF018, 0xD000),	two(0xF1F8, 0xFF00),	"Id+sl3", 	"48"},	/* fmovem.x from autoincrement, static and dynamic: */
{"fmovem",	two(0xF000, 0xD000),	two(0xF1C0, 0xFF00),	"Id&sl3", 	"48"},	/* fmovem.x from control, static and dynamic: */

/* fmoveml a FP-control register */
{"fmovem",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"Iis8@s", 	"48"},
{"fmovem",	two(0xF000, 0x8000),	two(0xF1C0, 0xE3FF),	"Ii*ss8", 	"48"},

/* fmoveml a FP-control reglist */
{"fmovem",	two(0xF000, 0xA000),	two(0xF1C0, 0xE3FF),	"IiL8@s", 	"48"},
{"fmovem",	two(0xF000, 0x8000),	two(0xF2C0, 0xE3FF),	"Ii*sL8", 	"48"},

{"fmulb",	two(0xF000, 0x5823),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fmuld",	two(0xF000, 0x5423),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fmull",	two(0xF000, 0x4023),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fmulp",	two(0xF000, 0x4C23),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fmuls",	two(0xF000, 0x4423),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fmulw",	two(0xF000, 0x5023),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fmulx",	two(0xF000, 0x0023),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fmulx",	two(0xF000, 0x4823),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
/* {"fmulx",	two(0xF000, 0x0023),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"}, JF */

{"fsmulb",	two(0xF000, 0x5863),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsmuld",	two(0xF000, 0x5463),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsmull",	two(0xF000, 0x4063),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsmulp",	two(0xF000, 0x4C63),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsmuls",	two(0xF000, 0x4463),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsmulw",	two(0xF000, 0x5063),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsmulx",	two(0xF000, 0x0063),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsmulx",	two(0xF000, 0x4863),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fdmulb",	two(0xF000, 0x5867),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdmuld",	two(0xF000, 0x5467),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdmull",	two(0xF000, 0x4067),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdmulp",	two(0xF000, 0x4C67),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdmuls",	two(0xF000, 0x4467),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdmulw",	two(0xF000, 0x5067),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdmulx",	two(0xF000, 0x0067),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdmulx",	two(0xF000, 0x4867),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},

{"fnegb",	two(0xF000, 0x581A),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fnegd",	two(0xF000, 0x541A),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fnegl",	two(0xF000, 0x401A),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fnegp",	two(0xF000, 0x4C1A),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fnegs",	two(0xF000, 0x441A),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fnegw",	two(0xF000, 0x501A),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fnegx",	two(0xF000, 0x001A),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fnegx",	two(0xF000, 0x481A),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fnegx",	two(0xF000, 0x001A),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fsnegb",	two(0xF000, 0x585A),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fsnegd",	two(0xF000, 0x545A),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fsnegl",	two(0xF000, 0x405A),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fsnegp",	two(0xF000, 0x4C5A),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fsnegs",	two(0xF000, 0x445A),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsnegw",	two(0xF000, 0x505A),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fsnegx",	two(0xF000, 0x005A),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fsnegx",	two(0xF000, 0x485A),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fsnegx",	two(0xF000, 0x005A),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fdnegb",	two(0xF000, 0x585E),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdnegd",	two(0xF000, 0x545E),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdnegl",	two(0xF000, 0x405E),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdnegp",	two(0xF000, 0x4C5E),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdnegs",	two(0xF000, 0x445E),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdnegw",	two(0xF000, 0x505E),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdnegx",	two(0xF000, 0x005E),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdnegx",	two(0xF000, 0x485E),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fdnegx",	two(0xF000, 0x005E),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fnop",	two(0xF280, 0x0000),	two(0xFFFF, 0xFFFF),	"Ii", 		"48"},

{"fremb",	two(0xF000, 0x5825),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fremd",	two(0xF000, 0x5425),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"freml",	two(0xF000, 0x4025),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fremp",	two(0xF000, 0x4C25),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"frems",	two(0xF000, 0x4425),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fremw",	two(0xF000, 0x5025),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fremx",	two(0xF000, 0x0025),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fremx",	two(0xF000, 0x4825),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
/* {"fremx",	two(0xF000, 0x0025),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"}, JF */

{"frestore",	one(0xF140),		one(0xF1C0),		"Id&s", 	"48p"},
{"frestore",	one(0xF158),		one(0xF1F8),		"Id+s", 	"48p"},
{"fsave",	one(0xF100),		one(0xF1C0),		"Id&s", 	"48p"},
{"fsave",	one(0xF120),		one(0xF1F8),		"Id-s", 	"48p"},

{"fsincosb",	two(0xF000, 0x5830),	two(0xF1C0, 0xFC78),	"Ii;bF7FC", 	"48e"},
{"fsincosd",	two(0xF000, 0x5430),	two(0xF1C0, 0xFC78),	"Ii;FF7FC", 	"48e"},
{"fsincosl",	two(0xF000, 0x4030),	two(0xF1C0, 0xFC78),	"Ii;lF7FC", 	"48e"},
{"fsincosp",	two(0xF000, 0x4C30),	two(0xF1C0, 0xFC78),	"Ii;pF7FC", 	"48e"},
{"fsincoss",	two(0xF000, 0x4430),	two(0xF1C0, 0xFC78),	"Ii;fF7FC", 	"48e"},
{"fsincosw",	two(0xF000, 0x5030),	two(0xF1C0, 0xFC78),	"Ii;wF7FC", 	"48e"},
{"fsincosx",	two(0xF000, 0x0030),	two(0xF1C0, 0xE078),	"IiF8F7FC", 	"48e"},
{"fsincosx",	two(0xF000, 0x4830),	two(0xF1C0, 0xFC78),	"Ii;xF7FC", 	"48e"},

{"fscaleb",	two(0xF000, 0x5826),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fscaled",	two(0xF000, 0x5426),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fscalel",	two(0xF000, 0x4026),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fscalep",	two(0xF000, 0x4C26),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fscales",	two(0xF000, 0x4426),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fscalew",	two(0xF000, 0x5026),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fscalex",	two(0xF000, 0x0026),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fscalex",	two(0xF000, 0x4826),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
/* {"fscalex",	two(0xF000, 0x0026),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"}, JF */

{"fseq",	two(0xF040, 0x0001),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsf",		two(0xF040, 0x0000),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsge",	two(0xF040, 0x0013),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsgl",	two(0xF040, 0x0016),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsgle",	two(0xF040, 0x0017),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsgt",	two(0xF040, 0x0012),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsle",	two(0xF040, 0x0015),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fslt",	two(0xF040, 0x0014),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsne",	two(0xF040, 0x000E),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsnge",	two(0xF040, 0x001C),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsngl",	two(0xF040, 0x0019),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsngle",	two(0xF040, 0x0018),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsngt",	two(0xF040, 0x001D),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsnle",	two(0xF040, 0x001A),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsnlt",	two(0xF040, 0x001B),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsoge",	two(0xF040, 0x0003),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsogl",	two(0xF040, 0x0006),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsogt",	two(0xF040, 0x0002),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsole",	two(0xF040, 0x0005),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsolt",	two(0xF040, 0x0004),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsor",	two(0xF040, 0x0007),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsseq",	two(0xF040, 0x0011),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fssf",	two(0xF040, 0x0010),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fssne",	two(0xF040, 0x001E),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsst",	two(0xF040, 0x001F),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fst",		two(0xF040, 0x000F),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsueq",	two(0xF040, 0x0009),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsuge",	two(0xF040, 0x000B),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsugt",	two(0xF040, 0x000A),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsule",	two(0xF040, 0x000D),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsult",	two(0xF040, 0x000C),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},
{"fsun",	two(0xF040, 0x0008),	two(0xF1C0, 0xFFFF),	"Ii@s", 	"48"},

{"fsgldivb",	two(0xF000, 0x5824),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fsgldivd",	two(0xF000, 0x5424),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fsgldivl",	two(0xF000, 0x4024),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fsgldivp",	two(0xF000, 0x4C24),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fsgldivs",	two(0xF000, 0x4424),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fsgldivw",	two(0xF000, 0x5024),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fsgldivx",	two(0xF000, 0x0024),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fsgldivx",	two(0xF000, 0x4824),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fsgldivx",	two(0xF000, 0x0024),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fsglmulb",	two(0xF000, 0x5827),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fsglmuld",	two(0xF000, 0x5427),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fsglmull",	two(0xF000, 0x4027),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fsglmulp",	two(0xF000, 0x4C27),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fsglmuls",	two(0xF000, 0x4427),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fsglmulw",	two(0xF000, 0x5027),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fsglmulx",	two(0xF000, 0x0027),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fsglmulx",	two(0xF000, 0x4827),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fsglmulx",	two(0xF000, 0x0027),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fsinb",	two(0xF000, 0x580E),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fsind",	two(0xF000, 0x540E),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48ee"},
{"fsinl",	two(0xF000, 0x400E),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fsinp",	two(0xF000, 0x4C0E),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fsins",	two(0xF000, 0x440E),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fsinw",	two(0xF000, 0x500E),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fsinx",	two(0xF000, 0x000E),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fsinx",	two(0xF000, 0x480E),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fsinx",	two(0xF000, 0x000E),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fsinhb",	two(0xF000, 0x5802),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"fsinhd",	two(0xF000, 0x5402),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"fsinhl",	two(0xF000, 0x4002),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"fsinhp",	two(0xF000, 0x4C02),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"fsinhs",	two(0xF000, 0x4402),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"fsinhw",	two(0xF000, 0x5002),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"fsinhx",	two(0xF000, 0x0002),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"fsinhx",	two(0xF000, 0x4802),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"fsinhx",	two(0xF000, 0x0002),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"fsqrtb",	two(0xF000, 0x5804),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fsqrtd",	two(0xF000, 0x5404),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fsqrtl",	two(0xF000, 0x4004),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fsqrtp",	two(0xF000, 0x4C04),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fsqrts",	two(0xF000, 0x4404),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fsqrtw",	two(0xF000, 0x5004),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fsqrtx",	two(0xF000, 0x0004),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fsqrtx",	two(0xF000, 0x4804),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fsqrtx",	two(0xF000, 0x0004),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fssqrtb",	two(0xF000, 0x5841),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fssqrtd",	two(0xF000, 0x5441),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fssqrtl",	two(0xF000, 0x4041),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fssqrtp",	two(0xF000, 0x4C41),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fssqrts",	two(0xF000, 0x4441),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fsssqrtw",	two(0xF000, 0x5041),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fssqrtx",	two(0xF000, 0x0041),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fssqrtx",	two(0xF000, 0x4841),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fssqrtx",	two(0xF000, 0x0041),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fdsqrtb",	two(0xF000, 0x5845),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdsqrtd",	two(0xF000, 0x5445),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdsqrtl",	two(0xF000, 0x4045),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdsqrtp",	two(0xF000, 0x4C45),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdsqrts",	two(0xF000, 0x4445),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdssqrtw",	two(0xF000, 0x5045),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdsqrtx",	two(0xF000, 0x0045),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdsqrtx",	two(0xF000, 0x4845),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fdsqrtx",	two(0xF000, 0x0045),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fsubb",	two(0xF000, 0x5828),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48"},
{"fsubd",	two(0xF000, 0x5428),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48"},
{"fsubl",	two(0xF000, 0x4028),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48"},
{"fsubp",	two(0xF000, 0x4C28),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48"},
{"fsubs",	two(0xF000, 0x4428),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48"},
{"fsubw",	two(0xF000, 0x5028),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48"},
{"fsubx",	two(0xF000, 0x0028),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48"},
{"fsubx",	two(0xF000, 0x4828),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48"},
{"fsubx",	two(0xF000, 0x0028),	two(0xF1C0, 0xE07F),	"IiFt", 	"48"},

{"fssubb",	two(0xF000, 0x5868),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fssubd",	two(0xF000, 0x5468),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fssubl",	two(0xF000, 0x4068),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fssubp",	two(0xF000, 0x4C68),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fssubs",	two(0xF000, 0x4468),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fssubw",	two(0xF000, 0x5068),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fssubx",	two(0xF000, 0x0068),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fssubx",	two(0xF000, 0x4868),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fssubx",	two(0xF000, 0x0068),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"fdsubb",	two(0xF000, 0x586C),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"4"},
{"fdsubd",	two(0xF000, 0x546C),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"4"},
{"fdsubl",	two(0xF000, 0x406C),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"4"},
{"fdsubp",	two(0xF000, 0x4C6C),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"4"},
{"fdsubs",	two(0xF000, 0x446C),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"4"},
{"fdsubw",	two(0xF000, 0x506C),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"4"},
{"fdsubx",	two(0xF000, 0x006C),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"4"},
{"fdsubx",	two(0xF000, 0x486C),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"4"},
{"fdsubx",	two(0xF000, 0x006C),	two(0xF1C0, 0xE07F),	"IiFt", 	"4"},

{"ftanb",	two(0xF000, 0x580F),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"ftand",	two(0xF000, 0x540F),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"ftanl",	two(0xF000, 0x400F),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"ftanp",	two(0xF000, 0x4C0F),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"ftans",	two(0xF000, 0x440F),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"ftanw",	two(0xF000, 0x500F),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"ftanx",	two(0xF000, 0x000F),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"ftanx",	two(0xF000, 0x480F),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"ftanx",	two(0xF000, 0x000F),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"ftanhb",	two(0xF000, 0x5809),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"ftanhd",	two(0xF000, 0x5409),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"ftanhl",	two(0xF000, 0x4009),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"ftanhp",	two(0xF000, 0x4C09),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"ftanhs",	two(0xF000, 0x4409),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"ftanhw",	two(0xF000, 0x5009),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"ftanhx",	two(0xF000, 0x0009),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"ftanhx",	two(0xF000, 0x4809),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"ftanhx",	two(0xF000, 0x0009),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"ftentoxb",	two(0xF000, 0x5812),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"ftentoxd",	two(0xF000, 0x5412),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"ftentoxl",	two(0xF000, 0x4012),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"ftentoxp",	two(0xF000, 0x4C12),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"ftentoxs",	two(0xF000, 0x4412),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"ftentoxw",	two(0xF000, 0x5012),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"ftentoxx",	two(0xF000, 0x0012),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"ftentoxx",	two(0xF000, 0x4812),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"ftentoxx",	two(0xF000, 0x0012),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},

{"ftrapeq",	two(0xF07C, 0x0001),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapf",	two(0xF07C, 0x0000),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapge",	two(0xF07C, 0x0013),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapgl",	two(0xF07C, 0x0016),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapgle",	two(0xF07C, 0x0017),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapgt",	two(0xF07C, 0x0012),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftraple",	two(0xF07C, 0x0015),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftraplt",	two(0xF07C, 0x0014),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapne",	two(0xF07C, 0x000E),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapnge",	two(0xF07C, 0x001C),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapngl",	two(0xF07C, 0x0019),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapngle",	two(0xF07C, 0x0018),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapngt",	two(0xF07C, 0x001D),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapnle",	two(0xF07C, 0x001A),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapnlt",	two(0xF07C, 0x001B),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapoge",	two(0xF07C, 0x0003),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapogl",	two(0xF07C, 0x0006),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapogt",	two(0xF07C, 0x0002),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapole",	two(0xF07C, 0x0005),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapolt",	two(0xF07C, 0x0004),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapor",	two(0xF07C, 0x0007),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapseq",	two(0xF07C, 0x0011),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapsf",	two(0xF07C, 0x0010),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapsne",	two(0xF07C, 0x001E),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapst",	two(0xF07C, 0x001F),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapt",	two(0xF07C, 0x000F),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapueq",	two(0xF07C, 0x0009),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapuge",	two(0xF07C, 0x000B),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapugt",	two(0xF07C, 0x000A),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapule",	two(0xF07C, 0x000D),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapult",	two(0xF07C, 0x000C),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
{"ftrapun",	two(0xF07C, 0x0008),	two(0xF1FF, 0xFFFF),	"Ii", 		"48"},
        
{"ftrapeqw",	two(0xF07A, 0x0001),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapfw",	two(0xF07A, 0x0000),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapgew",	two(0xF07A, 0x0013),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapglw",	two(0xF07A, 0x0016),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapglew",	two(0xF07A, 0x0017),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapgtw",	two(0xF07A, 0x0012),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftraplew",	two(0xF07A, 0x0015),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapltw",	two(0xF07A, 0x0014),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapnew",	two(0xF07A, 0x000E),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapngew",	two(0xF07A, 0x001C),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapnglw",	two(0xF07A, 0x0019),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapnglew",	two(0xF07A, 0x0018),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapngtw",	two(0xF07A, 0x001D),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapnlew",	two(0xF07A, 0x001A),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapnltw",	two(0xF07A, 0x001B),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapogew",	two(0xF07A, 0x0003),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapoglw",	two(0xF07A, 0x0006),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapogtw",	two(0xF07A, 0x0002),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapolew",	two(0xF07A, 0x0005),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapoltw",	two(0xF07A, 0x0004),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftraporw",	two(0xF07A, 0x0007),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapseqw",	two(0xF07A, 0x0011),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapsfw",	two(0xF07A, 0x0010),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapsnew",	two(0xF07A, 0x001E),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapstw",	two(0xF07A, 0x001F),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftraptw",	two(0xF07A, 0x000F),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapueqw",	two(0xF07A, 0x0009),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapugew",	two(0xF07A, 0x000B),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapugtw",	two(0xF07A, 0x000A),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapulew",	two(0xF07A, 0x000D),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapultw",	two(0xF07A, 0x000C),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},
{"ftrapunw",	two(0xF07A, 0x0008),	two(0xF1FF, 0xFFFF),	"Ii^w", 	"48"},

{"ftrapeql",	two(0xF07B, 0x0001),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapfl",	two(0xF07B, 0x0000),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapgel",	two(0xF07B, 0x0013),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapgll",	two(0xF07B, 0x0016),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapglel",	two(0xF07B, 0x0017),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapgtl",	two(0xF07B, 0x0012),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftraplel",	two(0xF07B, 0x0015),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapltl",	two(0xF07B, 0x0014),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapnel",	two(0xF07B, 0x000E),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapngel",	two(0xF07B, 0x001C),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapngll",	two(0xF07B, 0x0019),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapnglel",	two(0xF07B, 0x0018),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapngtl",	two(0xF07B, 0x001D),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapnlel",	two(0xF07B, 0x001A),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapnltl",	two(0xF07B, 0x001B),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapogel",	two(0xF07B, 0x0003),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapogll",	two(0xF07B, 0x0006),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapogtl",	two(0xF07B, 0x0002),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapolel",	two(0xF07B, 0x0005),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapoltl",	two(0xF07B, 0x0004),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftraporl",	two(0xF07B, 0x0007),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapseql",	two(0xF07B, 0x0011),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapsfl",	two(0xF07B, 0x0010),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapsnel",	two(0xF07B, 0x001E),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapstl",	two(0xF07B, 0x001F),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftraptl",	two(0xF07B, 0x000F),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapueql",	two(0xF07B, 0x0009),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapugel",	two(0xF07B, 0x000B),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapugtl",	two(0xF07B, 0x000A),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapulel",	two(0xF07B, 0x000D),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapultl",	two(0xF07B, 0x000C),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},
{"ftrapunl",	two(0xF07B, 0x0008),	two(0xF1FF, 0xFFFF),	"Ii^l", 	"48"},

{"ftstb",	two(0xF000, 0x583A),	two(0xF1C0, 0xFC7F),	"Ii;b", 	"48"},
{"ftstd",	two(0xF000, 0x543A),	two(0xF1C0, 0xFC7F),	"Ii;F", 	"48"},
{"ftstl",	two(0xF000, 0x403A),	two(0xF1C0, 0xFC7F),	"Ii;l", 	"48"},
{"ftstp",	two(0xF000, 0x4C3A),	two(0xF1C0, 0xFC7F),	"Ii;p", 	"48"},
{"ftsts",	two(0xF000, 0x443A),	two(0xF1C0, 0xFC7F),	"Ii;f", 	"48"},
{"ftstw",	two(0xF000, 0x503A),	two(0xF1C0, 0xFC7F),	"Ii;w", 	"48"},
{"ftstx",	two(0xF000, 0x003A),	two(0xF1C0, 0xE07F),	"IiF8", 	"48"},
{"ftstx",	two(0xF000, 0x483A),	two(0xF1C0, 0xFC7F),	"Ii;x", 	"48"},

{"ftwotoxb",	two(0xF000, 0x5811),	two(0xF1C0, 0xFC7F),	"Ii;bF7", 	"48e"},
{"ftwotoxd",	two(0xF000, 0x5411),	two(0xF1C0, 0xFC7F),	"Ii;FF7", 	"48e"},
{"ftwotoxl",	two(0xF000, 0x4011),	two(0xF1C0, 0xFC7F),	"Ii;lF7", 	"48e"},
{"ftwotoxp",	two(0xF000, 0x4C11),	two(0xF1C0, 0xFC7F),	"Ii;pF7", 	"48e"},
{"ftwotoxs",	two(0xF000, 0x4411),	two(0xF1C0, 0xFC7F),	"Ii;fF7", 	"48e"},
{"ftwotoxw",	two(0xF000, 0x5011),	two(0xF1C0, 0xFC7F),	"Ii;wF7", 	"48e"},
{"ftwotoxx",	two(0xF000, 0x0011),	two(0xF1C0, 0xE07F),	"IiF8F7", 	"48e"},
{"ftwotoxx",	two(0xF000, 0x4811),	two(0xF1C0, 0xFC7F),	"Ii;xF7", 	"48e"},
{"ftwotoxx",	two(0xF000, 0x0011),	two(0xF1C0, 0xE07F),	"IiFt", 	"48e"},


{"fjeq",	one(0xF081),		one(0xF1FF),		"IdBc", 	"48"},
{"fjf",		one(0xF080),		one(0xF1FF),		"IdBc", 	"48"},
{"fjge",	one(0xF093),		one(0xF1FF),		"IdBc", 	"48"},
{"fjgl",	one(0xF096),		one(0xF1FF),		"IdBc", 	"48"},
{"fjgle",	one(0xF097),		one(0xF1FF),		"IdBc", 	"48"},
{"fjgt",	one(0xF092),		one(0xF1FF),		"IdBc", 	"48"},
{"fjle",	one(0xF095),		one(0xF1FF),		"IdBc", 	"48"},
{"fjlt",	one(0xF094),		one(0xF1FF),		"IdBc", 	"48"},
{"fjne",	one(0xF08E),		one(0xF1FF),		"IdBc", 	"48"},
{"fjnge",	one(0xF09C),		one(0xF1FF),		"IdBc", 	"48"},
{"fjngl",	one(0xF099),		one(0xF1FF),		"IdBc", 	"48"},
{"fjngle",	one(0xF098),		one(0xF1FF),		"IdBc", 	"48"},
{"fjngt",	one(0xF09D),		one(0xF1FF),		"IdBc", 	"48"},
{"fjnle",	one(0xF09A),		one(0xF1FF),		"IdBc", 	"48"},
{"fjnlt",	one(0xF09B),		one(0xF1FF),		"IdBc", 	"48"},
{"fjoge",	one(0xF083),		one(0xF1FF),		"IdBc", 	"48"},
{"fjogl",	one(0xF086),		one(0xF1FF),		"IdBc", 	"48"},
{"fjogt",	one(0xF082),		one(0xF1FF),		"IdBc", 	"48"},
{"fjole",	one(0xF085),		one(0xF1FF),		"IdBc", 	"48"},
{"fjolt",	one(0xF084),		one(0xF1FF),		"IdBc", 	"48"},
{"fjor",	one(0xF087),		one(0xF1FF),		"IdBc", 	"48"},
{"fjseq",	one(0xF091),		one(0xF1FF),		"IdBc", 	"48"},
{"fjsf",	one(0xF090),		one(0xF1FF),		"IdBc", 	"48"},
{"fjsne",	one(0xF09E),		one(0xF1FF),		"IdBc", 	"48"},
{"fjst",	one(0xF09F),		one(0xF1FF),		"IdBc", 	"48"},
{"fjt",		one(0xF08F),		one(0xF1FF),		"IdBc", 	"48"},
{"fjueq",	one(0xF089),		one(0xF1FF),		"IdBc", 	"48"},
{"fjuge",	one(0xF08B),		one(0xF1FF),		"IdBc", 	"48"},
{"fjugt",	one(0xF08A),		one(0xF1FF),		"IdBc", 	"48"},
{"fjule",	one(0xF08D),		one(0xF1FF),		"IdBc", 	"48"},
{"fjult",	one(0xF08C),		one(0xF1FF),		"IdBc", 	"48"},
{"fjun",	one(0xF088),		one(0xF1FF),		"IdBc", 	"48"},

/* The assembler will ignore attempts to force a short offset */

{"bhis",	one(0061000),		one(0177400),		"Bg", 		"*"},
{"blss",	one(0061400),		one(0177400),		"Bg", 		"*"},
{"bccs",	one(0062000),		one(0177400),		"Bg", 		"*"},
{"bcss",	one(0062400),		one(0177400),		"Bg", 		"*"},
{"bnes",	one(0063000),		one(0177400),		"Bg", 		"*"},
{"beqs",	one(0063400),		one(0177400),		"Bg", 		"*"},
{"bvcs",	one(0064000),		one(0177400),		"Bg", 		"*"},
{"bvss",	one(0064400),		one(0177400),		"Bg", 		"*"},
{"bpls",	one(0065000),		one(0177400),		"Bg", 		"*"},
{"bmis",	one(0065400),		one(0177400),		"Bg", 		"*"},
{"bges",	one(0066000),		one(0177400),		"Bg", 		"*"},
{"blts",	one(0066400),		one(0177400),		"Bg", 		"*"},
{"bgts",	one(0067000),		one(0177400),		"Bg", 		"*"},
{"bles",	one(0067400),		one(0177400),		"Bg", 		"*"},

/* Alternate mnemonics for SUN */

{"jbsr",	one(0060400),		one(0177400),		"Bg", 		"*"},
{"jbsr",	one(0047200),		one(0177700),		"!s", 		"*"},
{"jra",		one(0060000),		one(0177400),		"Bg", 		"*"},
{"jra",		one(0047300),		one(0177700),		"!s", 		"*"},
  
{"jhi",		one(0061000),		one(0177400),		"Bg", 		"*"},
{"jls",		one(0061400),		one(0177400),		"Bg", 		"*"},
{"jcc",		one(0062000),		one(0177400),		"Bg", 		"*"},
{"jcs",		one(0062400),		one(0177400),		"Bg", 		"*"},
{"jne",		one(0063000),		one(0177400),		"Bg", 		"*"},
{"jeq",		one(0063400),		one(0177400),		"Bg", 		"*"},
{"jvc",		one(0064000),		one(0177400),		"Bg", 		"*"},
{"jvs",		one(0064400),		one(0177400),		"Bg", 		"*"},
{"jpl",		one(0065000),		one(0177400),		"Bg", 		"*"},
{"jmi",		one(0065400),		one(0177400),		"Bg", 		"*"},
{"jge",		one(0066000),		one(0177400),		"Bg", 		"*"},
{"jlt",		one(0066400),		one(0177400),		"Bg", 		"*"},
{"jgt",		one(0067000),		one(0177400),		"Bg", 		"*"},
{"jle",		one(0067400),		one(0177400),		"Bg", 		"*"},

/* Short offsets are ignored */

{"jbsrs",	one(0060400),		one(0177400),		"Bg", 		"*"},
{"jras",	one(0060000),		one(0177400),		"Bg", 		"*"},
{"jhis",	one(0061000),		one(0177400),		"Bg", 		"*"},
{"jlss",	one(0061400),		one(0177400),		"Bg", 		"*"},
{"jccs",	one(0062000),		one(0177400),		"Bg", 		"*"},
{"jcss",	one(0062400),		one(0177400),		"Bg", 		"*"},
{"jnes",	one(0063000),		one(0177400),		"Bg", 		"*"},
{"jeqs",	one(0063400),		one(0177400),		"Bg", 		"*"},
{"jvcs",	one(0064000),		one(0177400),		"Bg", 		"*"},
{"jvss",	one(0064400),		one(0177400),		"Bg", 		"*"},
{"jpls",	one(0065000),		one(0177400),		"Bg", 		"*"},
{"jmis",	one(0065400),		one(0177400),		"Bg", 		"*"},
{"jges",	one(0066000),		one(0177400),		"Bg", 		"*"},
{"jlts",	one(0066400),		one(0177400),		"Bg", 		"*"},
{"jgts",	one(0067000),		one(0177400),		"Bg", 		"*"},
{"jles",	one(0067400),		one(0177400),		"Bg", 		"*"},

{"movql",	one(0070000),		one(0170400),		"MsDd", 	"*"},
{"moveql",	one(0070000),		one(0170400),		"MsDd", 	"*"},
{"moval",	one(0020100),		one(0170700),		"*lAd", 	"*"},
{"movaw",	one(0030100),		one(0170700),		"*wAd", 	"*"},
{"movb",	one(0010000),		one(0170000),		";b$d", 	"*"},	/* mov */
{"movl",	one(0070000),		one(0170400),		"MsDd", 	"*"},	/* movq written as mov */
{"movl",	one(0020000),		one(0170000),		"*l$d", 	"*"},
{"movl",	one(0020100),		one(0170700),		"*lAd", 	"*"},
{"movl",	one(0047140),		one(0177770),		"AsUd", 	"*p"},	/* mov to USP */
{"movl",	one(0047150),		one(0177770),		"UdAs", 	"*p"},	/* mov from USP */
{"movc",	one(0047173),		one(0177777),		"R1Jj", 	"1234p"},
{"movc",	one(0047173),		one(0177777),		"R1#j", 	"1234p"},
{"movc",	one(0047172),		one(0177777),		"JjR1", 	"1234p"},
{"movc",	one(0047172),		one(0177777),		"#jR1", 	"1234p"},
{"movml",	one(0044300),		one(0177700),		"#w&s", 	"*"},	/* movm reg to mem. */
{"movml",	one(0044340),		one(0177770),		"#w-s", 	"*"},	/* movm reg to autodecrement. */
{"movml",	one(0046300),		one(0177700),		"!s#w", 	"*"},	/* movm mem to reg. */
{"movml",	one(0046330),		one(0177770),		"+s#w", 	"*"},	/* movm autoinc to reg. */
{"movml",	one(0044300),		one(0177700),		"Lw&s", 	"*"},	/* movm reg to mem. */
{"movml",	one(0044340),		one(0177770),		"lw-s", 	"*"},	/* movm reg to autodecrement. */
{"movml",	one(0046300),		one(0177700),		"!sLw", 	"*"},	/* movm mem to reg. */
{"movml",	one(0046330),		one(0177770),		"+sLw", 	"*"},	/* movm autoinc to reg. */
{"movmw",	one(0044200),		one(0177700),		"#w&s", 	"*"},	/* movm reg to mem. */
{"movmw",	one(0044240),		one(0177770),		"#w-s", 	"*"},	/* movm reg to autodecrement. */
{"movmw",	one(0046200),		one(0177700),		"!s#w", 	"*"},	/* movm mem to reg. */
{"movmw",	one(0046230),		one(0177770),		"+s#w", 	"*"},	/* movm autoinc to reg. */
{"movmw",	one(0044200),		one(0177700),		"Lw&s", 	"*"},	/* movm reg to mem. */
{"movmw",	one(0044240),		one(0177770),		"lw-s", 	"*"},	/* movm reg to autodecrement. */
{"movmw",	one(0046200),		one(0177700),		"!sLw", 	"*"},	/* movm mem to reg. */
{"movmw",	one(0046230),		one(0177770),		"+sLw", 	"*"},	/* movm autoinc to reg. */
{"movpl",	one(0000510),		one(0170770),		"dsDd", 	"*"},	/* memory to register */
{"movpl",	one(0000710),		one(0170770),		"Ddds", 	"*"},	/* register to memory */
{"movpw",	one(0000410),		one(0170770),		"dsDd", 	"*"},	/* memory to register */
{"movpw",	one(0000610),		one(0170770),		"Ddds", 	"*"},	/* register to memory */
{"movq",	one(0070000),		one(0170400),		"MsDd", 	"*"},
{"movw",	one(0030000),		one(0170000),		"*w$d", 	"*"},
{"movw",	one(0030100),		one(0170700),		"*wAd", 	"*"},	/* mova, written as mov */
{"movw",	one(0040300),		one(0177700),		"Ss$s", 	"*p"},	/* Move from sr */
{"movw",	one(0041300),		one(0177700),		"Cs$s", 	"1234"},/* Move from ccr */
{"movw",	one(0042300),		one(0177700),		";wCd", 	"*"},	/* mov to ccr */
{"movw",	one(0043300),		one(0177700),		";wSd", 	"*p"},	/* mov to sr */

{"movsb",	two(0007000, 0),	two(0177700, 07777),	"~sR1", 	"1234p"},
{"movsb",	two(0007000, 04000),	two(0177700, 07777),	"R1~s", 	"1234p"},
{"movsl",	two(0007200, 0),	two(0177700, 07777),	"~sR1", 	"1234p"},
{"movsl",	two(0007200, 04000),	two(0177700, 07777),	"R1~s", 	"1234p"},
{"movsw",	two(0007100, 0),	two(0177700, 07777),	"~sR1", 	"1234p"},
{"movsw",	two(0007100, 04000),	two(0177700, 07777),	"R1~s", 	"1234p"},

#ifdef m68851
#include "pmmu.h"
#endif

};

int numopcodes=sizeof(m68k_opcodes)/sizeof(m68k_opcodes[0]);

struct m68k_opcode *endop = m68k_opcodes+sizeof(m68k_opcodes)/sizeof(m68k_opcodes[0]);;
