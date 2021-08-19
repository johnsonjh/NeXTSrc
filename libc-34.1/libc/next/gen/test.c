struct pd_fp {
	unsigned int
		pf_signman:1,
		pf_signexp:1,
		pf_infnan:2,
		pf_exp[3]:4,
		pf_exp3:4,
		pf_xxx0:4,
		pf_xxx1:4,
		pf_man[17]:4;
} pf = { 0xc123fff0, 0x12345678, 0x9abcdef0 };

main()
{
	register int i;

	printf("pf_signman = %d\n", pf.pf_signman);
	printf("pf_signexp = %d\n", pf.pf_signexp);
	printf("pf_infnan = %d\n", pf.pf_infnan);
	printf("pf_exp = \n");
	for (i = 0; i < 3; i++)
		printf("%c", pf.pf_exp[i] + '0');
	printf("\n");
	printf("pf_man = \n");
	for (i = 0; i < 17; i++)
		printf("%c", pf.pf_man[i] + '0');
	printf("\n");
}
