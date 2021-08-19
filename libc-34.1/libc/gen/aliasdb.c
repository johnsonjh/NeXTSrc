/*
 * Unimplemented backward-compatible alias lookup routines.
 * These are only called if NetInfo is not running or if YP
 * has been turned on, in which case NetInfo will be tried first.
 * This is the standard NeXT policy for all lookup routines.
 */
#include <stdio.h>
#include <aliasdb.h>

void
_old_alias_setent(void)
{
}

aliasent *
_old_alias_getent(void)
{
	return (NULL);
}

void
_old_alias_endent(void)
{
}

aliasent *
_old_alias_getbyname(char *name)
{
	return (NULL);
}
