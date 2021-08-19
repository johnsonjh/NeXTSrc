/*
 * NeXT-specific C Threads definitions.
 */

/*
 * Spin locks.
 */
#define	spin_unlock(p)		(*(p) = 0)

/*
 * Mutex locks.
 */
#define	mutex_unlock(m)		((m)->lock = 0)
