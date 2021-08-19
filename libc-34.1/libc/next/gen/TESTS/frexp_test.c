double d[] = {
	1.0,
	2.0,
	4.0,
	0.5,
	.25,
	-1.0,
	-2.0,
	-4.0,
	-0.5,
	-.25,
	4096.0,
	1234567890123456789e100,
	1234567890123456789e-100,
	.1234567890123456789e100,
	.1234567890123456789e-100,
	.1234567890123456789e300,
	.1234567890123456789e-300,
};

#define	NELEM(x)	(sizeof(x)/sizeof((x)[0]))

main()
{
	register int i;
	int exp;
	double mantissa;
	double frexp();
	double xfrexp();

	for (i = 0; i < NELEM(d); i++) {
		mantissa = xfrexp(d[i], &exp);
		printf("d=%.15g, mantissa=%.15g, exponent=%d\n", d[i],
		    mantissa, exp);
		mantissa = frexp(d[i], &exp);
		printf("d=%.15g, mantissa=%.15g, exponent=%d\n", d[i],
		    mantissa, exp);
	}
	exit(0);
}
