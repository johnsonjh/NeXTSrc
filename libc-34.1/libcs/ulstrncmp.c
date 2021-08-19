/*
 * case INSENSITIVE.
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

ulstrncmp(s1, s2, n)
register char *s1, *s2;
register int n;
{
    register char c1,c2;

    for(;;) {
	if (--n < 0) 
	    return(0);
	c1 = *s1++;
	if (c1 <= 'Z')
	    if (c1 >= 'A')
		c1 += 040;
	c2 = *s2++;
	if (c2 <= 'Z')
	    if (c2 >= 'A')
		c2 += 040;
	if (c1 != c2)
	    break;
	if (c1 == '\0')
	    return(0);
    }
    return(c1 - c2);
}
