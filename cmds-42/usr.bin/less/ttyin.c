/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.h"

static int tty;

/*
 * Open keyboard for input.
#if	NeXT
 * (Just use file descriptor 1.)
 * (Since for some reason the SUN csh doesn't
 * always get 2 hooked up correctly to the tty
 * when executing shell scripts.)
#else
 * (Just use file descriptor 2.)
#endif	NeXT
 */
	public void
open_getchr()
{
#if	NeXT
	tty = 2;
#else
	tty = 2;
#endif	NeXT
}

/*
 * Get a character from the keyboard.
 */
	public int
getchr()
{
	char c;
	int result;

	do
	{
		result = iread(tty, &c, 1);
		if (result == READ_INTR)
			return (READ_INTR);
		if (result < 0)
		{
			/*
			 * Don't call error() here,
			 * because error calls getchr!
			 */
			quit();
		}
	} while (result != 1);
	return (c & 0177);
}
