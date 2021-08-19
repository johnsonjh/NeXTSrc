#include <ctype.h>
#include <sys/errno.h>
#include <stdio.h>

/*
 * Motorola 68881/2 packed decimal floating point format
 */
struct pd_fp {
	unsigned int
		pf_signman:1,
		pf_signexp:1,
		pf_infnan:2,
		pf_exp2:4,
		pf_exp1:4,
		pf_exp0:4,
		pf_exp3:4,
		pf_xxx0:4,
		pf_xxx1:4,
		pf_man16:4;
	unsigned int pf_man15_8;
	unsigned int pf_man7_0;
};

#define SKIPWHITE(s) while(*(s) == ' ' || *(s) == '\t') (s)++
#define MAXMANTISSA 17
#define NAN 0

char *
atof_m68k(str, type, words)
char *str;
char type;
unsigned short *words;
{
	struct pd_fp pd_fp;
	char mantissa[MAXMANTISSA];
	int i, left_of_decimal, exponent;
	char *p = str;
	int seen_non_zero;	/* To the left of decimal */
	unsigned int packmantissa();
	double pdfptodbl();
	extern int errno;

	/* Setup data structures */
	pd_fp.pf_signman = 0;
	pd_fp.pf_signexp = 0;
	pd_fp.pf_infnan = 0;
	pd_fp.pf_xxx0 = 0;
	pd_fp.pf_xxx1 = 0;
	exponent = 0;
	left_of_decimal = 0;
	seen_non_zero = 0;
	bzero(mantissa, sizeof(mantissa));

	SKIPWHITE(p);
	if (*p == '0') {
		switch(p[1]) {
		case 'f':
		case 'F':
		case 's':
		case 'S':
		case 'd':
		case 'D':
		case 'r':
		case 'R':
		case 'x':
		case 'X':
			p += 2;
			break;
		}
	}
	if (*p == '-') {
		p++;
		pd_fp.pf_signman = 1;
	} else if (*p == '+')
		p++;
	SKIPWHITE(p);
	/* 
	 * Handle floating point constants 0rInfinity, 0rNan
	 */
	if (isalpha(*p)) {
		union {
			double d;
			unsigned int x[2];
		} result;

		switch (*p) {
		case 'i':
		case 'I':
			result.x[0] = 0x7ff00000;
			result.x[1] = 0x00000000;
			break;
		case 'n':
		case 'N':
			result.x[0] = 0x7fffffff;
			result.x[1] = 0xffffffff;
			break;
		case 's':
		case 'S':
			result.x[0] = 0x7ff7ffff;
			result.x[1] = 0xffffffff;
			break;
		default:
			result.x[0] = 0x00000000;
			result.x[1] = 0x00000000;
		}
		if (pd_fp.pf_signman)
			result.x[0] |= 0x80000000;
/*		return(result.d); */
		switch(type) {
		case 'f':
		case 'F':
		case 's':
		case 'S':
		  *(float *)words = result.d;
		  break;
		case 'd':
		case 'D':
		case 'r':
		case 'R':
		  *(double *)words = result.d;
		  break;
		case 'p':
		case 'P':
		case 'x':
		case 'X':
		  *(double *)words = result.d;
		  break;
		}
		while(isalpha(*p))p++;
		return(p);
	}
	/* 
	 * Scan numeric chars until we hit a '.' or something we don't 
	 * recognize.
	 */
	while(isdigit(*p)) {
		if (*p == '0' && !seen_non_zero) {
			p++;
			continue;
		}
		seen_non_zero = 1;
		mantissa[(MAXMANTISSA-1)-left_of_decimal++]  = (*p++) - '0';
	}
	/* Check for a '.' and collect digits to the right */
	if (*p == '.') {
		int mantindex = MAXMANTISSA-1-left_of_decimal;
		/* Collect digits to the right of the decimal */
		p++;
		while(isdigit(*p)) {
			if (!seen_non_zero && *p == '0') {
				p++;
				left_of_decimal--;
				continue;
			}
			seen_non_zero = 1;
			if (mantindex >= 0)
			    mantissa[mantindex--] = (*p++) - '0';
			else 
			    p++;
		}
	}
	SKIPWHITE(p);
	if (*p == 'e' || *p == 'E') {
		p++;
		SKIPWHITE(p);
		/* optional sign before optional exponent */
		if (*p == '+')
			p++;
		else if (*p == '-') {
			p++;
			pd_fp.pf_signexp = 1;
		}
		SKIPWHITE(p);
		/* Collect exponent */
		while(isdigit(*p)) {
			/* ??? Should check for gross overflow here??? */
			exponent *= 10;
			exponent += (*p++) - '0';
		}
	}
	/* If signed, negate exponent now.  */
	if (pd_fp.pf_signexp)
		exponent *= -1;
	/* Adjust exponent by # of digits to the left of decimal - 1 */
	exponent += left_of_decimal - 1;
	if (exponent < 0) {
		exponent *= -1;
		pd_fp.pf_signexp = 1;
	} else
		/* Exponent could have gone positive.  If so, force sign
		   to be positive. */
		pd_fp.pf_signexp = 0;
	/* Move exponent into the packed decimal structure. */
	if (exponent > 999) {
		/* Exponent overflow! */
		errno = ERANGE;
#ifndef NeXT
		fprintf(stderr, "as: atof exponent overflow: %s\n", str);
#endif
		bzero(words, sizeof(double));
		if (pd_fp.pf_signman)
			*words = 0xfff0;
		else
			*words = 0x7ff0;
		return(p);
	}
	pd_fp.pf_exp0 = exponent % 10;
	pd_fp.pf_exp1 = (exponent/10) % 10;
	pd_fp.pf_exp2 = (exponent/100) % 10;
	/* Move mantissa into the packed decimal structure. */
	pd_fp.pf_man16 = mantissa[16];
	pd_fp.pf_man15_8 = packmantissa(&mantissa[15]);
	pd_fp.pf_man7_0 = packmantissa(&mantissa[7]);
	/* Temporary */
	bzero(words, 6);
	/* End temporary */
	switch(type) {
	case 'f':
	case 'F':
	case 's':
	case 'S':
		packed_decimal_float_to_single(&pd_fp, words);
		break;
	case 'd':
	case 'D':
	case 'r':
	case 'R':
		packed_decimal_float_to_double(&pd_fp, words);
		break;
	case 'p':
	case 'P':
	case 'x':
	case 'X':
		packed_decimal_float_to_extended(&pd_fp, words);
		break;
	}
	return(p);
}

unsigned int 
packmantissa(cp)
char *cp;
{
	unsigned int result = 0;
	int i;

	for(i=0;i<8;i++) {
		result <<= 4;
		result |= *cp--;
	}
	return(result);
}

gen_to_words(a,b,c)
{
	as_where();
	fprintf(stderr, " gen_to_words, file=%s, line=%d, a=0x%x, b=0x%x, c=0x%x\n", 
		a,b,c);
	abort();
}
