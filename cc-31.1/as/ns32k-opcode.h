/* ns32k-opcode.h */

/* Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of Gas, the GNU Assembler.

The GNU assembler is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU Assembler General
Public License for full details.

Everyone is granted permission to copy, modify and redistribute
the GNU Assembler, but only under the conditions described in the
GNU Assembler General Public License.  A copy of this license is
supposed to have been given to you along with the GNU Assembler
so you can know your rights and responsibilities.  It should be
in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.  */
/*bug: string inst */

/*
   After deciding the instruction entry (via hash.c) the instruction parser
   will try to match the operands after the instruction to the required set
   given in the entry operandfield. Every operand will result in a change in
   the opcode or the addition of data to the opcode.
   The operands in the source instruction are checked for inconsistent
   semantics.
 
	F : 32 bit float	general form
	L : 64 bit float	    "
	B : byte		    "
	W : word		    "
	D : double-word		    "
	Q : quad-word		    "
	A : double-word		gen-address-form ie no regs allowed
	d : displacement
	b : displacement - pc relative addressing  acb
	p : displacement - pc relative addressing  br bcond bsr cxp
	q : quick
	i : immediate (8 bits)
	    This is not a standard ns32k operandtype, it is used to build
	    instructions like    svc arg1,arg2
	    Svc is the instruction SuperVisorCall and is sometimes used to
	    call OS-routines from usermode. Some args might be handy!
	r : register number (3 bits)
	O : setcfg instruction optionslist
	C : cinv instruction optionslist
	S : stringinstruction optionslist
	U : registerlist	save,enter
	u : registerlist	restore,exit
	M : mmu register
	P : cpu register
	g : 3:rd operand of inss or exts instruction
	G : 4:th operand of inss or exts instruction
	    Those operands are encoded in the same byte.
	    This byte is placed last in the instruction.
	f : operand of sfsr
	H : sequent-hack for bsr (Warning)

column	1 	instructions
	2 	number of bits in opcode.
	3 	number of bits in opcode explicitly
		determined by the instruction type.
	4 	opcodeseed, the number we build our opcode
		from.
	5 	operandtypes, used by operandparser.
	6 	size in bytes of immediate
*/
struct ns32k_opcode {
  char *name;
  unsigned char opcode_id_size; /* not used by the assembler */
  unsigned char opcode_size;
  unsigned long opcode_seed;
  char *operands;
  unsigned char im_size;	/* not used by dissassembler */
  char *default_args;		/* default to those args when none given */
  char default_modec;		/* default to this addr-mode when ambigous
				   ie when the argument of a general addr-mode
				   is a plain constant */
  char default_model;		/* is a plain label */
};

