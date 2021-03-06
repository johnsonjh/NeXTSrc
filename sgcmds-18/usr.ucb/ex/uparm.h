#ifdef __STDC__
#define libpath(file) "/usr/lib/" #file
#define loclibpath(file) "/usr/local/lib/" #file
#define binpath(file) "/usr/ucb/" #file
#define usrpath(file) "/usr/" #file
#else
#define libpath(file) "/usr/lib/file"
#define loclibpath(file) "/usr/local/lib/file"
#define binpath(file) "/usr/ucb/file"
#define usrpath(file) "/usr/file"
#endif __STDC__

#define E_TERMCAP	"/etc/termcap"
#define B_CSH		"/bin/csh"
