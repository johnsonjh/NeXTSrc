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
	double integer;
	double fraction;
	double modf();

	for (i = 0; i < NELEM(d); i++) {
		fraction = modf(d[i], &integer);
		printf("d=%.15g, fraction=%.15g, integer=%f\n", d[i],
		    fraction, integer);
	}
	exit(0);
}
