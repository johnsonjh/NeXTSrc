/*
	tiffCompress.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#define NeXT_MOD

/*
 * This file contains the original tif_compress.c, tif_lzw.c, tif_dumpmode.c,
 * tif_next.c, tif_packbits.c, and tif_fax3.c.
 *
 * Original copyright:
 *
 * Copyright (c) 1988, 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

/*
 * TIFF Library
 *
 * Compression Scheme Configuration Support.
 */

#ifdef NeXT_MOD

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>

#import "tiffPrivate.h"

#else
#include "tiffio.h"
#endif

#ifdef NeXT_MOD
#define extern static
#endif

extern	int TIFFInitDumpMode();
#ifdef CCITT_SUPPORT
extern	int TIFFInitCCITTRLE(), TIFFInitCCITTRLEW();
#endif
extern	int TIFFInitPackBits();
#ifdef FAX3_SUPPORT
extern	int TIFFInitCCITTFax3();
#endif
#ifdef FAX4_SUPPORT
extern	int TIFFInitCCITTFax4();
#endif
#ifdef THUNDER_SUPPORT
extern	int TIFFInitThunderScan();
#endif
#ifdef PICIO_SUPPORT
extern	int TIFFInitPicio();
#endif
#ifdef NEXT_SUPPORT
extern	int TIFFInitNeXT();
#endif
#ifdef SGI_SUPPORT
extern	int TIFFInitSGI();
#endif
extern	int TIFFInitLZW();
#ifdef NeXT_MOD
#ifdef JPEG_SUPPORT
extern	int TIFFInitJPEG();
#endif
#endif

#ifdef NeXT_MOD
#undef extern
#endif

#ifdef NeXT_MOD
static const struct cscheme {
#else
static struct cscheme {
#endif
	int	scheme;
	int	(*init)();
} CompressionSchemes[] = {
    { COMPRESSION_NONE,		TIFFInitDumpMode },
    { COMPRESSION_LZW,		TIFFInitLZW },
    { COMPRESSION_PACKBITS,	TIFFInitPackBits },
#ifdef PICIO_SUPPORT
    { COMPRESSION_PICIO,	TIFFInitPicio },
#endif
#ifdef THUNDER_SUPPORT
    { COMPRESSION_THUNDERSCAN,	TIFFInitThunderScan },
#endif
#ifdef NEXT_SUPPORT
    { COMPRESSION_NEXT,		TIFFInitNeXT },
#endif
#ifdef SGI_SUPPORT
    { COMPRESSION_SGIRLE,	TIFFInitSGI },
#endif
#ifdef CCITT_SUPPORT
    { COMPRESSION_CCITTRLE,	TIFFInitCCITTRLE },
    { COMPRESSION_CCITTRLEW,	TIFFInitCCITTRLEW },
#endif
#ifdef FAX3_SUPPORT
    { COMPRESSION_CCITTFAX3,	TIFFInitCCITTFax3 },
#endif
#ifdef FAX4_SUPPORT
    { COMPRESSION_CCITTFAX4,	TIFFInitCCITTFax4 },
#endif
#ifdef NeXT_MOD
#ifdef JPEG_SUPPORT
    { COMPRESSION_JPEG,		TIFFInitJPEG },
#endif
#endif
};
#define	NSCHEMES (sizeof (CompressionSchemes) / sizeof (CompressionSchemes[0]))

#ifdef NeXT_MOD
/*
 * TIFFSetCompressionScheme returns 0 if there was an error (the compression
 * scheme wasn't found) or 1 if there was no error.
 */
int _NXTIFFSetCompressionScheme(TIFF *tif, int scheme)
#else
TIFFSetCompressionScheme(tif, scheme)
	TIFF *tif;
	int scheme;
#endif
{
#ifdef NeXT_MOD
	register const struct cscheme *c;
#else
	register struct cscheme *c;
#endif

	for (c = CompressionSchemes; c < &CompressionSchemes[NSCHEMES]; c++)
		if (c->scheme == scheme) {
			tif->tif_stripdecode = NULL;
			tif->tif_stripencode = NULL;
			tif->tif_encodestrip = NULL;
			tif->tif_close = NULL;
			tif->tif_seek = NULL;
			tif->tif_cleanup = NULL;
			tif->tif_flags &= ~TIFF_NOBITREV;
			tif->tif_options = 0;
			return ((*c->init)(tif));
		}
	TIFFError(tif->tif_name, "Unknown data compression algorithm %u (0x%x)",
	    scheme, scheme);
	return (0);
}



/* End of original tif_compress.c */

/* Start of original tif_lzw.c */

/*
 * TIFF Library.
 * Rev 5.0 Lempel-Ziv & Welch Compression Support
 *
 * This code is derived from the compress program whose code is
 * derived from software contributed to Berkeley by James A. Woods,
 * derived from original work by Spencer Thomas and Joseph Orost.
 *
 * The original Berkeley copyright notice appears below in its entirety.
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#include <stdio.h>
#include <assert.h>
#else
#import <assert.h>
#endif

#define MAXCODE(n)	((1 << (n)) - 1)
/*
 * The TIFF spec specifies that encoded bit strings range
 * from 9 to 12 bits.  This is somewhat unfortunate in that
 * experience indicates full color RGB pictures often need
 * ~14 bits for reasonable compression.
 */
#define	BITS_MIN	9		/* start with 9 bits */
#define	BITS_MAX	12		/* max of 12 bit strings */
/* predefined codes */
#define	CODE_CLEAR	256		/* code to clear string table */
#define	CODE_EOI	257		/* end-of-information code */
#define CODE_FIRST	258		/* first free code entry */
#define	CODE_MAX	MAXCODE(BITS_MAX)
#ifdef notdef
#define	HSIZE		9001		/* 91% occupancy */
#define	HSHIFT		(8-(16-13))
#else
#define	HSIZE		5003		/* 80% occupancy */
#define	HSHIFT		(8-(16-12))
#endif

/*
 * NB: The 5.0 spec describes a different algorithm than Aldus
 *     implements.  Specifically, Aldus does code length transitions
 *     one code earlier than should be done (for real LZW).
 *     Earlier versions of this library implemented the correct
 *     LZW algorithm, but emitted codes in a bit order opposite
 *     to the TIFF spec.  Thus, to maintain compatibility w/ Aldus
 *     we interpret MSB-LSB ordered codes to be images written w/
 *     old versions of this library, but otherwise adhere to the
 *     Aldus "off by one" algorithm.
 *
 * Future revisions to the TIFF spec are expected to "clarify this issue".
 */
#define	SetMaxCode(sp, v) {			\
	(sp)->lzw_maxcode = (v)-1;		\
	if ((sp)->lzw_flags & LZW_COMPAT)	\
		(sp)->lzw_maxcode++;		\
}

/*
 * Decoding-specific state.
 */
struct decode {
	short	prefixtab[HSIZE];	/* prefix(code) */
	u_char	suffixtab[CODE_MAX+1];	/* suffix(code) */
	u_char	stack[HSIZE-(CODE_MAX+1)];
	u_char	*stackp;		/* stack pointer */
	int	firstchar;		/* of string associated w/ last code */
};

/*
 * Encoding-specific state.
 */
struct encode {
	long	checkpoint;		/* point at which to clear table */
#define CHECK_GAP	10000		/* enc_ratio check interval */
	long	ratio;			/* current compression ratio */
	long	incount;		/* (input) data bytes encoded */
	long	outcount;		/* encoded (output) bytes */
	long	htab[HSIZE];		/* hash table */
	short	codetab[HSIZE];		/* code table */
};

/*
 * State block for each open TIFF
 * file using LZW compression/decompression.
 */
typedef	struct {
	int	lzw_oldcode;		/* last code encountered */
	u_char	lzw_hordiff;
#define	LZW_HORDIFF4	0x01		/* hor. diff w/ 4-bit samples */
#define	LZW_HORDIFF8	0x02		/* hor. diff w/ 8-bit samples */
#define	LZW_HORDIFF16	0x04		/* hor. diff w/ 16-bit samples */
#define	LZW_HORDIFF32	0x08		/* hor. diff w/ 32-bit samples */
	u_char	lzw_flags;
#define	LZW_RESTART	0x01		/* restart interrupted decode */
#define	LZW_COMPAT	0x02		/* read old bit-reversed codes */
	u_short	lzw_nbits;		/* number of bits/code */
	u_short	lzw_stride;		/* horizontal diferencing stride */
	int	lzw_maxcode;		/* maximum code for lzw_nbits */
	long	lzw_bitoff;		/* bit offset into data */
	long	lzw_bitsize;		/* size of strip in bits */
	int	lzw_free_ent;		/* next free entry in hash table */
	union {
		struct	decode dec;
		struct	encode enc;
	} u;
} LZWState;
#define	dec_prefix	u.dec.prefixtab
#define	dec_suffix	u.dec.suffixtab
#define	dec_stack	u.dec.stack
#define	dec_stackp	u.dec.stackp
#define	dec_firstchar	u.dec.firstchar

#define	enc_checkpoint	u.enc.checkpoint
#define	enc_ratio	u.enc.ratio
#define	enc_incount	u.enc.incount
#define	enc_outcount	u.enc.outcount
#define	enc_htab	u.enc.htab
#define	enc_codetab	u.enc.codetab

