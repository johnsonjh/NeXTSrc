#include <time.h>

#undef difftime
double
difftime(time_t time1, time_t time0) {
    return ((double)time1 - (double)time0);
}
