/*
 * if not profiling, defining a fake moncontrol()
 * just in case we're loaded with a few profiling routines
 */
	.text
	.globl	_moncontrol
_moncontrol:
	rts
