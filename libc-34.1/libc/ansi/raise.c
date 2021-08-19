#include <signal.h>

#undef raise
int
raise(int sig) {
    kill(getpid(), sig);
}