/* masks for extracting/inserting variable length bit codes */
#ifdef NeXT_MOD
static const u_char rmask[9] =
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
static const u_char lmask[9] =
    { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
#else
static	u_char rmask[9] =
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
static	u_char lmask[9] =
    { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
#endif

#if USE_PROTOTYPES
static	int LZWPreEncode(TIFF*);
static	int LZWEncode(TIFF*, u_char*, int);
static	int LZWPostEncode(TIFF*);
static	int LZWDecode(TIFF*, u_char*, int);
static	int LZWPreDecode(TIFF*);
static	int LZWCleanup(TIFF*);
static	int GetNextCode(TIFF*);
static	void PutNextCode(TIFF*, int);
static	void cl_block(TIFF*);
static	void cl_hash(LZWState*);
#else
static	int LZWPreEncode(), LZWEncode(), LZWPostEncode();
static	int LZWDecode(), LZWPreDecode();
static	int LZWCleanup();
static	int GetNextCode();
static	void PutNextCode();
static	void cl_block();
static	void cl_hash();
#endif

#ifdef NeXT_MOD
static int
#endif
TIFFInitLZW(tif)
	TIFF *tif;
{
	tif->tif_stripdecode = LZWPreDecode;
	tif->tif_decoderow = LZWDecode;
	tif->tif_stripencode = LZWPreEncode;
	tif->tif_encoderow = LZWEncode;
	tif->tif_encodestrip = LZWPostEncode;
	tif->tif_cleanup = LZWCleanup;
	return (1);
}

static
#ifdef NeXT_MOD
int
#endif
LZWCheckPredictor(tif, sp)
	TIFF *tif;
	LZWState *sp;
{
	TIFFDirectory *td = &tif->tif_dir;

	switch (td->td_predictor) {
	case 1:
		break;
	case 2:
		sp->lzw_stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
		    td->td_samplesperpixel : 1);
		if (td->td_bitspersample == 8) {
			sp->lzw_hordiff = LZW_HORDIFF8;
			break;
		}
		if (td->td_bitspersample == 16) {
			sp->lzw_hordiff = LZW_HORDIFF16;
			break;
		}
		TIFFError(tif->tif_name,
    "Horizontal differencing \"Predictor\" not supported with %d-bit samples",
		    td->td_bitspersample);
		return (0);
	default:
		TIFFError(tif->tif_name, "\"Predictor\" value %d not supported",
		    td->td_predictor);
		return (0);
	}
	return (1);
}

/*
 * LZW Decoder.
 */

/*
 * Setup state for decoding a strip.
 */
static
#ifdef NeXT_MOD
int
#endif
LZWPreDecode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code;

	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (LZWState));
#ifndef NeXT_MOD
		if (tif->tif_data == NULL) {
			TIFFError("LZWPreDecode",
			    "No space for LZW state block");
			return (0);
		}
#endif
		sp = (LZWState *)tif->tif_data;
		sp->lzw_flags = 0;
		sp->lzw_hordiff = 0;
		if (!LZWCheckPredictor(tif, sp))
			return (0);
	} else
		sp->lzw_flags &= ~LZW_RESTART;
	sp->lzw_nbits = BITS_MIN;
	/*
	 * Pre-load the table.
	 */
	for (code = 255; code >= 0; code--)
		sp->dec_suffix[code] = (u_char)code;
	sp->lzw_free_ent = CODE_FIRST;
	sp->lzw_bitoff = 0;
	/* calculate data size in bits */
	sp->lzw_bitsize = tif->tif_rawdatasize;
	sp->lzw_bitsize = (sp->lzw_bitsize << 3) - (BITS_MAX-1);
	sp->dec_stackp = sp->dec_stack;
	sp->lzw_oldcode = -1;
	sp->dec_firstchar = -1;
	/*
	 * Check for old bit-reversed codes.  All the flag
	 * manipulations are to insure only one warning is
	 * given for a file.
	 */
	if (tif->tif_rawdata[0] == 0 && (tif->tif_rawdata[1] & 0x1)) {
		if ((sp->lzw_flags & LZW_COMPAT) == 0)
			TIFFWarning(tif->tif_name,
			    "Old-style LZW codes, convert file");
		sp->lzw_flags |= LZW_COMPAT;
	} else
		sp->lzw_flags &= ~LZW_COMPAT;
	SetMaxCode(sp, MAXCODE(BITS_MIN));
	return (1);
}

#define REPEAT4(n, op)		\
    switch (n) {		\
    default: { int i; for (i = n-4; i > 0; i--) { op; } } \
    case 4:  op;		\
    case 3:  op;		\
    case 2:  op;		\
    case 1:  op;		\
    case 0:  ;			\
    }

static void
horizontalAccumulate8(cp, cc, stride)
	register char *cp;
	register int cc;
	register int stride;
{
	if (cc > stride) {
		cc -= stride;
		do {
			REPEAT4(stride, cp[stride] += cp[0]; cp++)
			cc -= stride;
		} while (cc > 0);
	}
}

static void
horizontalAccumulate16(wp, wc, stride)
	register short *wp;
	register int wc;
	register int stride;
{
	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
			wc -= stride;
		} while (wc > 0);
	}
}

/*
 * Decode the next scanline.
 */
static
#ifdef NeXT_MOD
int
#endif
LZWDecode(tif, op0, occ0)
	TIFF *tif;
	u_char *op0;
#ifdef NeXT_MOD
	int occ0;
#endif
{
	register char *op = (char *)op0;
	register int occ = occ0;
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code;
	register u_char *stackp;
	int firstchar, oldcode, incode;

	stackp = sp->dec_stackp;
	/*
	 * Restart interrupted unstacking operations.
	 */
	if (sp->lzw_flags & LZW_RESTART) {
		do {
			if (--occ < 0) {	/* end of scanline */
				sp->dec_stackp = stackp;
				return (1);
			}
			*op++ = *--stackp;
		} while (stackp > sp->dec_stack);
		sp->lzw_flags &= ~LZW_RESTART;
	}
	oldcode = sp->lzw_oldcode;
	firstchar = sp->dec_firstchar;
	while (occ > 0 && (code = GetNextCode(tif)) != CODE_EOI) {
		if (code == CODE_CLEAR) {
			bzero(sp->dec_prefix, sizeof (sp->dec_prefix));
			sp->lzw_free_ent = CODE_FIRST;
			sp->lzw_nbits = BITS_MIN;
			SetMaxCode(sp, MAXCODE(BITS_MIN));
			if ((code = GetNextCode(tif)) == CODE_EOI)
				break;
			*op++ = code, occ--;
			oldcode = firstchar = code;
			continue;
		}
		incode = code;
		/*
		 * When a code is not in the table we use (as spec'd):
		 *    StringFromCode(oldcode) +
		 *        FirstChar(StringFromCode(oldcode))
		 */
		if (code >= sp->lzw_free_ent) {	/* code not in table */
			*stackp++ = firstchar;
			code = oldcode;
		}

		/*
		 * Generate output string (first in reverse).
		 */
		for (; code >= 256; code = sp->dec_prefix[code])
			*stackp++ = sp->dec_suffix[code];
		*stackp++ = firstchar = sp->dec_suffix[code];
		do {
			if (--occ < 0) {	/* end of scanline */
				sp->lzw_flags |= LZW_RESTART;
				break;
			}
			*op++ = *--stackp;
		} while (stackp > sp->dec_stack);

		/*
		 * Add the new entry to the code table.
		 */
		if ((code = sp->lzw_free_ent) < CODE_MAX) {
			sp->dec_prefix[code] = (u_short)oldcode;
			sp->dec_suffix[code] = firstchar;
			sp->lzw_free_ent++;
			/*
			 * If the next entry is too big for the
			 * current code size, then increase the
			 * size up to the maximum possible.
			 */
			if (sp->lzw_free_ent > sp->lzw_maxcode) {
				sp->lzw_nbits++;
				if (sp->lzw_nbits > BITS_MAX)
					sp->lzw_nbits = BITS_MAX;
				SetMaxCode(sp, MAXCODE(sp->lzw_nbits));
			}
		} 
		oldcode = incode;
	}
	sp->dec_stackp = stackp;
	sp->lzw_oldcode = oldcode;
	sp->dec_firstchar = firstchar;
	switch (sp->lzw_hordiff) {
	case LZW_HORDIFF8:
		horizontalAccumulate8((char *)op0, occ0, sp->lzw_stride);
		break;
	case LZW_HORDIFF16:
		horizontalAccumulate16((short *)op0, occ0/2, sp->lzw_stride);
		break;
	}
	if (occ > 0) {
		TIFFError(tif->tif_name,
	    "LZWDecode: Not enough data for scanline %d (decode %d bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}

/*
 * Get the next code from the raw data buffer.
 */
static
#ifdef NeXT_MOD
int
#endif
GetNextCode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code, bits;
	register long r_off;
	register u_char *bp;

	/*
	 * This check shouldn't be necessary because each
	 * strip is suppose to be terminated with CODE_EOI.
	 * At worst it's a substitute for the CODE_EOI that's
	 * supposed to be there (see calculation of lzw_bitsize
	 * in LZWPreDecode()).
	 */
	if (sp->lzw_bitoff > sp->lzw_bitsize) {
		TIFFWarning(tif->tif_name,
		    "LZWDecode: Strip %d not terminated with EOI code",
		    tif->tif_curstrip);
		return (CODE_EOI);
	}
	r_off = sp->lzw_bitoff;
	bits = sp->lzw_nbits;
	/*
	 * Get to the first byte.
	 */
	bp = (u_char *)tif->tif_rawdata + (r_off >> 3);
	r_off &= 7;
	if (sp->lzw_flags & LZW_COMPAT) {
		/* Get first part (low order bits) */
		code = (*bp++ >> r_off);
		r_off = 8 - r_off;		/* now, offset into code word */
		bits -= r_off;
		/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
		if (bits >= 8) {
			code |= *bp++ << r_off;
			r_off += 8;
			bits -= 8;
		}
		/* high order bits. */
		code |= (*bp & rmask[bits]) << r_off;
	} else {
		r_off = 8 - r_off;		/* convert offset to count */
		code = *bp++ & rmask[r_off];	/* high order bits */
		bits -= r_off;
		if (bits >= 8) {
			code = (code<<8) | *bp++;
			bits -= 8;
		}
		/* low order bits */
		code = (code << bits) | ((*bp & lmask[bits]) >> (8 - bits));
	}
	sp->lzw_bitoff += sp->lzw_nbits;
	return (code);
}

/*
 * LZW Encoding.
 */

/*
 * Reset encoding state at the start of a strip.
 */
static
#ifdef NeXT_MOD
int
#endif
LZWPreEncode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;

	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (LZWState));
#ifndef NeXT_MOD
		if (tif->tif_data == NULL) {
			TIFFError("LZWPreEncode",
			    "No space for LZW state block");
			return (0);
		}
#endif
		sp = (LZWState *)tif->tif_data;
		sp->lzw_flags = 0;
		sp->lzw_hordiff = 0;
		if (!LZWCheckPredictor(tif, sp))
			return (0);
	}
	sp->enc_ratio = 0;
	sp->enc_checkpoint = CHECK_GAP;
	SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
	sp->lzw_free_ent = CODE_FIRST;
	sp->lzw_bitoff = 0;
	sp->lzw_bitsize = (tif->tif_rawdatasize << 3) - (BITS_MAX-1);
	cl_hash(sp);		/* clear hash table */
	sp->lzw_oldcode = -1;	/* generates CODE_CLEAR in LZWEncode */
	return (1);
}

