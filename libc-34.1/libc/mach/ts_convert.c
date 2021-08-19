/*
 *	ts_convert.c -- convert machine dependent timestamps to
 *		machine independent timevals.
 *
 * HISTORY
 * 13-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Timestamp format uses both high and low values for microseconds.  Conversion
 *	to secs/usecs uses GNU long long support.
 */

#include <sys/time.h>
#include <sys/time_stamp.h>

convert_ts_to_tv(ts_format,tsp,tvp)
int	ts_format;
struct tsval *tsp;
struct timeval *tvp;
{
	switch(ts_format) {
		case TS_FORMAT_DEFAULT:
			/*
			 *	High value is tick count at 100 Hz
			 */
			tvp->tv_sec = tsp->high_val/100;
			tvp->tv_usec = (tsp->high_val % 100) * 10000;
			break;
		case TS_FORMAT_MMAX:
			/*
			 *	Low value is usec.
			 */
			tvp->tv_sec = tsp->low_val/1000000;
			tvp->tv_usec = tsp->low_val % 1000000;
			break;
#if	TS_FORMAT_NeXT != TS_FORMAT_MMAX
		case TS_FORMAT_NeXT: {
			/*
			 * Timestamp is in a 64 bit value.  Get the number
			 * of seconds by dividing this value by 1000000.
			 */
			
			unsigned long long usec = *(unsigned long long *)tsp;
			tvp->tv_usec = (long)usec % 1000000;
			tvp->tv_sec = (long)usec / 1000000;
		}
#endif	TS_FORMAT_NeXT != TS_FORMAT_MMAX
		default:
			/*
			 *	Zero output timeval to indicate that
			 *	we can't decode this timestamp.
			 */
			tvp->tv_sec = 0;
			tvp->tv_usec = 0;
			break;
	 }
}
