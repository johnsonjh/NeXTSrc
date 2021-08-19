/* ttyloc.h -- describes the format of the binary ttyloc file */

#define TTYLOCFILE	"/etc/ttyloc.bin"

/* All integers are sizeof(long) bytes long, and are stored in the native
   byte order of the machine.

   The file has three sequences of tables: the host table, the terminal
   tables and the location strings.

   The prelude comes first in the file.
 */

#define TTYMAGIC	0x10c10ccc

struct TTYPrelude {
    long magic;			/* should be == TTYMAGIC */
    long timestamp;		/* from TOPS-20 */
};

/* Next comes the count of the number of host entries, followed by the
   entries themselves. */

struct HostEntry {
    long hostID;		/* host address */
    long termOffset;		/* offset of the terminal table */
};

/* Then we have the terminal tables for each host.  Each contains an entry
   count followed by a number of terminal entries. */

struct TermEntry {
    long termID;
    long locOffset;		/* offset of description */
};

/* The locOffset field points into the location part of the file which has a
   number of null-terminated terminal locations. */
