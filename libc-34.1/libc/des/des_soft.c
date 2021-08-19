#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_soft.c	1.4 88/05/20 4.0NFSSRC SMI;     from 1.3 88/02/08 (C) 1987 Sun Micro";
#endif
/*
 * Warning!  Things are arranged very carefully in this file to
 * allow read-only data to be moved to the text segment.  The
 * various DES tables must appear before any function definitions
 * (this is arranged by including them immediately below) and partab
 * must also appear before and function definitions
 * This arrangement allows all data up through the first text to
 * be moved to text.
 */

#ifndef KERNEL
#       define CRYPT    /* cannot configure out of user-level code */
#endif
 
#ifdef CRYPT
#ifdef KERNEL
#include "types.h"
#include "des.h"
#else
#include <sys/types.h>
#include "des.h"
#endif
#include "softdes.h"
#include "softdesdata.c"

/*
 * Fast (?) software implementation of DES
 * Has been seen going at 2000 bytes/sec on a Sun-2
 * Works on a VAX too.
 * Won't work without 8 bit chars and 32 bit longs
 */

#define btst(k, b)	(k[b >> 3] & (0x80 >> (b & 07)))
#define	BIT28	(1<<28)

#if	NeXT
static int des_setkey(u_char userkey[], struct deskeydata *kd, 
		      unsigned int dir);
static int des_encrypt(u_char *data, struct deskeydata *kd);
static void do_F(u_long iter, u_long inR, u_long outR, 
		 struct deskeydata *kd);
static void cipher(u_long iter, u_long inR, u_long inL, 
		   u_long outR, u_long outL,  struct deskeydata *kd);
#endif	NeXT

#endif /* def CRYPT */

#ifndef	KERNEL
/*
 * Table giving odd parity in the low bit for ASCII characters
 */
static const char partab[128] = {
	0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x07, 0x07,
	0x08, 0x08, 0x0b, 0x0b, 0x0d, 0x0d, 0x0e, 0x0e,
	0x10, 0x10, 0x13, 0x13, 0x15, 0x15, 0x16, 0x16,
	0x19, 0x19, 0x1a, 0x1a, 0x1c, 0x1c, 0x1f, 0x1f,
	0x20, 0x20, 0x23, 0x23, 0x25, 0x25, 0x26, 0x26,
	0x29, 0x29, 0x2a, 0x2a, 0x2c, 0x2c, 0x2f, 0x2f,
	0x31, 0x31, 0x32, 0x32, 0x34, 0x34, 0x37, 0x37,
	0x38, 0x38, 0x3b, 0x3b, 0x3d, 0x3d, 0x3e, 0x3e,
	0x40, 0x40, 0x43, 0x43, 0x45, 0x45, 0x46, 0x46,
	0x49, 0x49, 0x4a, 0x4a, 0x4c, 0x4c, 0x4f, 0x4f,
	0x51, 0x51, 0x52, 0x52, 0x54, 0x54, 0x57, 0x57,
	0x58, 0x58, 0x5b, 0x5b, 0x5d, 0x5d, 0x5e, 0x5e,
	0x61, 0x61, 0x62, 0x62, 0x64, 0x64, 0x67, 0x67,
	0x68, 0x68, 0x6b, 0x6b, 0x6d, 0x6d, 0x6e, 0x6e,
	0x70, 0x70, 0x73, 0x73, 0x75, 0x75, 0x76, 0x76,
	0x79, 0x79, 0x7a, 0x7a, 0x7c, 0x7c, 0x7f, 0x7f,
};

/*
 * Add odd parity to low bit of 8 byte key
 */
void
des_setparity(p)
	char *p;
{
	int i;

	for (i = 0; i < 8; i++) {
		*p = partab[*p & 0x7f];
		p++;
	}
}
#endif /* def KERNEL */

#ifdef CRYPT
/*
 * Software encrypt or decrypt a block of data (multiple of 8 bytes)
 * Do the CBC ourselves if needed.
 */
_des_crypt(buf, len, desp)
	register char *buf;
	register unsigned len;
	struct desparams *desp;
{
	register short i;
	register unsigned mode;
	register unsigned dir;
	char nextiv[8];
	struct deskeydata softkey;

