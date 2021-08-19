#define el(str, num, err) \
const char _sys_errlist_##err[] = str;

#include "errlst.h"
