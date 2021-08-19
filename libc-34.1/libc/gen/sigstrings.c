#include <signal.h>

#define ss(str, num, sig) \
const char _sys_siglist_##sig[] = str;

#include "siglist.h"
