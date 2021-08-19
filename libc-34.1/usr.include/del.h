/*  del  --  interrupt handling macros
 *
 *  HISTORY
 * 17-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Messages also moved to stderr instead of stdout.
 *
 */

#include <signal.h>

extern int del();
extern int _del_;
extern struct sigvec _del_vec;

#define ENABLEDEL {_del_=0; _del_vec.sv_handler=del; sigvec(SIGINT,&_del_vec,0);}
#define DISABLEDEL {_del_=0; _del_vec.sv_handler=SIG_DFL; sigvec(SIGINT,&_del_vec,0);}
#define IGNOREDEL {_del_=0; _del_vec.sv_handler=SIG_IGN; sigvec(SIGINT,&_del_vec,0);}

#define _DELNOTE_	_del_=0; fprintf (stderr,"  Break!\n"); fflush (stderr);
#define DELBREAK	if (_del_) {_DELNOTE_ break;}
#define DELRETURN	if (_del_) {_DELNOTE_ return;}
#define DELRETN(x)	if (_del_) {_DELNOTE_ return (x);}
#define DELCLEAR	if (_del_) {_del_=0; fprintf (stderr,"Break ignored.\n"); fflush (stderr);}
