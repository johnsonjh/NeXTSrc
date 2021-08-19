

#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreePrivate.h"

int 
NXBTreeCompareUnsignedShorts(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	short	resultValue;

	limitValue = 
		((char *) data1) + ((length1 > length2) ? length2 : length1);
	for (; (char *) data1 < limitValue; 
			((unsigned short *) data1)++, ((unsigned short *) data2)++)
		if (resultValue = 
			((*((unsigned short *) data1) < *((unsigned short *) data2)) ? -1 : 
		((*((unsigned short *) data1) > *((unsigned short *) data2)) ? 1 : 0)))
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareShorts(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	short	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	while ((char *) data1 < limitValue)
		if (resultValue = *((short *) data1)++ - *((short *) data2)++)
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareUnsignedLongs(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	long	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	for (; (char *) data1 < limitValue; 
			((unsigned long *) data1)++, ((unsigned long *) data2)++)
		if (resultValue = ((*((unsigned long *) data1) < *((unsigned long *) data2)) ? -1 : 
				((*((unsigned long *) data1) > *((unsigned long *) data2)) ? 1 : 0)))
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareLongs(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	long	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	while ((char *) data1 < limitValue)
		if (resultValue = *((long *) data1)++ - *((long *) data2)++)
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareUnsignedBytes(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	u_char	*limitValue;
	u_char	resultValue;

	limitValue = (u_char *) data1 + ((length1 > length2) ? length2 : length1);
	for (; (u_char *) data1 < limitValue; 
			((u_char *) data1)++, ((u_char *) data2)++)
		if (resultValue = ((*((u_char *) data1) < *((u_char *) data2)) ? -1 : 
				((*((u_char *) data1) > *((u_char *) data2)) ? 1 : 0)))
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareBytes(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	char	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	while ((char *) data1 < limitValue)
		if (resultValue = *((char *) data1)++ - *((char *) data2)++)
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareMonocaseStrings(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	long	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	for (; (char *) data1 < limitValue; ((char *) data1)++, ((char *) data2)++)
		if (resultValue = 
				monocase(*((char *) data1)) - monocase(*((char *) data2)))
			return resultValue;

	return length1 - length2;
}

int 
NXBTreeCompareStrings(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2)
{
	char	*limitValue;
	long	resultValue;

	limitValue = ((char *) data1) + ((length1 > length2) ? length2 : length1);
	while ((char *) data1 < limitValue)
		if (resultValue = *((char *) data1)++ - *((char *) data2)++)
			return resultValue;

	return length1 - length2;
}

