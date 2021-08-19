main()
{
	char *index();
	char *cp;

	printf("index(\"abcdef\", 'c')=%s\n", index("abcdef", 'c'));
	printf("index(\"abcdef\", 'a')=%s\n", index("abcdef", 'a'));
	printf("index(\"abcdef\", 'f')=%s\n", index("abcdef", 'f'));
	if (index("abcdef", 'g') != 0)
		printf("index failed\n");
	cp = "xxxxxx";
	if (index(cp, '\0') != &cp[strlen(cp)])
		printf("index(cp, '\\0') failed\n");
	exit(0);
}
