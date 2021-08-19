    /*
     *	offset (in bytes) of the code from the entry address of a routine.
     *	(see asgnsamples for use and explanation.)
     */
#define OFFSET_OF_CODE	0
#define	UNITS_TO_CODE	(OFFSET_OF_CODE / sizeof(UNIT))

#define BSR_OP		0x6100
#define BSR_MASK	0xff00
#define JBSR_OP		0x4e80
#define JBSR_MASK	0xffc0

