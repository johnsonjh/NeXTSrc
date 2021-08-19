struct dnes {
	double d;
	int e;
} dnes[] = {
	{ 1.0, 0 },
	{ 2.0, 0 },
	{ 4.0, 0 },
	{ .5, 0 },
	{ .25, 0 },
	{ 1.0, 1 },
	{ 2.0, 1 },
	{ 4.0, 1 },
	{ .5, 1 },
	{ .25, 1 },
	{ 1.0, -1 },
	{ 2.0, -1 },
	{ 4.0, -1 },
	{ .5, -1 },
	{ .25, -1 },
	{ -1.0, 0 },
	{ -2.0, 0 },
	{ -4.0, 0 },
	{ -.5, 0 },
	{ -.25, 0 },
	{ -1.0, 1 },
	{ -2.0, 1 },
	{ -4.0, 1 },
	{ -.5, 1 },
	{ -.25, 1 },
	{ -1.0, -1 },
	{ -2.0, -1 },
	{ -4.0, -1 },
	{ -.5, -1 },
	{ -.25, -1 }
};

#define	NELEM(x)	(sizeof(x)/sizeof((x)[0]))

main()
{
	double ldexp();
	register int i;
	register struct dnes *dep;

	for (dep = dnes; dep < &dnes[NELEM(dnes)]; dep++)
		printf("ldexp(%f, %d)=%f\n", dep->d, dep->e,
		    ldexp(dep->d, dep->e));
	exit(0);
}
