/*  putenv  --  put value into environment
 *
 *  Usage:  i = putenv (name,value)
 *	int i;
 *	char *name, *value;
 *
 *  Putenv associates "value" with the environment parameter "name".
 *  If "value" is 0, then "name" will be deleted from the environment.
 *  Putenv returns 0 normally, -1 on error (not enough core for malloc).
 *
 *  Putenv may need to add a new name into the environment, or to
 *  associate a value longer than the current value with a particular
 *  name.  So, to make life simpler, putenv() copies your entire
 *  environment into the heap (i.e. malloc()) from the stack
 *  (i.e. where it resides when your process is initiated) the first
 *  time you call it.
 *
 *  HISTORY
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.  Too bad Bell Labs didn't provide this.  It's
 *	unfortunate that you have to copy the whole environment onto the
 *	heap, but the bookkeeping-and-not-so-much-copying approach turns
 *	out to be much hairier.  So, I decided to do the simple thing,
 *	copying the entire environment onto the heap the first time you
 *	call putenv(), then doing realloc() uniformly later on.
 *	Note that "putenv(name,getenv(name))" is a no-op; that's the reason
 *	for the use of a 0 pointer to tell putenv() to delete an entry.
 *
 */

#define EXTRASIZE 5		/* increment to add to env. size */

char *index(), *malloc(), *realloc();
int strlen();

static int envsize = -1;	/* current size of environment */
extern char **environ;		/* the global which is your env. */

static int findenv();		/* look for a name in the env. */
static int newenv();		/* copy env. from stack to heap */
static int moreenv();		/* incr. size of env. */

int putenv (name,value)
char *name, *value;
{
	register int i,j;
	register char *p;

	if (envsize < 0) {	/* first time putenv called */
		if (newenv() < 0)	/* copy env. to heap */
			return (-1);
	}

	i = findenv (name);	/* look for name in environment */

	if (value) {		/* put value into environment */
		if (i < 0) {	/* name must be added */
			for (i=0; environ[i]; i++) ;
			if (i >= (envsize - 1)) {	/* need new slot */
				if (moreenv() < 0)  return (-1);
			}
			p = malloc (strlen(name) + strlen(value) + 2);
			if (p == 0)	/* not enough core */
				return (-1);
			environ[i+1] = 0;	/* new end of env. */
		}
		else {		/* name already in env. */
			p = realloc (environ[i],
				strlen(name) + strlen(value) + 2);
			if (p == 0)  return (-1);
		}
		sprintf (p,"%s=%s",name,value);	/* copy into env. */
		environ[i] = p;
	}
	else {			/* delete name from environment */
		if (i >= 0) {	/* name is currently in env. */
			free (environ[i]);
			for (j=i; environ[j]; j++) ;
			environ[i] = environ[j-1];
			environ[j-1] = 0;
		}
	}

	return (0);
}

static int findenv(name)
char *name;
{
	register char *namechar, *envchar;
	register int i,found;

	found = 0;
	for (i=0; environ[i] && !found; i++) {
		envchar = environ[i];
		namechar = name;
		while (*namechar && (*namechar == *envchar)) {
			namechar++;
			envchar++;
		}
		found = (*namechar == '\0' && *envchar == '=');
	}
	return (found ? i-1 : -1);
}

static int newenv ()
{
	register char **env, *elem;
	register int i,esize;

	for (i=0; environ[i]; i++) ;
	esize = i + EXTRASIZE + 1;
	env = (char **) malloc (esize * sizeof (elem));
	if (env == 0)  return (-1);

	for (i = 0; environ[i]; i++) {
		elem = malloc (strlen(environ[i]) + 1);
		if (elem == 0)  return (-1);
		env[i] = elem;
		strcpy (elem,environ[i]);
	}

	env[i] = 0;
	environ = env;
	envsize = esize;
	return (0);
}

static int moreenv()
{
	register int esize;
	register char **env;

	esize = envsize + EXTRASIZE;
	env = (char **) realloc (environ,esize * sizeof(*env));
	if (env == 0)  return (-1);
	environ = env;
	envsize = esize;
	return (0);
}
