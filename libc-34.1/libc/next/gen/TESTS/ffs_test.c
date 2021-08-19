main()
{
	register int i, j, pat;

	for (i = 0; i < 33; i++) {
		pat = 1<<i;
		for (j = 0; j < 20000; j++)
			xffs(pat);
	}
	exit(0);
}
