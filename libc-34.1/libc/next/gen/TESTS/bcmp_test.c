char a[512];
char b[512];

main()
{
	int i, j, l, k, m;

	for (i = 1; i < 5; i++)
	for (j = 1; j < 5; j++)
	for (l = 0; l < sizeof(a)-5; l++) {

		for (k = 0; k < l; k++)
			a[k+i] = k;
		a[i-1] = a[l+i] = 0;

		for (k = 0; k < l; k++)
			b[k+j] = k;
		b[j-1] = b[l+j] = 1;

		if (bcmp(&a[i], &b[j], l) != 0)
			printf("bcmp failed i=%d j=%d l=%d\n",
			    i, j, l);
	}

#ifdef notdef
	for (i = 1; i < 5; i++)
	for (j = 1; j < 5; j++)
	for (l = 0; l < sizeof(a)-5; l++)
	for (m = 0; m < l; l++) {
		for (k = 0; k < l; k++) a[k+i] = k;
		a[i-1] = a[l+i] = 0;
		for (k = 0; k < l; k++) b[k+j] = k;
		b[j-1] = a[l+j] = 1;
		a[m+i] = a[m+i] - 1;
		if (bcmp(&a[i], &b[j], l) == 0)
			printf("bcmp succeeded i=%d j=%d l=%d m=%d\n",
			    i, j, l, m);
	}
#endif notdef
	exit(0);
}