	mode = (unsigned) desp->des_mode;
	dir = (unsigned) desp->des_dir;
	des_setkey(desp->des_key, &softkey, dir);
	while (len > 0) {
		switch (mode) {
		case CBC:	
			switch (dir) {
			case ENCRYPT:	
				for (i = 0; i < 8; i++)
					buf[i] ^= desp->des_ivec[i];
				des_encrypt((u_char *)buf, &softkey);
				for (i = 0; i < 8; i++)
					desp->des_ivec[i] = buf[i];
				break;	
			case DECRYPT:	
 				for (i = 0; i < 8; i++)
					nextiv[i] = buf[i];
				des_encrypt((u_char *)buf, &softkey);	
				for (i = 0; i < 8; i++) {
					buf[i] ^= desp->des_ivec[i];
					desp->des_ivec[i] = nextiv[i];
				}
				break;
			}
			break;
		case ECB: 
			des_encrypt((u_char *)buf, &softkey);
			break;
		}
		buf += 8;
		len -= 8;
	}
	return (1);
}


/*
 * Set the key and direction for an encryption operation
 * We build the 16 key entries here
 */
static
des_setkey(userkey, kd, dir)
	u_char userkey[8];	
	register struct deskeydata *kd;
	unsigned dir;
{
	register long C, D;
	register short i;

	/*
	 * First, generate C and D by permuting
	 * the key. The low order bit of each
	 * 8-bit char is not used, so C and D are only 28
	 * bits apiece.
	 */
	{
#if	NeXT
		register short bit;
		register const short *pcc = PC1_C;
		register const short *pcd = PC1_D;
#else
		register short bit, *pcc = PC1_C, *pcd = PC1_D;
#endif	NeXT

		C = D = 0;
		for (i=0; i<28; i++) {
			C <<= 1;
			D <<= 1;
			bit = *pcc++;
			if (btst(userkey, bit))
				C |= 1;
			bit = *pcd++;
			if (btst(userkey, bit))
				D |= 1;
		}
	}
	/*
	 * To generate Ki, rotate C and D according
	 * to schedule and pick up a permutation
	 * using PC2.
	 */
	for (i=0; i<16; i++) {
		register chunk_t *c;
		register short j, k, bit;
		register long bbit;

		/*
		 * Do the "left shift" (rotate)
		 * We know we always rotate by either 1 or 2 bits
		 * the shifts table tells us if its 2
		 */
		C <<= 1;
		if (C & BIT28)
			C |= 1;
		D <<= 1;
		if (D & BIT28)
			D |= 1;
		if (shifts[i]) {
			C <<= 1;
			if (C & BIT28)
				C |= 1;
			D <<= 1;
			if (D & BIT28)
				D |= 1;
		}
		/*
		 * get Ki. Note C and D are concatenated.
		 */
		bit = 0;
		switch (dir) {
		case ENCRYPT: 
			c = &kd->keyval[i]; break;
		case DECRYPT: 
			c = &kd->keyval[15 - i]; break;
		}
		c->long0 = 0;
		c->long1 = 0;
		bbit = (1 << 5) << 24;
		for (j=0; j<4; j++) {
			for (k=0; k<6; k++) {
				if (C & (BIT28 >> PC2_C[bit]))
					c->long0 |= bbit >> k;
				if (D & (BIT28 >> PC2_D[bit]))
					c->long1 |= bbit >> k;
				bit++;
			}
			bbit >>= 8;
		}
	
	}
}



/*
 * Do an encryption operation
 * Much pain is taken (with preprocessor) to avoid loops so the compiler
 * can do address arithmetic instead of doing it at runtime.
 * Note that the byte-to-chunk conversion is necessary to guarantee
 * processor byte-order independence.
 */