static void
horizontalDifference8(cp, cc, stride)
	register char *cp;
	register int cc;
	register int stride;
{
	if (cc > stride) {
		cc -= stride;
		cp += cc - 1;
		do {
			REPEAT4(stride, cp[stride] -= cp[0]; cp--)
			cc -= stride;
		} while (cc > 0);
	}
}

static void
horizontalDifference16(wp, wc, stride)
	register short *wp;
	register int wc;
	register int stride;
{
	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] -= wp[0]; wp--)
			wc -= stride;
		} while (wc > 0);
	}
}

/*
 * Encode a scanline of pixels.
 *
 * Uses an open addressing double hashing (no chaining) on the 
 * prefix code/next character combination.  We do a variant of
 * Knuth's algorithm D (vol. 3, sec. 6.4) along with G. Knott's
 * relatively-prime secondary probe.  Here, the modular division
 * first probe is gives way to a faster exclusive-or manipulation. 
 * Also do block compression with an adaptive reset, whereby the
 * code table is cleared when the compression ratio decreases,
 * but after the table fills.  The variable-length output codes
 * are re-sized at this point, and a CODE_CLEAR is generated
 * for the decoder. 
 */
static
#ifdef NeXT_MOD
int
#endif
LZWEncode(tif, bp, cc)
	TIFF *tif;
	u_char *bp;
	int cc;
{
#ifndef NeXT_MOD
	static char module[] = "LZWEncode";
#endif
	register LZWState *sp;
	register long fcode;
	register int h, c, ent, disp;

	if ((sp = (LZWState *)tif->tif_data) == NULL)
		return (0);
/* XXX horizontal differencing alters user's data XXX */
	switch (sp->lzw_hordiff) {
	case LZW_HORDIFF8:
		horizontalDifference8((char *)bp, cc, sp->lzw_stride);
		break;
	case LZW_HORDIFF16:
		horizontalDifference16((short *)bp, cc/2, sp->lzw_stride);
		break;
	}
	ent = sp->lzw_oldcode;
	if (ent == -1 && cc > 0) {
		PutNextCode(tif, CODE_CLEAR);
		ent = *bp++; cc--; sp->enc_incount++;
	}
	while (cc > 0) {
		c = *bp++; cc--; sp->enc_incount++;
		fcode = ((long)c << BITS_MAX) + ent;
		h = (c << HSHIFT) ^ ent;	/* xor hashing */
		if (sp->enc_htab[h] == fcode) {
			ent = sp->enc_codetab[h];
			continue;
		}
		if (sp->enc_htab[h] >= 0) {
			/*
			 * Primary hash failed, check secondary hash.
			 */
			disp = HSIZE - h;
			if (h == 0)
				disp = 1;
			do {
				if ((h -= disp) < 0)
					h += HSIZE;
				if (sp->enc_htab[h] == fcode) {
					ent = sp->enc_codetab[h];
					goto hit;
				}
			} while (sp->enc_htab[h] >= 0);
		}
		/*
		 * New entry, emit code and add to table.
		 */
		PutNextCode(tif, ent);
		ent = c;
		sp->enc_codetab[h] = sp->lzw_free_ent++;
		sp->enc_htab[h] = fcode;
		if (sp->lzw_free_ent == CODE_MAX-1) {
			/* table is full, emit clear code and reset */
			sp->enc_ratio = 0;
			cl_hash(sp);
			sp->lzw_free_ent = CODE_FIRST;
			PutNextCode(tif, CODE_CLEAR);
			SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
		} else {
			if (sp->enc_incount >= sp->enc_checkpoint)
				cl_block(tif);
			/*
			 * If the next entry is going to be too big for
			 * the code size, then increase it, if possible.
			 */
			if (sp->lzw_free_ent > sp->lzw_maxcode) {
				sp->lzw_nbits++;
				assert(sp->lzw_nbits <= BITS_MAX);
				SetMaxCode(sp, MAXCODE(sp->lzw_nbits)+1);
			}
		}
	hit:
		;
	}
	sp->lzw_oldcode = ent;
	return (1);
}

/*
 * Finish off an encoded strip by flushing the last
 * string and tacking on an End Of Information code.
 */
static
#ifdef NeXT_MOD
int
#endif
LZWPostEncode(tif)
	TIFF *tif;
{
	LZWState *sp = (LZWState *)tif->tif_data;

	if (sp->lzw_oldcode != -1)
		PutNextCode(tif, sp->lzw_oldcode);
	PutNextCode(tif, CODE_EOI);
	return (1);
}

static void
PutNextCode(tif, c)
	TIFF *tif;
	int c;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register long r_off;
	register int bits, code = c;
	register char *bp;

	r_off = sp->lzw_bitoff;
	bits = sp->lzw_nbits;
	/*
	 * Flush buffer if code doesn't fit.
	 */
	if (r_off + bits > sp->lzw_bitsize) {
		/*
		 * Calculate the number of full bytes that can be
		 * written and save anything else for the next write.
		 */
		if (r_off & 7) {
			tif->tif_rawcc = r_off >> 3;
			bp = tif->tif_rawdata + tif->tif_rawcc;
			(void) TIFFFlushData1(tif);
			tif->tif_rawdata[0] = *bp;
		} else {
			/*
			 * Otherwise, on a byte boundary (in
			 * which tiff_rawcc is already correct).
			 */
			(void) TIFFFlushData1(tif);
		}
		bp = tif->tif_rawdata;
		sp->lzw_bitoff = (r_off &= 7);
	} else {
		/*
		 * Get to the first byte.
		 */
		bp = tif->tif_rawdata + (r_off >> 3);
		r_off &= 7;
	}
	/*
	 * Note that lzw_bitoff is maintained as the bit offset
	 * into the buffer w/ a right-to-left orientation (i.e.
	 * lsb-to-msb).  The bits, however, go in the file in
	 * an msb-to-lsb order.
	 */
	bits -= (8 - r_off);
	*bp = (*bp & lmask[r_off]) | (code >> bits);
	bp++;
	if (bits >= 8) {
		bits -= 8;
		*bp++ = code >> bits;
	}
	if (bits)
		*bp = (code & rmask[bits]) << (8 - bits);
	/*
	 * enc_outcount is used by the compression analysis machinery
	 * which resets the compression tables when the compression
	 * ratio goes up.  lzw_bitoff is used here (in PutNextCode) for
	 * inserting codes into the output buffer.  tif_rawcc must
	 * be updated for the mainline write code in TIFFWriteScanline()
	 * so that data is flushed when the end of a strip is reached.
	 * Note that the latter is rounded up to ensure that a non-zero
	 * byte count is present. 
	 */
	sp->enc_outcount += sp->lzw_nbits;
	sp->lzw_bitoff += sp->lzw_nbits;
	tif->tif_rawcc = (sp->lzw_bitoff + 7) >> 3;
}

/*
 * Check compression ratio and, if things seem to
 * be slipping, clear the hash table and reset state.
 */
static void
cl_block(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register long rat;

	sp->enc_checkpoint = sp->enc_incount + CHECK_GAP;
	if (sp->enc_incount > 0x007fffff) {	/* shift will overflow */
		rat = sp->enc_outcount >> 8;
		rat = (rat == 0 ? 0x7fffffff : sp->enc_incount / rat);
	} else
		rat = (sp->enc_incount<<8)/sp->enc_outcount; /* 8 fract bits */
	if (rat <= sp->enc_ratio) {
		sp->enc_ratio = 0;
		cl_hash(sp);
		sp->lzw_free_ent = CODE_FIRST;
		PutNextCode(tif, CODE_CLEAR);
		SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
	} else
		sp->enc_ratio = rat;
}

