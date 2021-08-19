#include <stddef.h>
#include <math.h>

#undef errno

typedef struct {
	unsigned pad : 24;
	unsigned iop : 1;
	unsigned ovfl : 1;
	unsigned unfl : 1;
	unsigned dz : 1;
	unsigned inex : 1;
	unsigned pad2 : 3;
} fpsr_t;

#pragma CC_OPT_OFF
int *_errno() {
	register fpsr_t fpsr;
	asm ("fmovel fps,%0": "=g" (fpsr));
	if (fpsr.iop) {
		if (errno == 0) errno = EDOM;
		fpsr.iop = 0;
	}
	if (fpsr.ovfl) {
		if (errno == 0) errno = ERANGE;
		fpsr.ovfl = 0;
	}
	asm ("fmovel %0,fps": "g" (fpsr));
	return &errno;
}
#pragma CC_OPT_RESTORE

