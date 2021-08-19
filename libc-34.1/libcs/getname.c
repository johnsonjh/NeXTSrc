/*
 * This file contains the code for the following getname routine:
 *
 * getname:
 *	purpose:
 *		getname returns the user from the password file
 *		associated with a specified user-id. The user-id
 *		is passed as an integer parameter to the routine.
 *
 *	If a user whose id matches the parameter uid is found in the
 *	password file, the name corresponding to that user is returned.
 *	If no match is found, NULL is returned.  If malloc() fails,
 *	NULL is returned.  If id is -1, all allocated memory is freed.
 *
 *	During the search for a particular user-id, a hash table names
 *	is built for storing user-id's and their corresponding names
 *	from the password file. When the routine is first called, entries
 *	are read from the password file until a match for uid, or the
 *	end of the password file, is found. Any entries not matching uid
 *	and not already stored in the names structure are then stored in
 *	names.
 *
 *	On subsequent calls to getname, the structure names is checked
 *	first for a match for uid. If no match is found, and  if the entire
 *	password file has not been stored, entries from it are stored in
 *	names as before.
 *
 *	When the entire password file has been stored, only names
 *	is checked for a matching user-id on subsequent calls to getname.
 **********************************************************************
 * HISTORY
 * 25-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Ran through lint and fixed bad routine calls and returns.  Added
 *	register variables where apropriate.  Added code to detect malloc
 *	failures.  Added special call value (-1) to free hash table for
 *	cleaning up allocated memory.
 *
 **********************************************************************
 */

#include <pwd.h>			/* passwd struct include file */
#include <utmp.h>			/* utmp struct include file */
#include <stdio.h>

#define HASHBITS 6			/* number of bits to hash */
#define HASHSIZE (1<<HASHBITS)		/* limit on the number of buckets
					   in the hash table */
#define HASHMASK (HASHSIZE-1)		/* mask of bits to hash */
#define hash(uid) ((uid)&HASHMASK)	/* determines in which of the HASHSIZE
					   buckets uid would belong */
static struct utmp ut;
#define NMAX (sizeof ut.ut_name)	/* maximum length for a name */

/* struct for storing an entry of the hash table */
struct entry {
	char a_name[NMAX+1];		/* stores the name of the user  */
	int a_uid;			/* stores the uid of the user   */
	struct entry *next;		/* stores the pointer to the
					   next entry of the hash table */
};

static struct entry *names[HASHSIZE];	/* the hash table of users */

/* returns the entry of names that stores the user of id uid if
   one exists in names, else NULL is returned */
static struct entry *inset(uid)
	register int uid;
{
	register struct entry *temp;

	for (temp = names[hash(uid)]; temp != NULL; temp = temp->next)
		if (temp->a_uid == uid)
			return(temp);
	return(NULL);
}

/* allocates space for an entry in names, setting next field to NULL */
static struct entry *make_blank_entry()
{
	register struct entry *blank_entry;

	blank_entry = (struct entry*)(malloc(sizeof(struct entry)));
	if (blank_entry == NULL)
		return(NULL);
	blank_entry->next = NULL;
	return(blank_entry);
}

/* inserts a blank entry into the correct position of names for a user
   whose id is uid */
static struct entry *place_blank_entry(uid)
	register int uid;
{
	register struct entry **temp;
	struct entry *make_blank_entry();

	temp = &names[hash(uid)];
	while (*temp != NULL)
		temp = &((*temp)->next);
	return(*temp = make_blank_entry());
}

/* inserts the data of an entry from the password file (stored in pw_entry)
   into an entry of names (the parameter blank_entry) */
static fill_in(blank_entry, pw_entry)
	register struct entry *blank_entry;
	register struct passwd *pw_entry;
{
	strncpy(blank_entry->a_name, pw_entry->pw_name, NMAX);
	blank_entry->a_name[NMAX] = '\0';
	blank_entry->a_uid = pw_entry->pw_uid;
}

/* creates an entry in names storing the data of an entry from the
   password file which stored in the paramter passwd_entry */
static struct entry *create_names_entry(passwd_entry)
	register struct passwd *passwd_entry;
{
	register struct entry *new_entry;
	struct entry *place_blank_entry();

	new_entry = place_blank_entry(passwd_entry->pw_uid);
	if (new_entry == NULL)
		return(NULL);
	fill_in(new_entry, passwd_entry);
	return(new_entry);
}

/* initializes hash table */
static set_hash_table()
{
	register int i;

	for (i = 0; i < HASHSIZE; i++)
		names[i] = NULL;
}

/* free hash table */
static free_hash_table()
{
	register int i;
	register struct entry *temp;

	for (i = 0; i < HASHSIZE; i++)
		while ((temp = names[i]) != NULL) {
			names[i] = temp->next;
			free((char *)temp);
		}
}

/* returns the name of the user in the passwords file whose id is uid.
   NULL is returned if none exists */
char *getname(uid)
register uid;
{
	register struct passwd *pw;	/* pre-defined struct */
	static init;			/* indicates status of the password file
						0 = file unopen
						1 = file open
						2 = file entirely read
					 */
	/* pre-defined routines for accessing the password routine */
	struct passwd *getpwent();
	int setpwent();
	int endpwent();

	struct entry *inset();
	register struct entry *Location;

	if (uid == -1) {
		if (init != 0)
			free_hash_table();
		if (init == 1)
			endpwent();
		init = 0;
		return(NULL);
	}

	if (init == 0) {
		set_hash_table();
		setpwent();
		init = 1;
	}

	Location = inset(uid);		/* check if user is in names */
	if (Location != NULL)
		return(Location->a_name); /* user already stored in names */
	if (init == 2)
		return(NULL);		/* entire password file has been
					   stored in names */

       /* continue reading entries from the password file, storing any in
	  names whose uid is not already located in names, stopping when
	  a match is found for the uid or the entire password file has
	  been stored */

	while ((pw = getpwent()) != NULL) {
		Location = inset(pw->pw_uid);
		if (Location != NULL)
			continue;
		if (create_names_entry(pw) == NULL)
			return(NULL);
		if (pw->pw_uid == uid)
			return(pw->pw_name);
	}
	init = 2;
	endpwent();
	return(NULL);
}