/*
 * Reset code table.
 */
static void
cl_hash(sp)
	LZWState *sp;
{
	register long *htab_p = sp->enc_htab+HSIZE;
	register long i, m1 = -1;

	i = HSIZE - 16;
 	do {
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
    	for (i += 16; i > 0; i--)
		*--htab_p = m1;
}

static
#ifdef NeXT_MOD
int
#endif
LZWCleanup(tif)
	TIFF *tif;
{
	if (tif->tif_data) {
		free(tif->tif_data);
		tif->tif_data = NULL;
	}
#ifdef NeXT_MOD
	return 1;
#endif
}

/*
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* End of original tif_lzw.c */

/* Start of original tif_dumpmode.c */

/*
 * TIFF Library.
 *
 * "Null" Compression Algorithm Support.
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#include <stdio.h>
#include <assert.h>
#endif

#if USE_PROTOTYPES
static	int DumpModeEncode(TIFF *, u_char *, int);
static	int DumpModeDecode(TIFF *, u_char *, int);
static	int DumpModeSeek(TIFF *, int);
#else
static	int DumpModeEncode(), DumpModeDecode(), DumpModeSeek();
#endif

/*
 * Initialize dump mode.
 */
#ifdef NeXT_MOD
static int
#endif
TIFFInitDumpMode(tif)
	register TIFF *tif;
{
	tif->tif_decoderow = DumpModeDecode;
	tif->tif_encoderow = DumpModeEncode;
	tif->tif_seek = DumpModeSeek;
	return (1);
}

/*
 * Encode a scanline of pixels.
 */
static int
DumpModeEncode(tif, pp, cc)
	register TIFF *tif;
	u_char *pp;
	int cc;
{
	if (tif->tif_rawcc + cc > tif->tif_rawdatasize)
#ifdef NeXT_MOD
		if (!_NXTIFFFlushData(tif))
#else
		if (!TIFFFlushData(tif))
#endif
			return (-1);
	bcopy(pp, tif->tif_rawcp, cc);
	if (tif->tif_flags & TIFF_SWAB) {
		switch (tif->tif_dir.td_bitspersample) {
		case 16:
			assert((cc & 3) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfShort((u_short *)tif->tif_rawcp, cc/2);
#else
			TIFFSwabArrayOfShort((u_short *)tif->tif_rawcp, cc/2);
#endif
			break;
		case 32:
			assert((cc & 15) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfLong((u_long *)tif->tif_rawcp, cc/4);
#else
			TIFFSwabArrayOfLong((u_long *)tif->tif_rawcp, cc/4);
#endif
			break;
		}
	}
	tif->tif_rawcp += cc;
	tif->tif_rawcc += cc;
	return (1);
}

/*
 * Decode a scanline of pixels.
 */
static int
DumpModeDecode(tif, buf, cc)
	register TIFF *tif;
	u_char *buf;
	int cc;
{
	if (tif->tif_rawcc < cc) {
		TIFFError(tif->tif_name,
		    "DumpModeDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	bcopy(tif->tif_rawcp, buf, cc);
	if (tif->tif_flags & TIFF_SWAB) {
		switch (tif->tif_dir.td_bitspersample) {
		case 16:
			assert((cc & 3) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfShort((u_short *)buf, cc/2);
#else
			TIFFSwabArrayOfShort((u_short *)buf, cc/2);
#endif
			break;
		case 32:
			assert((cc & 15) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfLong((u_long *)buf, cc/4);
#else
			TIFFSwabArrayOfLong((u_long *)buf, cc/4);
#endif
			break;
		}
	}
	tif->tif_rawcp += cc;
	tif->tif_rawcc -= cc;
	return (1);
}

/*
 * Seek forwards nrows in the current strip.
 */
static int
DumpModeSeek(tif, nrows)
	register TIFF *tif;
	int nrows;
{
	tif->tif_rawcp += nrows * tif->tif_scanlinesize;
	tif->tif_rawcc -= nrows * tif->tif_scanlinesize;
	return (1);
}

/* End of original tif_dumpmode.c */

/* Start of original tif_next.c */

/*
 * TIFF Library.
 *
 * NeXT 2-bit Grey Scale Compression Algorithm Support
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#endif

#if USE_PROTOTYPES
static	int NeXTEncode(TIFF *, u_char *, int);
static	int NeXTDecode(TIFF *, u_char *, int);
#else
static	int NeXTEncode(), NeXTDecode();
#endif

#ifdef NeXT
static int
#endif
TIFFInitNeXT(tif)
	TIFF *tif;
{
	tif->tif_decoderow = NeXTDecode;
	tif->tif_encoderow = NeXTEncode;
	return (1);
}

static int
NeXTEncode(tif, pp, cc)
	TIFF *tif;
	u_char *pp;
	int cc;
{
	TIFFError(tif->tif_name, "NeXT encoding is not implemented");
	return (-1);
}

#define SETPIXEL(op, v) {			\
	switch (npixels++ & 3) {		\
	case 0:	op[0]  = (v) << 6; break;	\
	case 1:	op[0] |= (v) << 4; break;	\
	case 2:	op[0] |= (v) << 2; break;	\
	case 3:	*op++ |= (v);	   break;	\
	}					\
}

#define LITERALROW	0x00
#define LITERALSPAN	0x40
#ifndef NeXT_MOD
#define WHITE   	((1<<2)-1)
#endif

static int
NeXTDecode(tif, buf, occ)
	TIFF *tif;
	u_char *buf;
	int occ;
{
	register u_char *bp, *op;
	register int cc, n;
	u_char *row;
	int scanline;

	/*
	 * Each scanline is assumed to start off as all
	 * white (we assume a PhotometricInterpretation
	 * of ``min-is-black'').
	 */
	for (op = buf, cc = occ; cc-- > 0;)
		*op++ = 0xff;

	bp = (u_char *)tif->tif_rawcp;
	cc = tif->tif_rawcc;
	scanline = tif->tif_scanlinesize;
	for (row = buf; occ > 0; occ -= scanline, row += scanline) {
		n = *bp++, cc--;
		switch (n) {
		case LITERALROW:
			/*
			 * The entire scanline is given as literal values.
			 */
			if (cc < scanline)
				goto bad;
			bcopy(bp, row, scanline);
			bp += scanline;
			cc -= scanline;
			break;
		case LITERALSPAN: {
			int off;
			/*
			 * The scanline has a literal span
			 * that begins at some offset.
			 */
			off = (bp[0] * 256) + bp[1];
			n = (bp[2] * 256) + bp[3];
			if (cc < 4+n)
				goto bad;
			bcopy(bp+4, row+off, n);
			bp += 4+n;
			cc -= 4+n;
			break;
		}
		default: {
			register int npixels = 0, grey;
			int imagewidth = tif->tif_dir.td_imagewidth;

			/*
			 * The scanline is composed of a sequence
			 * of constant color ``runs''.  We shift
			 * into ``run mode'' and interpret bytes
			 * as codes of the form <color><npixels>
			 * until we've filled the scanline.
			 */
			op = row;
			for (;;) {
				grey = (n>>6) & 0x3;
				n &= 0x3f;
				while (n-- > 0)
					SETPIXEL(op, grey);
				if (npixels >= imagewidth)
					break;
				if (cc == 0)
					goto bad;
				n = *bp++, cc--;
			}
			break;
		}
		}
	}
	tif->tif_rawcp = (char *)bp;
	tif->tif_rawcc = cc;
	return (1);
bad:
	TIFFError(tif->tif_name, "NeXTDecode: Not enough data for scanline %d",
	    tif->tif_row);
	return (0);
}

/* End of original tif_next.c */

/* Start of original tif_packbits.c */

/*
 * TIFF Library.
 *
 * PackBits Compression Algorithm Support
 */
#ifndef NeXT_MOD
#include "machdep.h"
#include "tiffio.h"
#endif

#if USE_PROTOTYPES
static	int PackBitsEncode(TIFF *, u_char *, int);
static	int PackBitsDecode(TIFF *, u_char *, int);
#else
static	int PackBitsEncode(), PackBitsDecode();
#endif

#ifdef NeXT_MOD
static int
#endif
TIFFInitPackBits(tif)
	TIFF *tif;
{
	tif->tif_decoderow = PackBitsDecode;
	tif->tif_encoderow = PackBitsEncode;
	return (1);
}

/*
 * Encode a scanline of pixels.
 */
static int
PackBitsEncode(tif, bp, cc)
	TIFF *tif;
	u_char *bp;
	register int cc;
{
	register char *op, *lastliteral;
	register int n, b;
	enum { BASE, LITERAL, RUN, LITERAL_RUN } state;
	char *ep;
	int slop;

	op = tif->tif_rawcp;
	ep = tif->tif_rawdata + tif->tif_rawdatasize;
	state = BASE;
	lastliteral = 0;
	while (cc > 0) {
		/*
		 * Find the longest string of identical bytes.
		 */
		b = *bp++, cc--, n = 1;
		for (; cc > 0 && b == *bp; cc--, bp++)
			n++;
	again:
		if (op + 2 >= ep) {		/* insure space for new data */
			/*
			 * Be careful about writing the last
			 * literal.  Must write up to that point
			 * and then copy the remainder to the
			 * front of the buffer.
			 */
			if (state == LITERAL || state == LITERAL_RUN) {
				slop = op - lastliteral;
				tif->tif_rawcc += lastliteral - tif->tif_rawcp;
#ifdef NeXT_MOD
				if (!_NXTIFFFlushData(tif))
#else
				if (!TIFFFlushData(tif))
#endif
					return (-1);
				op = tif->tif_rawcp;
				for (; slop-- > 0; *op++ = *lastliteral++)
					;
				lastliteral = tif->tif_rawcp;
			} else {
				tif->tif_rawcc += op - tif->tif_rawcp;
#ifdef NeXT_MOD
				if (!_NXTIFFFlushData(tif))
#else
				if (!TIFFFlushData(tif))
#endif
					return (-1);
				op = tif->tif_rawcp;
			}
		}
		switch (state) {
		case BASE:		/* initial state, set run/literal */
			if (n > 1) {
				state = RUN;
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);
				*op++ = b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = b;
				state = LITERAL;
			}
			break;
		case LITERAL:		/* last object was literal string */
			if (n > 1) {
				state = LITERAL_RUN;
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);		/* encode run */
				*op++ = b;
			} else {			/* extend literal */
				if (++(*lastliteral) == 127)
					state = BASE;
				*op++ = b;
			}
			break;
		case RUN:		/* last object was run */
			if (n > 1) {
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);
				*op++ = b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = b;
				state = LITERAL;
			}
			break;
		case LITERAL_RUN:	/* literal followed by a run */
			/*
			 * Check to see if previous run should
			 * be converted to a literal, in which
			 * case we convert literal-run-literal
			 * to a single literal.
			 */
			if (n == 1 && op[-2] == (char)-1 &&
			    *lastliteral < 126) {
				state = (((*lastliteral) += 2) == 127 ?
				    BASE : LITERAL);
				op[-2] = op[-1];	/* replicate */
			} else
				state = RUN;
			goto again;
		}
	}
	tif->tif_rawcc += op - tif->tif_rawcp;
	tif->tif_rawcp = op;
	return (1);
}

static int
PackBitsDecode(tif, op, occ)
	TIFF *tif;
	register u_char *op;
	register int occ;
{
	register char *bp;
	register int n, b;
	register int cc;

	bp = tif->tif_rawcp; cc = tif->tif_rawcc;
	while (cc > 0 && occ > 0) {
		SIGNEXTEND(*bp++, n);
		if (n == 128) {
			cc--;
			continue;
		}
		if (n < 0) {		/* replicate next byte -n+1 times */
			n = -n + 1;
			cc--;
			occ -= n;
			for (b = *bp++; n-- > 0;)
				*op++ = b;
		} else {		/* copy next n+1 bytes literally */
			bcopy(bp, op, ++n);
			op += n; occ -= n;
			bp += n; cc -= n;
		}
	}
	tif->tif_rawcp = bp;
	tif->tif_rawcc = cc;
	if (occ > 0) {
		TIFFError(tif->tif_name,
		    "PackBitsDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	/* check for buffer overruns? */
	return (1);
}

/* End of original tif_packbits.c */

#ifdef FAX3_SUPPORT
/* Start of original tif_fax3.c */

/*
 * TIFF Library.
 *
 * CCITT Group 3 Facsimile-compatible
 * Compression Scheme Support.
 *
 * This stuff is derived from code by Paul Haeberli and
 * Jef Pozkanzer.
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#include <stdio.h>
#define	NDEBUG			/* performance tradeoff */
#include <assert.h>
#else
#define	NDEBUG			/* performance tradeoff */
#endif

/* 
 * 	fax.h
 *
 * 	Copyright (C) 1989 by Jef Poskanzer.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */
/*
 * The code+length are placed next to each other and
 * first in the structure so that comparisons can be
 * done 32-bits at a time (instead of as two 16-bit
 * compares) -- on machines where it is worthwhile.
 */
typedef struct tableentry {
    short code;
    short length;
    short count;
    short tabid;
} tableentry;

/*
 * Table identifiers are coded so that the decoding
 * algorithm can check the sign bit to decide if the
 * current code is "complete".
 */
#define TWTABLE		-2
#define TBTABLE		-1
#define VRTABLE		0
#define MWTABLE		1
#define MBTABLE		2
#define EXTABLE		3
#define	MINTABID	TWTABLE
#define	MAXTABID	EXTABLE

#ifdef NeXT_MOD
static const struct tableentry twtable[] = {
#else
static struct tableentry twtable[] = {
#endif
    { 0x35, 8, 0, TWTABLE },
    { 0x7, 6, 1, TWTABLE },
    { 0x7, 4, 2, TWTABLE },
    { 0x8, 4, 3, TWTABLE },
    { 0xb, 4, 4, TWTABLE },
    { 0xc, 4, 5, TWTABLE },
    { 0xe, 4, 6, TWTABLE },
    { 0xf, 4, 7, TWTABLE },
    { 0x13, 5, 8, TWTABLE },
    { 0x14, 5, 9, TWTABLE },
    { 0x7, 5, 10, TWTABLE },
    { 0x8, 5, 11, TWTABLE },
    { 0x8, 6, 12, TWTABLE },
    { 0x3, 6, 13, TWTABLE },
    { 0x34, 6, 14, TWTABLE },
    { 0x35, 6, 15, TWTABLE },
    { 0x2a, 6, 16, TWTABLE },
    { 0x2b, 6, 17, TWTABLE },
    { 0x27, 7, 18, TWTABLE },
    { 0xc, 7, 19, TWTABLE },
    { 0x8, 7, 20, TWTABLE },
    { 0x17, 7, 21, TWTABLE },
    { 0x3, 7, 22, TWTABLE },
    { 0x4, 7, 23, TWTABLE },
    { 0x28, 7, 24, TWTABLE },
    { 0x2b, 7, 25, TWTABLE },
    { 0x13, 7, 26, TWTABLE },
    { 0x24, 7, 27, TWTABLE },
    { 0x18, 7, 28, TWTABLE },
    { 0x2, 8, 29, TWTABLE },
    { 0x3, 8, 30, TWTABLE },
    { 0x1a, 8, 31, TWTABLE },
    { 0x1b, 8, 32, TWTABLE },
    { 0x12, 8, 33, TWTABLE },
    { 0x13, 8, 34, TWTABLE },
    { 0x14, 8, 35, TWTABLE },
    { 0x15, 8, 36, TWTABLE },
    { 0x16, 8, 37, TWTABLE },
    { 0x17, 8, 38, TWTABLE },
    { 0x28, 8, 39, TWTABLE },
    { 0x29, 8, 40, TWTABLE },
    { 0x2a, 8, 41, TWTABLE },
    { 0x2b, 8, 42, TWTABLE },
    { 0x2c, 8, 43, TWTABLE },
    { 0x2d, 8, 44, TWTABLE },
    { 0x4, 8, 45, TWTABLE },
    { 0x5, 8, 46, TWTABLE },
    { 0xa, 8, 47, TWTABLE },
    { 0xb, 8, 48, TWTABLE },
    { 0x52, 8, 49, TWTABLE },
    { 0x53, 8, 50, TWTABLE },
    { 0x54, 8, 51, TWTABLE },
    { 0x55, 8, 52, TWTABLE },
    { 0x24, 8, 53, TWTABLE },
    { 0x25, 8, 54, TWTABLE },
    { 0x58, 8, 55, TWTABLE },
    { 0x59, 8, 56, TWTABLE },
    { 0x5a, 8, 57, TWTABLE },
    { 0x5b, 8, 58, TWTABLE },
    { 0x4a, 8, 59, TWTABLE },
    { 0x4b, 8, 60, TWTABLE },
    { 0x32, 8, 61, TWTABLE },
    { 0x33, 8, 62, TWTABLE },
    { 0x34, 8, 63, TWTABLE },
};

#ifdef NeXT_MOD
static const struct tableentry mwtable[] = {
#else
static struct tableentry mwtable[] = {
#endif
    { 0x1b, 5, 64, MWTABLE },
    { 0x12, 5, 128, MWTABLE },
    { 0x17, 6, 192, MWTABLE },
    { 0x37, 7, 256, MWTABLE },
    { 0x36, 8, 320, MWTABLE },
    { 0x37, 8, 384, MWTABLE },
    { 0x64, 8, 448, MWTABLE },
    { 0x65, 8, 512, MWTABLE },
    { 0x68, 8, 576, MWTABLE },
    { 0x67, 8, 640, MWTABLE },
    { 0xcc, 9, 704, MWTABLE },
    { 0xcd, 9, 768, MWTABLE },
    { 0xd2, 9, 832, MWTABLE },
    { 0xd3, 9, 896, MWTABLE },
    { 0xd4, 9, 960, MWTABLE },
    { 0xd5, 9, 1024, MWTABLE },
    { 0xd6, 9, 1088, MWTABLE },
    { 0xd7, 9, 1152, MWTABLE },
    { 0xd8, 9, 1216, MWTABLE },
    { 0xd9, 9, 1280, MWTABLE },
    { 0xda, 9, 1344, MWTABLE },
    { 0xdb, 9, 1408, MWTABLE },
    { 0x98, 9, 1472, MWTABLE },
    { 0x99, 9, 1536, MWTABLE },
    { 0x9a, 9, 1600, MWTABLE },
    { 0x18, 6, 1664, MWTABLE },
    { 0x9b, 9, 1728, MWTABLE },
};

#ifdef NeXT_MOD
static const struct tableentry tbtable[] = {
#else
static struct tableentry tbtable[] = {
#endif
    { 0x37, 10, 0, TBTABLE },
    { 0x2, 3, 1, TBTABLE },
    { 0x3, 2, 2, TBTABLE },
    { 0x2, 2, 3, TBTABLE },
    { 0x3, 3, 4, TBTABLE },
    { 0x3, 4, 5, TBTABLE },
    { 0x2, 4, 6, TBTABLE },
    { 0x3, 5, 7, TBTABLE },
    { 0x5, 6, 8, TBTABLE },
    { 0x4, 6, 9, TBTABLE },
    { 0x4, 7, 10, TBTABLE },
    { 0x5, 7, 11, TBTABLE },
    { 0x7, 7, 12, TBTABLE },
    { 0x4, 8, 13, TBTABLE },
    { 0x7, 8, 14, TBTABLE },
    { 0x18, 9, 15, TBTABLE },
    { 0x17, 10, 16, TBTABLE },
    { 0x18, 10, 17, TBTABLE },
    { 0x8, 10, 18, TBTABLE },
    { 0x67, 11, 19, TBTABLE },
    { 0x68, 11, 20, TBTABLE },
    { 0x6c, 11, 21, TBTABLE },
    { 0x37, 11, 22, TBTABLE },
    { 0x28, 11, 23, TBTABLE },
    { 0x17, 11, 24, TBTABLE },
    { 0x18, 11, 25, TBTABLE },
    { 0xca, 12, 26, TBTABLE },
    { 0xcb, 12, 27, TBTABLE },
    { 0xcc, 12, 28, TBTABLE },
    { 0xcd, 12, 29, TBTABLE },
    { 0x68, 12, 30, TBTABLE },
    { 0x69, 12, 31, TBTABLE },
    { 0x6a, 12, 32, TBTABLE },
    { 0x6b, 12, 33, TBTABLE },
    { 0xd2, 12, 34, TBTABLE },
    { 0xd3, 12, 35, TBTABLE },
    { 0xd4, 12, 36, TBTABLE },
    { 0xd5, 12, 37, TBTABLE },
    { 0xd6, 12, 38, TBTABLE },
    { 0xd7, 12, 39, TBTABLE },
    { 0x6c, 12, 40, TBTABLE },
    { 0x6d, 12, 41, TBTABLE },
    { 0xda, 12, 42, TBTABLE },
    { 0xdb, 12, 43, TBTABLE },
    { 0x54, 12, 44, TBTABLE },
    { 0x55, 12, 45, TBTABLE },
    { 0x56, 12, 46, TBTABLE },
    { 0x57, 12, 47, TBTABLE },
    { 0x64, 12, 48, TBTABLE },
    { 0x65, 12, 49, TBTABLE },
    { 0x52, 12, 50, TBTABLE },
    { 0x53, 12, 51, TBTABLE },
    { 0x24, 12, 52, TBTABLE },
    { 0x37, 12, 53, TBTABLE },
    { 0x38, 12, 54, TBTABLE },
    { 0x27, 12, 55, TBTABLE },
    { 0x28, 12, 56, TBTABLE },
    { 0x58, 12, 57, TBTABLE },
    { 0x59, 12, 58, TBTABLE },
    { 0x2b, 12, 59, TBTABLE },
    { 0x2c, 12, 60, TBTABLE },
    { 0x5a, 12, 61, TBTABLE },
    { 0x66, 12, 62, TBTABLE },
    { 0x67, 12, 63, TBTABLE },
};

#ifdef NeXT_MOD
static const struct tableentry mbtable[] = {
#else
static struct tableentry mbtable[] = {
#endif
    { 0xf, 10, 64, MBTABLE },
    { 0xc8, 12, 128, MBTABLE },
    { 0xc9, 12, 192, MBTABLE },
    { 0x5b, 12, 256, MBTABLE },
    { 0x33, 12, 320, MBTABLE },
    { 0x34, 12, 384, MBTABLE },
    { 0x35, 12, 448, MBTABLE },
    { 0x6c, 13, 512, MBTABLE },
    { 0x6d, 13, 576, MBTABLE },
    { 0x4a, 13, 640, MBTABLE },
    { 0x4b, 13, 704, MBTABLE },
    { 0x4c, 13, 768, MBTABLE },
    { 0x4d, 13, 832, MBTABLE },
    { 0x72, 13, 896, MBTABLE },
    { 0x73, 13, 960, MBTABLE },
    { 0x74, 13, 1024, MBTABLE },
    { 0x75, 13, 1088, MBTABLE },
    { 0x76, 13, 1152, MBTABLE },
    { 0x77, 13, 1216, MBTABLE },
    { 0x52, 13, 1280, MBTABLE },
    { 0x53, 13, 1344, MBTABLE },
    { 0x54, 13, 1408, MBTABLE },
    { 0x55, 13, 1472, MBTABLE },
    { 0x5a, 13, 1536, MBTABLE },
    { 0x5b, 13, 1600, MBTABLE },
    { 0x64, 13, 1664, MBTABLE },
    { 0x65, 13, 1728, MBTABLE },
};

#ifdef NeXT_MOD
static const struct tableentry extable[] = {
#else
static struct tableentry extable[] = {
#endif
    { 0x8, 11, 1792, EXTABLE },
    { 0xc, 11, 1856, EXTABLE },
    { 0xd, 11, 1920, EXTABLE },
    { 0x12, 12, 1984, EXTABLE },
    { 0x13, 12, 2048, EXTABLE },
    { 0x14, 12, 2112, EXTABLE },
    { 0x15, 12, 2176, EXTABLE },
    { 0x16, 12, 2240, EXTABLE },
    { 0x17, 12, 2304, EXTABLE },
    { 0x1c, 12, 2368, EXTABLE },
    { 0x1d, 12, 2432, EXTABLE },
    { 0x1e, 12, 2496, EXTABLE },
    { 0x1f, 12, 2560, EXTABLE },
};

#ifdef notdef		/* not doing 2D yet...*/
#define HORIZ	1000
#define PASS	2000

#ifdef NeXT_MOD
static const struct tableentry vrtable[] = {
#else
static struct tableentry vrtable[] = {
#endif
    { 0x3, 7, -3, VRTABLE },
    { 0x3, 6, -2, VRTABLE },
    { 0x3, 3, -1, VRTABLE },
    { 0x1, 1,  0, VRTABLE },
    { 0x2, 3,  1, VRTABLE },
    { 0x2, 6,  2, VRTABLE },
    { 0x2, 7,  3, VRTABLE },
    { 0x1, 3,  HORIZ, VRTABLE },
    { 0x1, 4,  PASS, VRTABLE },
};
#endif

#define WHASHA	3510
#define WHASHB	1178

#define BHASHA	293
#define BHASHB	2695

#define	FAX3_CLASSF	TIFF_OPT0	/* use Class F protocol */

typedef struct {
	int	data;
	int	bit;
#ifdef NeXT_MOD
	const u_char*	bitmap;
#define HASHSIZE	1021
	const tableentry *whash[HASHSIZE];
	const tableentry *bhash[HASHSIZE];
#else
	u_char*	bitmap;
#define HASHSIZE	1021
	tableentry *whash[HASHSIZE];
	tableentry *bhash[HASHSIZE];
#endif
} Fax3DecodeState;

typedef struct {
	int	data;
	int	bit;
#ifdef NeXT_MOD
	const u_char*	bitmap;
	const u_char	*wruns;
	const u_char	*bruns;
#else
	u_char*	bitmap;
	u_char	*wruns;
	u_char	*bruns;
#endif
} Fax3EncodeState;

#define	EOL	0x1			/* 11 bits of zero, followed by 1 */

#if USE_PROTOTYPES
static	Fax3PreDecode(TIFF *);
static	Fax3Decode(TIFF*, char *, int);
static	int Fax3DecodeRow(TIFF*, u_char *, int);
static	Fax3PreEncode(TIFF *);
static	Fax3PostEncode(TIFF *);
static	Fax3Encode(TIFF*, u_char *, int);
static	Fax3Close(TIFF *);
static	Fax3Cleanup(TIFF *);
#else
static	int Fax3PreEncode(), Fax3Encode(), Fax3PostEncode();
static	int Fax3Decode(), Fax3PreDecode(), Fax3DecodeRow();
static	int Fax3Close(), Fax3Cleanup();
#endif

#ifdef NeXT
static int
#endif
TIFFInitCCITTFax3(tif)
	TIFF *tif;
{
	tif->tif_stripdecode = Fax3PreDecode;
	tif->tif_decoderow = Fax3Decode;
	tif->tif_stripencode = Fax3PreEncode;
	tif->tif_encoderow = Fax3Encode;
	tif->tif_encodestrip = Fax3PostEncode;
	tif->tif_close = Fax3Close;
	tif->tif_cleanup = Fax3Cleanup;
	tif->tif_options |= FAX3_CLASSF;	/* default */
	tif->tif_flags |= TIFF_NOBITREV;	/* we handle bit reversal */
	return (1);
}

#ifndef NeXT_MOD
#ifdef NeXT_MOD
static void
#endif
TIFFModeCCITTFax3(tif, isClassF)
	TIFF *tif;
	int isClassF;
{
	if (isClassF)
		tif->tif_options |= FAX3_CLASSF;
	else
		tif->tif_options &= ~FAX3_CLASSF;
}
#endif

static int
addtohash(hash, te, n, a, b)
	tableentry *hash[];
	tableentry *te;
	int n, a, b;
{
	while (n--) {
		u_int pos = ((te->length + a) * (te->code + b)) % HASHSIZE;
		if (hash[pos] != 0) {
			TIFFError("Fax3(addtohash)", "Fatal hash collision");
			return (0);
		}
		hash[pos] = te;
		te++;
	}
	return (1);
}

static void
skiptoeol(tif)
	TIFF *tif;
{
	Fax3DecodeState *sp = (Fax3DecodeState *)tif->tif_data;
	register int bit = sp->bit;
	register int data = sp->data;
	int code, len;

	do {
		code = len = 0;
		do {
			if (bit == 0) {
				if (tif->tif_rawcc <= 0) {
					TIFFError(tif->tif_name,
				    "skiptoeol: Premature EOF at scanline %d",
					    tif->tif_row);
					return;
				}
				bit = 0x80;
				data = sp->bitmap[*(u_char *)tif->tif_rawcp++];
			}
			code <<= 1;
			if (data & bit)
				code |= 1;
			bit >>= 1;
			len++;
		} while (code <= 0);
	} while (len < 12 || code != EOL);
	sp->bit = bit;
	sp->data = data;
}

/*
 * Setup state for decoding a strip.
 */
static
#ifdef NeXT_MOD
int
#endif
Fax3PreDecode(tif)
	TIFF *tif;
{
	register Fax3DecodeState *sp = (Fax3DecodeState *)tif->tif_data;
	TIFFDirectory *td = &tif->tif_dir;

	if (td->td_group3options & GROUP3OPT_2DENCODING) {
		TIFFError(tif->tif_name, "2-D encoding is not supported");
		return (0);
	}
	if (td->td_bitspersample != 1) {
		TIFFError(tif->tif_name,
		    "Bits/sample must be 1 for Group 3 encoding");
		return (0);
	}
	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (Fax3DecodeState));
		if (tif->tif_data == NULL) {
			TIFFError(tif->tif_name,
			    "No space for Fax3 state block");
			return (0);
		}
		sp = (Fax3DecodeState *)tif->tif_data;
		bzero(sp->whash, HASHSIZE*sizeof (tableentry *));
#define	TABSIZE(tab)	(sizeof (tab) / sizeof (tab[0]))
#define	WHITE(tab)	addtohash(sp->whash, tab, TABSIZE(tab), WHASHA, WHASHB)
#define	BLACK(tab)	addtohash(sp->bhash, tab, TABSIZE(tab), BHASHA, BHASHB)
		if (!WHITE(twtable) || !WHITE(mwtable) || !WHITE(extable))
			return (0);
		bzero(sp->bhash, HASHSIZE*sizeof (tableentry *));
		if (!BLACK(tbtable) || !BLACK(mbtable) || !BLACK(extable))
			return (0);
#undef BLACK
#undef WHITE
#undef TABSIZE
#ifdef NeXT_MOD
		sp->bitmap = _NXTIFFBitTable 
				(tif->tif_fillorder!=td->td_fillorder);
#else
		sp->bitmap = (tif->tif_fillorder != td->td_fillorder ? 
		    TIFFBitRevTable : TIFFNoBitRevTable);
#endif
	}
	sp->bit = 0;				/* force initial read */
	sp->data = 0;
	if ((tif->tif_options & FAX3_CLASSF) == 0 && tif->tif_curstrip == 0)
		skiptoeol(tif);			/* skip leading EOL */
	return (1);
}

/*
 * Decode the requested amount of data.
 */
static
#ifdef NeXT_MOD
int
#endif
Fax3Decode(tif, buf, occ)
	TIFF *tif;
	char *buf;
	int occ;
{
	int scanline;
	u_char *row;
	int cc, imagewidth;

	bzero(buf, occ);		/* decoding only sets non-zero bits */
	scanline = tif->tif_scanlinesize;
	imagewidth = tif->tif_dir.td_imagewidth;
	row = (u_char *)buf;
	for (cc = occ; cc > 0; cc -= scanline) {
		if (!Fax3DecodeRow(tif, row, imagewidth))
			return (0);
		row += scanline;
	}
	return (1);
}

/*
 * Fill a span with ones.
 */
static void
fillspan(cp, x, count)
	register char *cp;
	register int x, count;
{
#ifdef NeXT_MOD
	static const unsigned char masks[] =
#else
	static unsigned char masks[] =
#endif
	    { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

	cp += x>>3;
	if (x &= 7) {			/* align to byte boundary */
		if (count < 8 - x) {
			*cp++ |= masks[count] >> x;
			return;
		}
		*cp++ |= 0xff >> x;
		count -= 8 - x;
	}
	while (count >= 8) {
		*cp++ = 0xff;
		count -= 8;
	}
	*cp |= masks[count];
}

#define	BITCASE(b)			\
    case b:				\
	code <<= 1;			\
	if (data & b) code |= 1;	\
	len++;				\
	if (code > 0) { bit = (b>>1); break; }

static int
Fax3DecodeRow(tif, buf, npels)
	TIFF *tif;
	u_char *buf;
	int npels;
{
	Fax3DecodeState *sp = (Fax3DecodeState *)tif->tif_data;
	short bit = sp->bit;
	short data = sp->data;
	short x = 0;
	short len = 0;
	short code = 0;
	short count = 0;
	int color = 1;
	int fillcolor = (tif->tif_dir.td_photometric == PHOTOMETRIC_MINISWHITE);
#ifdef NeXT_MOD
	const
#endif
	tableentry *te;
	u_int h;
#ifdef NeXT_MOD
	char *module = NULL;
#else
	static char module[] = "Fax3Decode";
#endif

	for (;;) {
		switch (bit) {
	again:	BITCASE(0x80);
		BITCASE(0x40);
		BITCASE(0x20);
		BITCASE(0x10);
		BITCASE(0x08);
		BITCASE(0x04);
		BITCASE(0x02);
		BITCASE(0x01);
		default:
			if (tif->tif_rawcc <= 0) {
			    TIFFError(module,
				"%s: Premature EOF at scanline %d (x %d)",
			        tif->tif_name, tif->tif_row, x);
				return (0);
			}
			data = sp->bitmap[*(u_char *)tif->tif_rawcp++];
			goto again;
		}
		if (len >= 12) {
			if (code == EOL) {
				if (x == 0) {
					TIFFWarning(tif->tif_name,
				    "%s: Ignoring null row at scanline %d",
					    module, tif->tif_row);
					code = len = 0;
					continue;
				}
				sp->bit = bit;
				sp->data = data;
				TIFFWarning(tif->tif_name,
				    "%s: Premature EOL at scanline %d (x %d)",
				    module, tif->tif_row, x);
				return (1);	/* try to resynchronize... */
			}
			if (len > 13) {
				TIFFError(tif->tif_name,
		    "%s: Bad code word (len %d code %d) at scanline %d (x %d)",
				    module, len, code, tif->tif_row, x);
				break;
			}
		}
		if (color) {
			if (len < 4)
				continue;
			h = ((len + WHASHA) * (code + WHASHB)) % HASHSIZE;
			assert(0 <= h && h < HASHSIZE);
			te = sp->whash[h];
		} else {
			if (len < 2)
				continue;
			h = ((len + BHASHA) * (code + BHASHB)) % HASHSIZE;
			assert(0 <= h && h < HASHSIZE);
			te = sp->bhash[h];
		}
		if (te && te->length == len && te->code == code) {
			assert(MINTABID <= te->tabid && te->tabid <= MAXTABID);
			if (te->tabid < 0) {
				count += te->count;
				if (x+count > npels)
					count = npels-x;
				if (count > 0) {
					if (color ^ fillcolor)
						fillspan(buf, x, count);
					x += count;
					if (x >= npels)
						break;
				}
				count = 0;
				color = !color;
			} else
				count += te->count;
			code = len = 0;
		}
	}
	sp->data = data;
	sp->bit = bit;
	skiptoeol(tif);
	return (x == npels);
}

/*
 * CCITT Group 3 FAX Encoding.
 */

static void
putbit(tif, d)
	TIFF* tif;
	int d;
{
	Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;

	if (d) 
		sp->data |= sp->bit;
	sp->bit >>= 1;
	if (sp->bit == 0) {
		if (tif->tif_rawcc >= tif->tif_rawdatasize)
			(void) TIFFFlushData1(tif);	/* XXX */
		*tif->tif_rawcp++ = sp->bitmap[sp->data];
		tif->tif_rawcc++;
		sp->data = 0;
		sp->bit = 0x80;
	}
}

static void
putcode(tif, te)
	TIFF *tif;
	tableentry *te;
{
	Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;
	int bit = sp->bit;
	int data = sp->data;
	u_int mask;

	for (mask = 1 << (te->length-1); mask; mask >>= 1) {
		if (te->code & mask)
			data |= bit;
		bit >>= 1;
		if (bit == 0) {
			if (tif->tif_rawcc >= tif->tif_rawdatasize)
				(void) TIFFFlushData1(tif);	/* XXX */
			*tif->tif_rawcp++ = sp->bitmap[data];
			tif->tif_rawcc++;
			data = 0;
			bit = 0x80;
		}
	}
	sp->data = data;
	sp->bit = bit;
}

static void
puteol(tif)
	TIFF *tif;
{
	int i;

	if (tif->tif_dir.td_group3options & GROUP3OPT_FILLBITS) {
		Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;
		/*
		 * Force bit alignment so EOL will terminate
		 * on a byte boundary.  That is, force the bit
		 * offset to 16-12 = 4, or bit pattern 0x08
		 * before putting out the EOL code.
		 */
		while (sp->bit != 0x08)
			putbit(tif, 0);
	}
	for (i = 0; i < 11; i++)
		putbit(tif, 0);
	putbit(tif, 1);
}

#ifdef NeXT_MOD
static const u_char zeroruns[256] = {
#else
static u_char zeroruns[256] = {
#endif
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,	/* 0x00 - 0x0f */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 0x10 - 0x1f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0x20 - 0x2f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0x30 - 0x3f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x40 - 0x4f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x50 - 0x5f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x60 - 0x6f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x70 - 0x7f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x80 - 0x8f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x90 - 0x9f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xa0 - 0xaf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xb0 - 0xbf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xc0 - 0xcf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xd0 - 0xdf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xe0 - 0xef */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xf0 - 0xff */
};
#ifdef NeXT_MOD
static const u_char oneruns[256] = {
#else
static u_char oneruns[256] = {
#endif
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x00 - 0x0f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x10 - 0x1f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x20 - 0x2f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x30 - 0x3f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x40 - 0x4f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x50 - 0x5f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x60 - 0x6f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x70 - 0x7f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x80 - 0x8f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x90 - 0x9f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0xa0 - 0xaf */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0xb0 - 0xbf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0xc0 - 0xcf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0xd0 - 0xdf */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 0xe0 - 0xef */
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,	/* 0xf0 - 0xff */
};

/*
 * Reset encoding state at the start of a strip.
 */
static
#ifdef NeXT_MOD
int
#endif
Fax3PreEncode(tif)
	TIFF *tif;
{
	register Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;
	TIFFDirectory *td = &tif->tif_dir;

	if (td->td_group3options & GROUP3OPT_2DENCODING) {
		TIFFError(tif->tif_name,
		    "Sorry, can not handle 2-d FAX encoding");
		return (0);
	}
	if (td->td_bitspersample != 1) {
		TIFFError("Fax3PreEncode",
		    "%s: Must have 1 bit/sample", tif->tif_name);
		return (0);
	}
	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (Fax3EncodeState));
		if (tif->tif_data == NULL) {
			TIFFError(tif->tif_name,
			    "No space for Fax3 state block");
			return (0);
		}
		sp = (Fax3EncodeState *)tif->tif_data;
		if (tif->tif_dir.td_photometric == PHOTOMETRIC_MINISWHITE) {
			sp->wruns = zeroruns;
			sp->bruns = oneruns;
		} else {
			sp->wruns = oneruns;
			sp->bruns = zeroruns;
		}
#ifdef NeXT_MOD
		sp->bitmap = _NXTIFFBitTable 
				(tif->tif_fillorder!=td->td_fillorder);
#else
		sp->bitmap = (tif->tif_fillorder != td->td_fillorder ? 
		    TIFFBitRevTable : TIFFNoBitRevTable);
#endif
	}
	sp->bit = 0x80;
	sp->data = 0;
	if ((tif->tif_options & FAX3_CLASSF) == 0 && tif->tif_curstrip == 0)
		puteol(tif);
	return (1);
}

static int
findspan(bpp, bits, tab)
	u_char **bpp;
	register int bits;
	register u_char *tab;
{
	register u_char *bp = *bpp;
	register int span;
	int n;

	/*
	 * To find a run of ones or zeros we use tables
	 * that give the number of consecutive ones or
	 * zeros starting from the msb.  The tables are
	 * indexed by byte value so a starting condition
	 * is to first align the search to a byte boundary.
	 */
	if (n = (bits & 7)) {
		/*
		 * NB: bits is counted *down* so we have to
		 *     account for this in calculating the
		 *     bit offset in the byte.
		 */
		span = tab[(*bp++ << (8-n)) & 0xff];
		if (span < n)		/* doesn't extend to edge of byte */
			return (span);
		span = n;		/* table value may be too generous */
		bits -= span;
	} else
		span = 0;
	while (bits >= 8) {
		int len = tab[*bp];
		span += len;
		bits -= len;
		if (len < 8)
			break;
		bp++;
	}
	*bpp = bp;
	return (span);
}

/*
 * Encode a buffer of pixels.
 */
static
#ifdef NeXT_MOD
int
#endif
Fax3Encode(tif, bp, cc)
	TIFF *tif;
	u_char *bp;
	int cc;
{
	Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;
	int span, bits = cc << 3;		/* XXX */
#ifdef NeXT_MOD
	const
#endif
	tableentry *te;

	while (bits > 0) {
		span = findspan(&bp, bits, sp->wruns);	/* white span */
		bits -= span;
		if (span >= 64) {
			te = &mwtable[(span/64)-1];
			putcode(tif, te);
			span -= te->count;
		}
		putcode(tif, &twtable[span]);
		if (bits == 0)
			break;
		span = findspan(&bp, bits, sp->bruns);	/* black span */
		bits -= span;
		if (span >= 64) {
			te = &mbtable[(span/64)-1];
			putcode(tif, te);
			span -= te->count;
		}
		putcode(tif, &tbtable[span]);
	}
	puteol(tif);
	return (1);
}

static int
Fax3PostEncode(tif)
	TIFF *tif;
{
	Fax3EncodeState *sp = (Fax3EncodeState *)tif->tif_data;

	if (sp->bit != 0x80) {
		if (tif->tif_rawcc >= tif->tif_rawdatasize &&
		    !TIFFFlushData1(tif))
			return (0);
		*tif->tif_rawcp++ = sp->bitmap[sp->data];
		tif->tif_rawcc++;
	}
	return (1);
}

static
#ifdef NeXT_MOD
int
#endif
Fax3Close(tif)
	TIFF *tif;
{
	if ((tif->tif_options & FAX3_CLASSF) == 0) {	/* append RTC */
		int i;
		for (i = 0; i < 6; i++)
			puteol(tif);
		(void) Fax3PostEncode(tif);
	}
#ifdef NeXT_MOD
	return 1;
#endif
}

static
#ifdef NeXT_MOD
int
#endif
Fax3Cleanup(tif)
	TIFF *tif;
{
	if (tif->tif_data) {
		free(tif->tif_data);
		tif->tif_data = NULL;
	}
#ifdef NeXT_MOD
	return 1;
#endif
}

/* End of original tif_fax3.c */
#endif

#ifdef NeXT_MOD
#ifdef JPEG_SUPPORT

/*
 * Our TIFF file support will be pretty kludgy for now; instead of going through the 
 * nice, scanline-based compression layer, we'll detect JPEG in _NXTIFFRead() and branch
 * to the JPEG stuff there.
 */
static int TIFFInitJPEG (register TIFF *tif)
{
	tif->tif_dir.td_usetiles = 1;
	return (1);
}

#if 0
static	int JPEGModeEncode(TIFF *, u_char *, int);
static	int JPEGModeDecode(TIFF *, u_char *, int);
static	int JPEGModeSeek(TIFF *, int);

/*
 * Encode a scanline of pixels.
 */
static int
JPEGModeEncode(tif, pp, cc)
	register TIFF *tif;
	u_char *pp;
	int cc;
{
	if (tif->tif_rawcc + cc > tif->tif_rawdatasize)
#ifdef NeXT_MOD
		if (!_NXTIFFFlushData(tif))
#else
		if (!TIFFFlushData(tif))
#endif
			return (-1);
	bcopy(pp, tif->tif_rawcp, cc);
	if (tif->tif_flags & TIFF_SWAB) {
		switch (tif->tif_dir.td_bitspersample) {
		case 16:
			assert((cc & 3) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfShort((u_short *)tif->tif_rawcp, cc/2);
#else
			TIFFSwabArrayOfShort((u_short *)tif->tif_rawcp, cc/2);
#endif
			break;
		case 32:
			assert((cc & 15) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfLong((u_long *)tif->tif_rawcp, cc/4);
#else
			TIFFSwabArrayOfLong((u_long *)tif->tif_rawcp, cc/4);
#endif
			break;
		}
	}
	tif->tif_rawcp += cc;
	tif->tif_rawcc += cc;
	return (1);
}

/*
 * Decode a scanline of pixels.
 */
static int
JPEGModeDecode(tif, buf, cc)
	register TIFF *tif;
	u_char *buf;
	int cc;
{
	if (tif->tif_rawcc < cc) {
		TIFFError(tif->tif_name,
		    "DumpModeDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	bcopy(tif->tif_rawcp, buf, cc);
	if (tif->tif_flags & TIFF_SWAB) {
		switch (tif->tif_dir.td_bitspersample) {
		case 16:
			assert((cc & 3) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfShort((u_short *)buf, cc/2);
#else
			TIFFSwabArrayOfShort((u_short *)buf, cc/2);
#endif
			break;
		case 32:
			assert((cc & 15) == 0);
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfLong((u_long *)buf, cc/4);
#else
			TIFFSwabArrayOfLong((u_long *)buf, cc/4);
#endif
			break;
		}
	}
	tif->tif_rawcp += cc;
	tif->tif_rawcc -= cc;
	return (1);
}

/*
 * Seek forwards nrows in the current strip.
 */
static int
JPEGModeSeek(tif, nrows)
	register TIFF *tif;
	int nrows;
{
	tif->tif_rawcp += nrows * tif->tif_scanlinesize;
	tif->tif_rawcc -= nrows * tif->tif_scanlinesize;
	return (1);
}
#endif /* to #if 0 up above */

#endif
#endif

/*
  
Modifications (since 77):

77
--
 2/19/90 aozer	Created from Sam Leffler's freely distributable library
 3/5/90 aozer	Explicitly declared the various "int" functions.

86
--
 6/9/90 aozer	Integrated changes for Sam Leffler's TIFF Library v2.2.

87
--
 6/15/90 aozer	Dropped Fax Group 3 support in, bounded by #ifdef FAX3_SUPPORT

*/
