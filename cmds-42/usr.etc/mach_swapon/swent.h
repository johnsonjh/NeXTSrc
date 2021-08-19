/*
 * swent.h - Header file for swaptab entries.
 *
 * Copyright (c) 1989 by NeXT, Inc.
 *
 **********************************************************************
 * HISTORY
 * 28-Feb-89  Peter King (king) at NeXT
 *	Created.
 **********************************************************************
 */

/*
 * Swaptab entry
 */
struct swapent {
	char		*sw_file;
	unsigned int	sw_prefer : 1;
	unsigned int	sw_noauto : 1;
	unsigned int	sw_lowat;
	unsigned int	sw_hiwat;
};
typedef struct swapent *swapent_t;
#define SWAPENT_NULL (swapent_t)0

/*
 * Swapent related routines:
 *
 * swent_start	- Sets up to read through swaptab entries.
 * swent_get	- Gets next swaptab entry, returns NULL when done.
 * swent_rele	- Release swaptab entry returned by swent_get().
 * swent_end	- Cleans up after reading through swaptab entries.
 * swent_parseopts - Parses options in string "str" into "sw".
 */
int		swent_start(char *);
swapent_t	swent_get();
void		swent_rele(swapent_t);
void		swent_end();
void		swent_parseopts(char *str, swapent_t sw);

