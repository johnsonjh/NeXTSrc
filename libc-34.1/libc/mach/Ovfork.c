/*****************************************************
 *
 *  Abstract:
 *	Just calls fork, as there the vfork kernel call no longer
 *	does anything different from fork.  The fork program calls
 *	mach_init for the child.
 *
 *  HISTORY
 *    Jan-15-87	Mary R. Thompson
 *	Started
 *****************************************************/


  int vfork()
  {  return( fork());
  }
