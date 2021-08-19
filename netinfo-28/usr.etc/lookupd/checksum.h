typedef struct checksum {
	unsigned len;
	unsigned *val;
} checksum;

void checksum_free(checksum  *sum);
int checksum_eq(checksum sum1, checksum sum2);
int checksum_valid(checksum sum);
void checksum_mark(void);
int checksum_unchanged(void);
int checksum_makes_no_sense(void);

