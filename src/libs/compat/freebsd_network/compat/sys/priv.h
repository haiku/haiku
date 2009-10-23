/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_PRIV_H_
#define _FBSD_COMPAT_SYS_PRIV_H_


/*
 * 802.11-related privileges.
 */
#define	PRIV_NET80211_GETKEY	440	/* Query 802.11 keys. */
#define	PRIV_NET80211_MANAGE	441	/* Administer 802.11. */

#define	PRIV_DRIVER		14	/* Low-level driver privilege. */


/*
 * Privilege check interfaces, modeled after historic suser() interfacs, but
 * with the addition of a specific privilege name.  No flags are currently
 * defined for the API.  Historically, flags specified using the real uid
 * instead of the effective uid, and whether or not the check should be
 * allowed in jail.
 */
struct thread;


int	priv_check(struct thread*, int);

#endif /* _FBSD_COMPAT_SYS_PRIV_H_ */