static
des_encrypt(data, kd)
	register u_char *data;
	register struct deskeydata *kd;
{
	chunk_t work1, work2;

	/* 
	 * Initial permutation 
 	 * and byte to chunk conversion
 	 */
	{
#if	NeXT
		register const u_long *lp;
#else
		register u_long *lp;
#endif	NeXT
		register u_long l0, l1, w;
		register short i, pbit;

		work1.byte0 = data[0];
		work1.byte1 = data[1];
		work1.byte2 = data[2];
		work1.byte3 = data[3];
		work1.byte4 = data[4];
		work1.byte5 = data[5];
		work1.byte6 = data[6];
		work1.byte7 = data[7];
		l0 = l1 = 0;
		w = work1.long0;
		for (lp = &longtab[0], i = 0; i < 32; i++) {
			if (w & *lp++) {
				pbit = IPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		w = work1.long1;
		for (lp = &longtab[0], i = 32; i < 64; i++) {
			if (w & *lp++) {
				pbit = IPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		work2.long0 = l0;
		work2.long1 = l1;
	}

	/*
	 * Apply the 16 ciphering steps
	 */
	{
		register u_long r0, l0, r1, l1;

		l0 = work2.long0;
		r0 = work2.long1;
		cipher( 0, r0, l0, r1, l1, kd);
		cipher( 1, r1, l1, r0, l0, kd);
		cipher( 2, r0, l0, r1, l1, kd);
		cipher( 3, r1, l1, r0, l0, kd);
		cipher( 4, r0, l0, r1, l1, kd);
		cipher( 5, r1, l1, r0, l0, kd);
		cipher( 6, r0, l0, r1, l1, kd);
		cipher( 7, r1, l1, r0, l0, kd);
		cipher( 8, r0, l0, r1, l1, kd);
		cipher( 9, r1, l1, r0, l0, kd);
		cipher(10, r0, l0, r1, l1, kd);
		cipher(11, r1, l1, r0, l0, kd);
		cipher(12, r0, l0, r1, l1, kd);
		cipher(13, r1, l1, r0, l0, kd);
		cipher(14, r0, l0, r1, l1, kd);
		cipher(15, r1, l1, r0, l0, kd);
		work1.long0 = r0;
		work1.long1 = l0;
	}

	/* 
	 * Final permutation
	 * and chunk to byte conversion
	 */
	{
#if	NeXT
		register const u_long *lp;
#else
		register u_long *lp;
#endif	NeXT
		register u_long l0, l1, w;
		register short i, pbit;

		l0 = l1 = 0;
		w = work1.long0;
		for (lp = &longtab[0], i = 0; i < 32; i++) {
			if (w & *lp++) {
				pbit = FPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		w = work1.long1;
		for (lp = &longtab[0], i = 32; i < 64; i++) {
			if (w & *lp++) {
				pbit = FPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		work2.long0 = l0;
		work2.long1 = l1;
	}
	data[0] = work2.byte0;
	data[1] = work2.byte1;
	data[2] = work2.byte2;
	data[3] = work2.byte3;
	data[4] = work2.byte4;
	data[5] = work2.byte5;
	data[6] = work2.byte6;
	data[7] = work2.byte7;
}
#endif /* def CRYPT */


/*
 * Inner part of the algorithm:
 * Expand R from 32 to 48 bits; xor key value;
 * apply S boxes; permute 32 bits of output
 */
/*
 * Expand 8 bits of 32 bit R to 48 bit R
 */
#ifdef __GNU__	
#define	do_R_to_ER(op, b)	{					\
	register const struct R_to_ER *p = &R_to_ER_tab[b][R.byte##b];	\
	e0 op p->l0;							\
	e1 op p->l1;							\
}
#else
#define	do_R_to_ER(op, b)	{			\
	register struct R_to_ER *p = &R_to_ER_tab[b][R.byte/**/b];	\
	e0 op p->l0;				\
	e1 op p->l1;				\
}
#endif __GNU__

static void do_F(u_long iter, u_long inR, u_long outR, struct deskeydata *kd)
{
	chunk_t R, ER;
	register u_long e0, e1;
	register const struct R_to_ER *p;
	R.long0 = inR;		

	p = &R_to_ER_tab[0][R.byte0];
	e0 = p->l0;
	e1 = p->l1;

	p = &R_to_ER_tab[1][R.byte1];
	e0 |= p->l0;
	e1 |= p->l1;

	p = &R_to_ER_tab[2][R.byte0];
	e0 |= p->l0;
	e1 |= p->l1;

	p = &R_to_ER_tab[3][R.byte3];
	e0 |= p->l0;
	e1 |= p->l1;

#ifdef notdef
	do_R_to_ER(=,0);	
	do_R_to_ER(|=,1);	
	do_R_to_ER(|=,2);	
	do_R_to_ER(|=,3);	
#endif notdef

	ER.long0 = e0 ^ kd->keyval[iter].long0;
	ER.long1 = e1 ^ kd->keyval[iter].long1;
	R.long0 =				
		S_tab[0][ER.byte0] +		
		S_tab[1][ER.byte1] +		
		S_tab[2][ER.byte2] +		
		S_tab[3][ER.byte3] +		
		S_tab[4][ER.byte4] +		
		S_tab[5][ER.byte5] +		
		S_tab[6][ER.byte6] +		
		S_tab[7][ER.byte7]; 		
	outR =					
		P_tab[0][R.byte0] +		
		P_tab[1][R.byte1] +		
		P_tab[2][R.byte2] +		
		P_tab[3][R.byte3];  
}

/*
 * Do a cipher step
 * Apply inner part; do xor and exchange of 32 bit parts
 */
static void cipher(u_long iter, u_long inR, u_long inL, 
		   u_long outR, u_long outL,  struct deskeydata *kd)
{
	do_F(iter, inR, outR, kd);	
	outR ^= inL;
	outL = inR;
}

