/*	ranlib.h	4.1	83/05/03	*/

/*
 * There are two known orders of table of contents for archives.  The first is
 * the order ranlib(1) originally produced and still produces without any
 * options.  This table of contents has the archive member name "__.SYMDEF"
 * This order has the ranlib structures in the order the objects appear in the
 * archive and the symbol names of those objects in the order of symbol table.
 * The second know order is sorted by symbol name and is produced with the -s
 * option to ranlib(1).  This table of contents has the archive member name
 * "__.SYMDEF SORTED" and many programs (notably the 1.0 version of ld(1) can't
 * tell the difference between names because of the imbedded blank in the name
 * and works with either table of contents).  This second order is used by the
 * post 1.0 link editor to produce faster linking.  The original 1.0 version of
 * ranlib(1) gets confused when it is run on a archive with the second type of
 * table of contents because it and ar(1) which it uses use different ways to
 * determined the member name (ar(1) treats all blanks in the name as
 * significant and ranlib(1) only checks for the first one).
 */
#define SYMDEF		"__.SYMDEF"
#define SYMDEF_SORTED	"__.SYMDEF SORTED"

/*
 * Structure of the __.SYMDEF table of contents for an archive.
 * __.SYMDEF begins with a long giving the size in bytes of the ranlib
 * structures which immediately follow, and then continues with a string
 * table consisting of a long giving the number of bytes of strings which
 * follow and then the strings themselves.  The ran_strx fields index the
 * string table whose first byte is numbered 0.
 */
struct	ranlib {
	union {
		off_t	ran_strx;	/* string table index of */
		char	*ran_name;	/* symbol defined by */
	} ran_un;
	off_t	ran_off;		/* library member at this offset */
};
