/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1998-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>


#define ALIGN(p) (((uintptr_t)(p) + (sizeof(long) - 1)) & ~(sizeof(long) - 1))


struct hostent *gethostbyname_r(const char *, struct hostent *,
                                       char *, int, int *);

struct hostent *
gethostbyaddr_r(const char *addr, int len, int type,
	struct hostent *hptr, char *buf, int buflen, int *h_errnop);


static struct hostent *
copy_hostent(struct hostent *he, struct hostent *hptr, char *buf, int buflen) {
	char *cp;
	char **ptr;
	int i, n;
	int nptr, len;

	/* Find out the amount of space required to store the answer. */
	nptr = 2; /*%< NULL ptrs */
	len = (char *)ALIGN(buf) - buf;
	for (i = 0; he->h_addr_list[i]; i++, nptr++) {
		len += he->h_length;
	}
	for (i = 0; he->h_aliases[i]; i++, nptr++) {
		len += strlen(he->h_aliases[i]) + 1;
	}
	len += strlen(he->h_name) + 1;
	len += nptr * sizeof(char*);

	if (len > buflen) {
		errno = ERANGE;
		return 0;
	}

	/* copy address size and type */
	hptr->h_addrtype = he->h_addrtype;
	n = hptr->h_length = he->h_length;

	ptr = (char **)ALIGN(buf);
	cp = (char *)ALIGN(buf) + nptr * sizeof(char *);

	/* copy address list */
	hptr->h_addr_list = ptr;
	for (i = 0; he->h_addr_list[i]; i++ , ptr++) {
		memcpy(cp, he->h_addr_list[i], n);
		hptr->h_addr_list[i] = cp;
		cp += n;
	}
	hptr->h_addr_list[i] = NULL;
	ptr++;

	/* copy official name */
	n = strlen(he->h_name) + 1;
	strcpy(cp, he->h_name);
	hptr->h_name = cp;
	cp += n;

	/* copy aliases */
	hptr->h_aliases = ptr;
	for (i = 0 ; he->h_aliases[i]; i++) {
		n = strlen(he->h_aliases[i]) + 1;
		strcpy(cp, he->h_aliases[i]);
		hptr->h_aliases[i] = cp;
		cp += n;
	}
	hptr->h_aliases[i] = NULL;

	return hptr;
}


struct hostent *
gethostbyname_r(const char *name,  struct hostent *hptr,
	char *buf, int buflen, int *h_errnop)
{
	struct hostent *he = gethostbyname(name);

	*h_errnop = h_errno;

	if (he == NULL)
		return NULL;

	return copy_hostent(he, hptr, buf, buflen);
}


struct hostent *
gethostbyaddr_r(const char *addr, int len, int type,
	struct hostent *hptr, char *buf, int buflen, int *h_errnop)
{
	struct hostent *he = gethostbyaddr(addr, len, type);

	*h_errnop = h_errno;

    if (he == NULL)
        return NULL;

    return copy_hostent(he, hptr, buf, buflen);
}