struct ns32k_opcode ns32k_opcodes[]=
{
  { "absf",	14,24,	0x35be,	"1F2F",		4,	"",	20,21	},
  { "absl",	14,24,	0x34be,	"1L2L",		8,	"",	20,21	},
  { "absb",	14,24,	0x304e, "1B2B",		1,	"",	20,21	},
  { "absw",	14,24,	0x314e, "1W2W",		2,	"",	20,21	},
  { "absd",	14,24,	0x334e, "1D2D",		4,	"",	20,21	},
  { "acbb",	 7,16,	0x4c,	"2B1q3d",	1,	"",	20,21	},
  { "acbw",	 7,16,	0x4d,	"2W1q3d",	2,	"",	20,21	},
  { "acbd",	 7,16,	0x4f,	"2D1q3d",	4,	"",	20,21	},
  { "addf",	14,24,	0x01be,	"1F2F",		4,	"",	20,21	},
  { "addl",	14,24,	0x00be, "1L2L",		8,	"",	20,21	},
  { "addb",	 6,16,	0x00,	"1B2B",		1,	"",	20,21	},
  { "addw",	 6,16,	0x01,	"1W2W",		2,	"",	20,21	},
  { "addd",	 6,16,	0x03,	"1D2D",		4,	"",	20,21	},
  { "addcb",	 6,16,	0x10,	"1B2B",		1,	"",	20,21	},
  { "addcw",	 6,16,	0x11,	"1W2W",		2,	"",	20,21	},
  { "addcd",	 6,16,	0x13,	"1D2D",		4,	"",	20,21	},
  { "addpb",	14,24,	0x3c4e,	"1B2B",		1,	"",	20,21	},
  { "addpw",	14,24,	0x3d4e,	"1W2W",		2,	"",	20,21	},
  { "addpd",	14,24,	0x3f4e,	"1D2D",		4,	"",	20,21	},
  { "addqb",	 7,16,	0x0c,	"2B1q",		1,	"",	20,21	},
  { "addqw",	 7,16,	0x0d,	"2W1q",		2,	"",	20,21	},
  { "addqd",	 7,16,	0x0f,	"2D1q",		4,	"",	20,21	},
  { "addr",	 6,16,	0x27,	"1A2D",		4,	"",	21,21	},
  { "adjspb",	11,16,	0x057c,	"1B",		1,	"",	20,21	},
  { "adjspw",	11,16,	0x057d,	"1W", 		2,	"",	20,21	},
  { "adjspd",	11,16,	0x057f,	"1D", 		4,	"",	20,21	},
  { "andb",	 6,16,	0x28,	"1B2B",		1,	"",	20,21	},
  { "andw",	 6,16,	0x29,	"1W2W",		1,	"",	20,21	},
  { "andd",	 6,16,	0x2b,	"1D2D",		1,	"",	20,21	},
  { "ashb",	14,24,	0x044e,	"1B2B",		1,	"",	20,21	},
  { "ashw",	14,24,	0x054e,	"1B2W",		1,	"",	20,21	},
  { "ashd",	14,24,	0x074e,	"1B2D",		1,	"",	20,21	},
  { "beq",	 8,8,	0x0a,	"1p",		0,	"",	20,21	},
  { "bne",	 8,8,	0x1a,	"1p",		0,	"",	20,21	},
  { "bcs",	 8,8,	0x2a,	"1p",		0,	"",	20,21	},
  { "bcc",	 8,8,	0x3a,	"1p",		0,	"",	20,21	},
  { "bhi",	 8,8,	0x4a,	"1p",		0,	"",	20,21	},
  { "bls",	 8,8,	0x5a,	"1p",		0,	"",	20,21	},
  { "bgt",	 8,8,	0x6a,	"1p",		0,	"",	20,21	},
  { "ble",	 8,8,	0x7a,	"1p",		0,	"",	20,21	},
  { "bfs",	 8,8,	0x8a,	"1p",		0,	"",	20,21	},
  { "bfc",	 8,8,	0x9a,	"1p",		0,	"",	20,21	},
  { "blo",	 8,8,	0xaa,	"1p",		0,	"",	20,21	},
  { "bhs",	 8,8,	0xba,	"1p",		0,	"",	20,21	},
  { "blt",	 8,8,	0xca,	"1p",		0,	"",	20,21	},
  { "bge",	 8,8,	0xda,	"1p",		0,	"",	20,21	},
  { "but",	 8,8,	0xea,	"1p",		0,	"",	20,21	},
  { "buf",	 8,8,	0xfa,	"1p",		0,	"",	20,21	},
  { "bicb",	 6,16,	0x08,	"1B2B",		1,	"",	20,21	},
  { "bicw",	 6,16,	0x09,	"1W2W",		2,	"",	20,21	},
  { "bicd",	 6,16,	0x0b,	"1D2D",		4,	"",	20,21	},
  { "bicpsrb",	11,16,	0x17c,	"1B",		1,	"",	20,21	},
  { "bicpsrw",	11,16,	0x17d,	"1W",		2,	"",	20,21	},
  { "bispsrb",	11,16,	0x37c,	"1B",		1,	"",	20,21	},
  { "bispsrw",	11,16,	0x37d,	"1W",		2,	"",	20,21	},
  { "bpt",	 8,8,	0xf2,	"",		0,	"",	20,21	},
  { "br",	 8,8,	0xea,	"1p",		0,	"",	20,21	},
  { "bsr",	 8,8,	0x02,	"1H",		0,	"",	20,21	},
  { "caseb",	11,16,	0x77c,	"1B",		1,	"",	20,21	},
  { "casew",	11,16,	0x77d,	"1W",		2,	"",	20,21	},
  { "cased",	11,16,	0x77f,	"1D",		4,	"",	20,21	},
  { "cbitb",	14,24,	0x084e,	"1B2D",		1,	"",	20,21	},
  { "cbitw",	14,24,	0x094e,	"1W2D",		2,	"",	20,21	},
  { "cbitd",	14,24,	0x0b4e,	"1D2D",		4,	"",	20,21	},
  { "cbitib",	14,24,	0x0c4e,	"1B2D",		1,	"",	20,21	},
  { "cbitiw",	14,24,	0x0d4e,	"1W2D",		2,	"",	20,21	},
  { "cbitid",	14,24,	0x0f4e,	"1D2D",		4,	"",	20,21	},
  { "checkb",	11,24,	0x0ee,	"2A3B1r",	1,	"",	20,21	},
  { "checkw",	11,24,	0x1ee,	"2A3W1r",	2,	"",	20,21	},
  { "checkd",	11,24,	0x3ee,	"2A3D1r",	4,	"",	20,21	},
  { "cinv",	14,24,	0x271e,	"2C2D",		4,	"",	20,21	},
  { "cmpf",	14,24,	0x09be,	"1F2F",		4,	"",	20,21	},
  { "cmpl",	14,24,	0x08be,	"1L2L",		8,	"",	20,21	},
  { "cmpb",	 6,16,	0x04,	"1B2B",		1,	"",	20,21	},
  { "cmpw",	 6,16,	0x05,	"1W2W",		2,	"",	20,21	},
  { "cmpd",	 6,16,	0x07,	"1D2D",		4,	"",	20,21	},
  { "cmpmb",	14,24,	0x04ce,	"1A2A3b",	1,	"",	20,21	},
  { "cmpmw",	14,24,	0x05ce,	"1A2A3b",	2,	"",	20,21	},
  { "cmpmd",	14,24,	0x07ce,	"1A2A3b",	4,	"",	20,21	},
  { "cmpqb",	 7,16,	0x1c,	"2B1q",		0,	"",	20,21	},
  { "cmpqw",	 7,16,	0x1d,	"2W1q",		0,	"",	20,21	},
  { "cmpqd",	 7,16,	0x1f,	"2D1q",		0,	"",	20,21	},
  { "cmpsb",	16,24,	0x040e,	"1S",		0,	"[]",	20,21	},
  { "cmpsw",	16,24,	0x050e,	"1S",		0,	"[]",	20,21	},
  { "cmpsd",	16,24,	0x070e,	"1S",		0,	"[]",	20,21	},
  { "cmpst",	16,24,	0x840e,	"1S",		0,	"[]",	20,21	},
  { "comb",	14,24,	0x344e,	"1B2B",		1,	"",	20,21	},
  { "comw",	14,24,	0x354e,	"1W2W",		2,	"",	20,21	},
  { "comd",	14,24,	0x374e,	"1D2D",		4,	"",	20,21	},
  { "cvtp",	11,24,	0x036e,	"2D3D1r",	4,	"",	20,21	},
  { "cxp",	 8,8,	0x22,	"1p",		0,	"",	20,21	},
  { "cxpd",	11,16,	0x07f,	"1D",		4,	"",	20,21	},
  { "deib",	14,24,	0x2cce,	"1B2W",		1,	"",	20,21	},
  { "deiw",	14,24,	0x2bce,	"1W2D",		2,	"",	20,21	},
  { "deid",	14,24,	0x2fce,	"1D2Q",		4,	"",	20,21	},
  { "dia",	 8,8,	0xc2,	"",		1,	"",	20,21	},
  { "divf",	14,24,	0x21be,	"1F2F",		4,	"",	20,21	},
  { "divl",	14,24,	0x20be,	"1L2L",		8,	"",	20,21	},
  { "divb",	14,24,	0x3cce,	"1B2B",		1,	"",	20,21	},
  { "divw",	14,24,	0x3dce,	"1W2W",		2,	"",	20,21	},
  { "divd",	14,24,	0x3fce,	"1D2D",		4,	"",	20,21	},
  { "enter",	 8,8,	0x82,	"1U2d",		0,	"",	20,21	},
  { "exit",	 8,8,	0x92,	"1u",		0,	"",	20,21	},
  { "extb",	11,24,	0x02e,	"2D3B1r4d",	1,	"",	20,21	},
  { "extw",	11,24,	0x12e,	"2D3W1r4d",	2,	"",	20,21	},
  { "extd",	11,24,	0x32e,	"2D3D1r4d",	4,	"",	20,21	},
  { "extsb",	14,24,	0x0cce,	"1D2B3g4G",	1,	"",	20,21	},
  { "extsw",	14,24,	0x0dce,	"1D2W3g4G",	2,	"",	20,21	},
  { "extsd",	14,24,	0x0fce,	"1D2D3g4G",	4,	"",	20,21	},
  { "ffsb",	14,24,	0x046e,	"1B2B",		1,	"",	20,21	},
  { "ffsw",	14,24,	0x056e,	"1W2B",		2,	"",	20,21	},
  { "ffsd",	14,24,	0x076e,	"1D2B",		4,	"",	20,21	},
  { "flag",	 8,8,	0xd2,	"",		0,	"",	20,21	},
  { "floorfb",	14,24,	0x3c3e,	"1F2B",		4,	"",	20,21	},
  { "floorfw",	14,24,	0x3d3e,	"1F2W",		4,	"",	20,21	},
  { "floorfd",	14,24,	0x3f3e,	"1F2D",		4,	"",	20,21	},
  { "floorlb",	14,24,	0x383e,	"1L2B",		8,	"",	20,21	},
  { "floorlw",	14,24,	0x393e,	"1L2W",		8,	"",	20,21	},
  { "floorld",	14,24,	0x3b3e,	"1L2D",		8,	"",	20,21	},
  { "ibitb",	14,24,	0x384e,	"1B2D",		1,	"",	20,21	},
  { "ibitw",	14,24,	0x394e,	"1W2D",		2,	"",	20,21	},
  { "ibitd",	14,24,	0x3b4e,	"1D2D",		4,	"",	20,21	},
  { "indexb",	11,24,	0x42e,	"2B3B1r",	1,	"",	20,21	},
  { "indexw",	11,24,	0x52e,	"2W3W1r",	2,	"",	20,21	},
  { "indexd",	11,24,	0x72e,	"2D3D1r",	4,	"",	20,21	},
  { "insb",	11,24,	0x0ae,	"2B3B1r4d",	1,	"",	20,21	},
  { "insw",	11,24,	0x1ae,	"2W3W1r4d",	2,	"",	20,21	},
  { "insd",	11,24,	0x3ae,	"2D3D1r4d",	4,	"",	20,21	},
  { "inssb",	14,24,	0x08ce,	"1B2D3g4G",	1,	"",	20,21	},
  { "inssw",	14,24,	0x09ce,	"1W2D3g4G",	2,	"",	20,21	},
  { "inssd",	14,24,	0x0bce,	"1D2D3g4G",	4,	"",	20,21	},
  { "jsr",	11,16,	0x67f,	"1A",		4,	"",	20,21	},
  { "jump",	11,16,	0x27f,	"1A",		4,	"",	20,21	},
  { "lfsr",	19,24,	0x00f3e,"1D",		4,	"",	20,21	},
  { "lmr",	15,24,	0x0b1e,	"2D1M",		4,	"",	20,21	},
  { "lprb",	 7,16,	0x6c,	"2B1P",		1,	"",	20,21	},
  { "lprw",	 7,16,	0x6d,	"2W1P",		2,	"",	20,21	},
  { "lprd",	 7,16,	0x6f,	"2D1P",		4,	"",	20,21	},
  { "lshb",	14,24,	0x144e,	"1B2B",		1,	"",	20,21	},
  { "lshw",	14,24,	0x154e,	"1B2W",		2,	"",	20,21	},
  { "lshd",	14,24,	0x174e,	"1B2D",		4,	"",	20,21	},
  { "meib",	14,24,	0x24ce,	"1B2W",		1,	"",	20,21	},
  { "meiw",	14,24,	0x25ce,	"1W2D",		2,	"",	20,21	},
  { "meid",	14,24,	0x27ce,	"1D2Q",		4,	"",	20,21	},
  { "modb",	14,24,	0x38ce,	"1B2B",		1,	"",	20,21	},
  { "modw",	14,24,	0x39ce,	"1W2W",		2,	"",	20,21	},
  { "modd",	14,24,	0x3bce,	"1D2D",		4,	"",	20,21	},
  { "movf",	14,24,	0x05be,	"1F2F",		4,	"",	20,21	},
  { "movl",	14,24,	0x04be,	"1L2L",		8,	"",	20,21	},
  { "movb",	 6,16,	0x14,	"1B2B",		1,	"",	20,21	},
  { "movw",	 6,16,	0x15,	"1W2W",		2,	"",	20,21	},
  { "movd",	 6,16,	0x17,	"1D2D",		4,	"",	20,21	},
  { "movbf",	14,24,	0x043e,	"1B2F",		1,	"",	20,21	},
  { "movwf",	14,24,	0x053e,	"1W2F",		2,	"",	20,21	},
  { "movdf",	14,24,	0x073e,	"1D2F",		4,	"",	20,21	},
  { "movbl",	14,24,	0x003e,	"1B2L",		1,	"",	20,21	},
  { "movwl",	14,24,	0x013e,	"1W2L",		2,	"",	20,21	},
  { "movdl",	14,24,	0x033e,	"1D2L",		4,	"",	20,21	},
  { "movfl",	14,24,	0x1b3e,	"1F2L",		4,	"",	20,21	},
  { "movlf",	14,24,	0x163e,	"1L2F",		8,	"",	20,21	},
  { "movmb",	14,24,	0x00ce,	"1A2A3b",	1,	"",	20,21	},
  { "movmw",	14,24,	0x01ce,	"1A2A3b",	2,	"",	20,21	},
  { "movmd",	14,24,	0x03ce,	"1A2A3b",	4,	"",	20,21	},
  { "movqb",	 7,16,	0x5c,	"2B1q",		1,	"",	20,21	},
  { "movqw",	 7,16,	0x5d,	"2B1q",		2,	"",	20,21	},
  { "movqd",	 7,16,	0x5f,	"2B1q",		4,	"",	20,21	},
  { "movsb",	16,24,	0x000e,	"1S",		0,	"[]",	20,21	},
  { "movsw",	16,24,	0x010e,	"1S",		0,	"[]",	20,21	},
  { "movsd",	16,24,	0x030e,	"1S",		0,	"[]",	20,21	},
  { "movst",	16,24,	0x800e,	"1S",		0,	"[]",	20,21	},
  { "movsub",	14,24,	0x0cae,	"1A2A",		1,	"",	20,21	},
  { "movsuw",	14,24,	0x0dae,	"1A2A",		2,	"",	20,21	},
  { "movsud",	14,24,	0x0fae,	"1A2A",		4,	"",	20,21	},
  { "movusb",	14,24,	0x1cae,	"1A2A",		1,	"",	20,21	},
  { "movusw",	14,24,	0x1dae,	"1A2A",		2,	"",	20,21	},
  { "movusd",	14,24,	0x1fae,	"1A2A",		4,	"",	20,21	},
  { "movxbd",	14,24,	0x1cce,	"1B2D",		1,	"",	20,21	},
  { "movxwd",	14,24,	0x1dce,	"1W2D",		2,	"",	20,21	},
  { "movxbw",	14,24,	0x10ce,	"1B2W",		1,	"",	20,21	},
  { "movzbd",	14,24,	0x18ce,	"1B2D",		1,	"",	20,21	},
  { "movzwd",	14,24,	0x19ce,	"1W2D",		2,	"",	20,21	},
  { "movzbw",	14,24,	0x14ce,	"1B2W",		1,	"",	20,21	},
  { "mulf",	14,24,	0x31be,	"1F2F",		4,	"",	20,21	},
  { "mull",	14,24,	0x30be,	"1L2L",		8,	"",	20,21	},
  { "mulb",	14,24,	0x20ce, "1B2B",		1,	"",	20,21	},
  { "mulw",	14,24,	0x21ce, "1W2W",		2,	"",	20,21	},
  { "muld",	14,24,	0x23ce, "1D2D",		4,	"",	20,21	},
  { "negf",	14,24,	0x15be, "1F2F",		4,	"",	20,21	},
  { "negl",	14,24,	0x14be, "1L2L",		8,	"",	20,21	},
  { "negb",	14,24,	0x204e, "1B2B",		1,	"",	20,21	},
  { "negw",	14,24,	0x214e, "1W2W",		2,	"",	20,21	},
  { "negd",	14,24,	0x234e, "1D2D",		4,	"",	20,21	},
  { "nop",	 8,8,	0xa2,	"",		0,	"",	20,21	},
  { "notb",	14,24,	0x244e, "1B2B",		1,	"",	20,21	},
  { "notw",	14,24,	0x254e, "1W2W",		2,	"",	20,21	},
  { "notd",	14,24,	0x274e, "1D2D",		4,	"",	20,21	},
  { "orb",	 6,16,	0x18,	"1B2B",		1,	"",	20,21	},
  { "orw",	 6,16,	0x19,	"1W2W",		2,	"",	20,21	},
  { "ord",	 6,16,	0x1b,	"1D2D",		4,	"",	20,21	},
  { "quob",	14,24,	0x30ce,	"1B2B",		1,	"",	20,21	},
  { "quow",	14,24,	0x31ce,	"1W2W",		2,	"",	20,21	},
  { "quod",	14,24,	0x33ce,	"1D2D",		4,	"",	20,21	},
  { "rdval",	19,24,	0x0031e,"1A",		4,	"",	20,21	},
  { "remb",	14,24,	0x34ce,	"1B2B",		1,	"",	20,21	},
  { "remw",	14,24,	0x35ce,	"1W2W",		2,	"",	20,21	},
  { "remd",	14,24,	0x37ce,	"1D2D",		4,	"",	20,21	},
  { "restore",	 8,8,	0x72,	"1u",		0,	"",	20,21	},
  { "ret",	 8,8,	0x12,	"1d",		0,	"",	20,21	},
  { "reti",	 8,8,	0x52,	"",		0,	"",	20,21	},
  { "rett",	 8,8,	0x42,	"1d",		0,	"",	20,21	},
  { "rotb",	14,24,	0x004e,	"1B2B",		1,	"",	20,21	},
  { "rotw",	14,24,	0x014e,	"1B2W",		1,	"",	20,21	},
  { "rotd",	14,24,	0x034e,	"1B2D",		1,	"",	20,21	},
  { "roundfb",	14,24,	0x243e,	"1F2B",		4,	"",	20,21	},
  { "roundfw",	14,24,	0x253e,	"1F2W",		4,	"",	20,21	},
  { "roundfd",	14,24,	0x273e,	"1F2D",		4,	"",	20,21	},
  { "roundlb",	14,24,	0x203e,	"1L2B",		8,	"",	20,21	},
  { "roundlw",	14,24,	0x213e,	"1L2W",		8,	"",	20,21	},
  { "roundld",	14,24,	0x233e,	"1L2D",		8,	"",	20,21	},
  { "rxp",	 8,8,	0x32,	"1d",		0,	"",	20,21	},
  { "seqb",	11,16,	0x3c,	"1B",		0,	"",	20,21	},
  { "seqw",	11,16,	0x3d,	"1W",		0,	"",	20,21	},
  { "seqd",	11,16,	0x3f,	"1D",		0,	"",	20,21	},
  { "sneb",	11,16,	0xbc,	"1B",		0,	"",	20,21	},
  { "snew",	11,16,	0xbd,	"1W",		0,	"",	20,21	},
  { "sned",	11,16,	0xbf,	"1D",		0,	"",	20,21	},
  { "scsb",	11,16,	0x13c,	"1B",		0,	"",	20,21	},
  { "scsw",	11,16,	0x13d,	"1W",		0,	"",	20,21	},
  { "scsd",	11,16,	0x13f,	"1D",		0,	"",	20,21	},
  { "sccb",	11,16,	0x1bc,	"1B",		0,	"",	20,21	},
  { "sccw",	11,16,	0x1bd,	"1W",		0,	"",	20,21	},
  { "sccd",	11,16,	0x1bf,	"1D",		0,	"",	20,21	},
  { "shib",	11,16,	0x23c,	"1B",		0,	"",	20,21	},
  { "shiw",	11,16,	0x23d,	"1W",		0,	"",	20,21	},
  { "shid",	11,16,	0x23f,	"1D",		0,	"",	20,21	},
  { "slsb",	11,16,	0x2bc,	"1B",		0,	"",	20,21	},
  { "slsw",	11,16,	0x2bd,	"1W",		0,	"",	20,21	},
  { "slsd",	11,16,	0x2bf,	"1D",		0,	"",	20,21	},
  { "sgtb",	11,16,	0x33c,	"1B",		0,	"",	20,21	},
  { "sgtw",	11,16,	0x33d,	"1W",		0,	"",	20,21	},
  { "sgtd",	11,16,	0x33f,	"1D",		0,	"",	20,21	},
  { "sleb",	11,16,	0x3bc,	"1B",		0,	"",	20,21	},
  { "slew",	11,16,	0x3bd,	"1W",		0,	"",	20,21	},
  { "sled",	11,16,	0x3bf,	"1D",		0,	"",	20,21	},
  { "sfsb",	11,16,	0x43c,	"1B",		0,	"",	20,21	},
  { "sfsw",	11,16,	0x43d,	"1W",		0,	"",	20,21	},
  { "sfsd",	11,16,	0x43f,	"1D",		0,	"",	20,21	},
  { "sfcb",	11,16,	0x4bc,	"1B",		0,	"",	20,21	},
  { "sfcw",	11,16,	0x4bd,	"1W",		0,	"",	20,21	},
  { "sfcd",	11,16,	0x4bf,	"1D",		0,	"",	20,21	},
  { "slob",	11,16,	0x53c,	"1B",		0,	"",	20,21	},
  { "slow",	11,16,	0x53d,	"1W",		0,	"",	20,21	},
  { "slod",	11,16,	0x53f,	"1D",		0,	"",	20,21	},
  { "shsb",	11,16,	0x5bc,	"1B",		0,	"",	20,21	},
  { "shsw",	11,16,	0x5bd,	"1W",		0,	"",	20,21	},
  { "shsd",	11,16,	0x5bf,	"1D",		0,	"",	20,21	},
  { "sltb",	11,16,	0x63c,	"1B",		0,	"",	20,21	},
  { "sltw",	11,16,	0x63d,	"1W",		0,	"",	20,21	},
  { "sltd",	11,16,	0x63f,	"1D",		0,	"",	20,21	},
  { "sgeb",	11,16,	0x6bc,	"1B",		0,	"",	20,21	},
  { "sgew",	11,16,	0x6bd,	"1W",		0,	"",	20,21	},
  { "sged",	11,16,	0x6bf,	"1D",		0,	"",	20,21	},
  { "sutb",	11,16,	0x73c,	"1B",		0,	"",	20,21	},
  { "sutw",	11,16,	0x73d,	"1W",		0,	"",	20,21	},
  { "sutd",	11,16,	0x73f,	"1D",		0,	"",	20,21	},
  { "sufb",	11,16,	0x7bc,	"1B",		0,	"",	20,21	},
  { "sufw",	11,16,	0x7bd,	"1W",		0,	"",	20,21	},
  { "sufd",	11,16,	0x7bf,	"1D",		0,	"",	20,21	},
  { "save",	 8,8,	0x62,	"1U",		0,	"",	20,21	},
  { "sbitb",    14,24,	0x184e,	"1B2A",		1,	"",	20,21	},
  { "sbitw",	14,24,	0x194e,	"1W2A",		2,	"",	20,21	},
  { "sbitd",	14,24,	0x1b4e,	"1D2A",		4,	"",	20,21	},
  { "sbitib",	14,24,	0x1c4e,	"1B2A",		1,	"",	20,21	},
  { "sbitiw",	14,24,	0x1d4e,	"1W2A",		2,	"",	20,21	},
  { "sbitid",	14,24,	0x1f4e,	"1D2A",		4,	"",	20,21	},
  { "setcfg",	15,24,	0x0b0e,	"1O",		0,	"",	20,21	},
  { "sfsr",	14,24,	0x373e,	"1f",		0,	"",	20,21	},
  { "skpsb",	16,24,	0x0c0e,	"1S",		0,	"[]",	20,21	},
  { "skpsw",	16,24,	0x0d0e,	"1S",		0,	"[]",	20,21	},
  { "skpsd",	16,24,	0x0f0e, "1S",		0,	"[]",	20,21	},
  { "skpst",	16,24,	0x8c0e,	"1S",		0,	"[]",	20,21	},
  { "smr",	15,24,	0x0f1e,	"2D1M",		4,	"",	20,21	},
  { "sprb",	 7,16,	0x2c,	"2B1P",		1,	"",	20,21	},
  { "sprw",	 7,16,	0x2d,	"2W1P",		2,	"",	20,21	},
  { "sprd",	 7,16,	0x2f,	"2D1P",		4,	"",	20,21	},
  { "subf",	14,24,	0x11be,	"1F2F",		4,	"",	20,21	},
  { "subl",	14,24,	0x10be,	"1L2L",		8,	"",	20,21	},
  { "subb",	 6,16,	0x20,	"1B2B",		1,	"",	20,21	},
  { "subw",	 6,16,	0x21,	"1W2W",		2,	"",	20,21	},
  { "subd",	 6,16,	0x23,	"1D2D",		4,	"",	20,21	},
  { "subcb",	 6,16,	0x30,	"1B2B",		1,	"",	20,21	},
  { "subcw",	 6,16,	0x31,	"1W2W",		2,	"",	20,21	},
  { "subcd",	 6,16,	0x33,	"1D2D",		4,	"",	20,21	},
  { "subpb",	14,24,	0x2c4e,	"1B2B",		1,	"",	20,21	},
  { "subpw",	14,24,	0x2d4e,	"1W2W",		2,	"",	20,21	},
  { "subpd",	14,24,	0x2f4e,	"1D2D",		4,	"",	20,21	},
#ifdef NS32K_SVC_IMMED_OPERANDS
  { "svc",	 8,8,	0xe2,	"2i1i",		1,	"",	20,21	}, /* not really, but unix uses it */
#else
  { "svc",	 8,8,	0xe2,	"",		0,	"",	20,21	},
#endif
  { "tbitb",	 6,16,	0x34,	"1B2A",		1,	"",	20,21	},
  { "tbitw",	 6,16,	0x35,	"1W2A",		2,	"",	20,21	},
  { "tbitd",	 6,16,	0x37,	"1D2A",		4,	"",	20,21	},
  { "truncfb",	14,24,	0x2c3e,	"1F2B",		4,	"",	20,21	},
  { "truncfw",	14,24,	0x2d3e,	"1F2W",		4,	"",	20,21	},
  { "truncfd",	14,24,	0x2f3e,	"1F2D",		4,	"",	20,21	},
  { "trunclb",	14,24,	0x283e,	"1L2B",		8,	"",	20,21	},
  { "trunclw",	14,24,	0x293e,	"1L2W",		8,	"",	20,21	},
  { "truncld",	14,24,	0x2b3e,	"1L2D",		8,	"",	20,21	},
  { "wait",	 8,8,	0xb2,	"",		0,	"",	20,21	},
  { "wrval",	19,24,	0x0071e,"1A",		0,	"",	20,21	},
  { "xorb",	 6,16,	0x38,	"1B2B",		1,	"",	20,21	},
  { "xorw",	 6,16,	0x39,	"1W2W",		2,	"",	20,21	},
  { "xord",	 6,16,	0x3b,	"1D2D",		4,	"",	20,21	}
};

int numopcodes=sizeof(ns32k_opcodes)/sizeof(ns32k_opcodes[0]);

struct ns32k_opcode *endop = ns32k_opcodes+sizeof(ns32k_opcodes)/sizeof(ns32k_opcodes[0]);

#define MAX_ARGS 4
#define ARG_LEN 50

