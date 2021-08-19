/*
 * a fake mcount in case someone links in a profiling routine or two
 */
	.text
	.globl	mcount
mcount:
	rts
