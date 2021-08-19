/*  del  --  interrupt handling routine
 *
 *  ENABLEDEL;	will use this routine to increment the variable "_del_"
 *  whenever an interrupt is received.  Other macros can test the
 *  value of this variable and do interesting things.
 *
 *  HISTORY
 * 17-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.
 *
 */

#include <signal.h>
int _del_;

struct sigvec _del_vec = {0,0,0};

del(sig,code,scp)
int sig,code;
struct sigcontext *scp;
{
  _del_ ++;
}
