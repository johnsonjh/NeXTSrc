#import <objc/Object.h>

#define LOG_ENTRY 17		/* there are about 2^16 entries */
#define LOG_EXPANSION 1		/* to avoid hash collisions */
#define LOG_BYTES_PER_BIT (-3)	
#define LOG_DICTSIZE (LOG_ENTRY + LOG_EXPANSION + LOG_BYTES_PER_BIT)
#define DICTSIZE (1 << LOG_DICTSIZE)

#define MOD_MASK ((unsigned int)((1 << (LOG_ENTRY + LOG_EXPANSION)) - 1))
#define DICT_MOD(x) ((unsigned int) (x % MOD_MASK))
#define BITS_PER_BYTE ((unsigned int) 8)
#define BIT_ADDR(hash) (hash & (BITS_PER_BYTE - 1))
#define SET_BIT(char, hash) ( (1 << BIT_ADDR(hash)) | char )
#define GET_BIT(char, hash) ((1 << BIT_ADDR(hash)) & char)
#define DICT_VERSION 1
#define NDICTS 8

typedef struct {
    int version;
    int nTables;
    int tableSize;
    int nEntries;
    int firstTable;
    char source[0];
} DictHeader;

extern void setDict(const char *entry, unsigned char *tables[]);









