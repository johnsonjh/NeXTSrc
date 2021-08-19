/* Copyright 1988 NeXT, Inc. - CCH */

/*
 * An ANSI C compatible implementation of the clock() function.
 * The ANSI specification is mute on some important issues:
 * The specification says that it measures "processor time",
 * but is unclear as to whether this includes both system and
 * user time and as to whether this includes child process time.
 * We assume that system, user, self, and child times are all summed.
 */

#include <time.h>

#include <sys/time.h>
#include <sys/resource.h>

static clock_t s(register struct timeval *tvp) {

	return (tvp->tv_sec * CLK_TCK + tvp->tv_usec / (1000000/CLK_TCK));
}

#undef clock
clock_t
clock(void) {
	struct rusage ru;
	register clock_t result;

	if (getrusage(RUSAGE_SELF, &ru) < 0)
		return (clock_t)(-1);
	result = s(&ru.ru_utime) + s(&ru.ru_stime);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		return (clock_t)(-1);
	result += s(&ru.ru_utime) + s(&ru.ru_stime);
	return result;
}
