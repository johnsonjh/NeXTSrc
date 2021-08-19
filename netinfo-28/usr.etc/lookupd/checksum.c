#include <stdio.h>
#include <stdlib.h>
#include "clib_internal.h"
#include "checksum.h"
#include "mm.h"

static checksum sum_start;
static checksum sum_finish;

int
checksum_makes_no_sense(void)
{
	/*
	 * XXX: We should really factor out the netinfo stuff here,
	 * so that YP users can get the caching benefit too (although, they
	 * will still have to pay for non-caching YP).
	 */
	return (_yp_running());
}

int
checksum_valid(
	       checksum sum
	       )
{
	return (sum.len > 0);
}


int
checksum_eq(
	    checksum sum1,
	    checksum sum2
	    )
{
	int i;

	if (sum1.len != sum2.len) {
		return (0);
	}
	for (i = 0; i < sum1.len; i++) {
		if (sum1.val[i] != sum2.val[i]) {
			return (0);
		}
	}
	return (1);
}


void
checksum_free(
	      checksum  *sum
	      )
{
	if (sum->len > 0) {
		MM_FREE(sum->val);
		sum->val = NULL;
		sum->len = 0;
	}
}

void 
checksum_mark()
{
	if (checksum_makes_no_sense()) {
		return;
	}
	checksum_free(&sum_finish);
	ni_getchecksums(&sum_finish.len, &sum_finish.val);
}

int 
checksum_unchanged()
{
	checksum cursum;

	if (checksum_makes_no_sense()) {
		return (0);
	}
	ni_getchecksums(&cursum.len, &cursum.val);
	if (checksum_valid(sum_start) &&
	    checksum_valid(sum_finish) &&
	    checksum_eq(sum_start, sum_finish) &&
	    checksum_eq(sum_start, cursum)) {
		checksum_free(&cursum);
		return (1);
	}

	checksum_free(&sum_start);
	checksum_free(&sum_finish);
	sum_start = cursum;
	return (0);
}
