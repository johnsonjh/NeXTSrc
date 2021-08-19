/*
 * hash_string() compute a hash code for the specified null terminated string.
 * The caller can then mod it with the size of the hash table.
 */
static
inline
long
hash_string(
char *key)
{
    char *cp;
    long k;

	cp = key;
	k = 0;
	while(*cp)
	    k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;
	return(k);
}
