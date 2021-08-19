/*	NXCType.h */

/* Copyright (c) 1990 NeXT, Inc. - 8/21/90 RM */
/* patterned after ctype.h ; for 8-bit encoding, Europe only */

#ifndef _NXCTYPE_H
#define _NXCTYPE_H

#define	_U	01	/* Upper case */
#define	_L	02	/* Lower case */
#define	_N	04	/* Numeric */
#define	_S	010	/* Space */
#define _P	020	/* Printable */
#define _C	040	/* Control */
#define _X	0100	/* Hex Digit */
#define	_B	0200	/* Blank */
/*
 * The following three not presently used (need more than
 * 8 bits to represent them).
 */
#define _G	0400	/* Ligature (w/ "_U" or "_L" */
#define _Z	0800	/* Superscript (w/ "_P") */
#define _A	1000	/* Accent/Diacritical mark (w/ "_P") */

extern int NXIsAlNum(unsigned c);
extern int NXIsAlpha(unsigned c);
extern int NXIsCntrl(unsigned c);
extern int NXIsDigit(unsigned c);
extern int NXIsGraph(unsigned c);
extern int NXIsLower(unsigned c);
extern int NXIsPrint(unsigned c);
extern int NXIsPunct(unsigned c);
extern int NXIsSpace(unsigned c);
extern int NXIsUpper(unsigned c);
extern int NXIsXDigit(unsigned c);
extern int _NXToLower(unsigned c);
extern int _NXToUpper(unsigned c);
extern int NXToLower(unsigned c);
extern int NXToUpper(unsigned c);
extern int NXIsAscii(unsigned c);
extern unsigned char *NXToAscii(unsigned c);

/*
 * Data structures used by the internationized NX... versions of the
 * ctype(3) routines.  These structures are private to the above routines
 * and should NOT be referenced by the application.
 */
extern unsigned char _NX_CTypeTable_[];	/* char types */
extern unsigned char _NX_ULTable_[]; 	/* case conversion table */

#endif /* _NXCTYPE_H */
