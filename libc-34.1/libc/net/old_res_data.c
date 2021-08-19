/*
 * This file **CANNOT** be removed from the LIBC library.
 * It is included for 1.0 SHLIB compatibility reasons.
 * The size of _res changed with the upgrade to the
 * BIND 4.8 distribution.  The symbol here sits on the same
 * address as the old _res symbol for 1.0 and is the 
 * exact same size.
 *
 * Anything that used _res in 1.0 is incompatible and will
 * have to be recompileds.
 */

char _old_res [324] = {0};
