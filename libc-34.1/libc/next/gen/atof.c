#include <ctype.h>
#include <sys/errno.h>

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

static unsigned int packmantissa();

double
atof(str)
char *str;
{
	register char *p = str;
	struct pd_fp pd_fp;
	char mantissa[MAXMANTISSA];
	int left_of_decimal, exponent, mantindex;
	int seen_non_zero;	/* To the left of decimal */
	unsigned int packmantissa();
	extern double _pdfptodbl();
	extern int errno;

	/* Setup data structures */
	pd_fp.pf_signman = 0;
	pd_fp.pf_signexp = 0;
	pd_fp.pf_infnan = 0;
	pd_fp.pf_xxx0 = 0;
	pd_fp.pf_xxx1 = 0;
	pd_fp.pf_exp3 = 0;
	mantindex = 0;
	exponent = 0;
	left_of_decimal = 0;
	seen_non_zero = 0;
	bzero(mantissa, sizeof(mantissa));

	SKIPWHITE(p);
	if (*p == '-') {
		p++;
		pd_fp.pf_signman = 1;
	} else if (*p == '+')
		p++;
	SKIPWHITE(p);
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
		return(result.d);
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
		mantindex = MAXMANTISSA - 1 - left_of_decimal++;
		if (mantindex >= 0)
			mantissa [mantindex] = (*p++) - '0';
		else
			*p++;
	}
	/* Check for a '.' and collect digits to the right */
	if (*p == '.') {
		mantindex = MAXMANTISSA-1-left_of_decimal;
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
			exponent *= 10;
			exponent += (*p++) - '0';
			/* check for gross overflow */
			if (exponent > 100000)
				goto expover;
		}
	}
	/* If signed, negate exponent now.  */
	if (pd_fp.pf_signexp)
		exponent = -exponent;
	/* Adjust exponent by # of digits to the left of decimal - 1 */
	exponent += left_of_decimal - 1;
	if (exponent < 0) {
		exponent = -exponent;
		pd_fp.pf_signexp = 1;
	} else
	        /* Exponent could have gone positive. If so, force
		 * sign to be positive. */
	        pd_fp.pf_signexp = 0;
	/* Move exponent into the packed decimal structure. */
	if (exponent > 999) {
		union {
			double d;
			unsigned int x[2];
		} inf;
expover:
		/* Exponent overflow! */
		if (pd_fp.pf_signexp)
			return(0.0);
		errno = ERANGE;
		if (pd_fp.pf_signman)
			inf.x[0] = 0x7ff00000;
		else
			inf.x[0] = 0xfff00000;
		inf.x[1] = 0;
		return(inf.d);
	}
	pd_fp.pf_exp0 = exponent % 10;
	pd_fp.pf_exp1 = (exponent/10) % 10;
	pd_fp.pf_exp2 = (exponent/100) % 10;
	/* Move mantissa into the packed decimal structure. */
	pd_fp.pf_man16 = mantissa[16];
	pd_fp.pf_man15_8 = packmantissa(&mantissa[15]);
	pd_fp.pf_man7_0 = packmantissa(&mantissa[7]);
	return(_pdfptodbl(&pd_fp));
}

static unsigned int 
packmantissa(cp)
char *cp;
{
	register unsigned int result = 0;
	register int i;

	for(i=0;i<8;i++) {
		result <<= 4;
		result |= *cp--;
	}
	return(result);
}
