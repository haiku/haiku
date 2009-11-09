/*
 * Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static const char rcsid[] = "$Id: ns_newmsg.c,v 1.3 2009/02/26 10:48:57 marka Exp $";
#endif

#include <arpa/nameser.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

static int	rdcpy(ns_newmsg *, ns_type, const u_char *, size_t);

/* Initialize a "newmsg" object to empty.
 */
int
ns_newmsg_init(u_char *buffer, size_t bufsiz, ns_newmsg *handle) {
	ns_msg *msg = &handle->msg;

	memset(handle, 0, sizeof *handle);
	msg->_msg = buffer;
	msg->_eom = buffer + bufsiz;
	msg->_sect = ns_s_qd;
	msg->_rrnum = 0;
	msg->_msg_ptr = buffer + NS_HFIXEDSZ;
	handle->dnptrs[0] = msg->_msg;
	handle->dnptrs[1] = NULL;
	handle->lastdnptr = &handle->dnptrs[sizeof handle->dnptrs /
					    sizeof handle->dnptrs[0] - 1];
	return (0);
}

/* Initialize a "newmsg" object by copying an existing parsed message.
 */
int
ns_newmsg_copy(ns_newmsg *handle, ns_msg *msg) {
	ns_flag flag;
	ns_sect sect;

	ns_newmsg_id(handle, ns_msg_id(*msg));
	for (flag = ns_f_qr; flag < ns_f_max; flag++)
		ns_newmsg_flag(handle, flag, ns_msg_getflag(*msg, flag));
	for (sect = ns_s_qd; sect < ns_s_max; sect++) {
		int i, count;

		count = ns_msg_count(*msg, sect);
		for (i = 0; i < count; i++) {
			ns_rr2 rr;
			int x;

			if (ns_parserr2(msg, sect, i, &rr) < 0)
				return (-1);
			if (sect == ns_s_qd)
				x = ns_newmsg_q(handle,
						ns_rr_nname(rr),
						ns_rr_type(rr),
						ns_rr_class(rr));
			else
				x = ns_newmsg_rr(handle, sect,
						 ns_rr_nname(rr),
						 ns_rr_type(rr),
						 ns_rr_class(rr),
						 ns_rr_ttl(rr),
						 ns_rr_rdlen(rr),
						 ns_rr_rdata(rr));
			if (x < 0)
				return (-1);
		}
	}
	return (0);
}

/* Set the message-ID in a "newmsg" object.
 */
void
ns_newmsg_id(ns_newmsg *handle, u_int16_t id) {
	ns_msg *msg = &handle->msg;

	msg->_id = id;
}

/* Set a flag (including rcode or opcode) in a "newmsg" object.
 */
void
ns_newmsg_flag(ns_newmsg *handle, ns_flag flag, u_int value) {
	extern struct _ns_flagdata _ns_flagdata[16];
	struct _ns_flagdata *fd = &_ns_flagdata[flag];
	ns_msg *msg = &handle->msg;

	assert(flag < ns_f_max);
	msg->_flags &= (~fd->mask);
	msg->_flags |= (value << fd->shift);
}

/* Add a question (or zone, if it's an update) to a "newmsg" object.
 */
int
ns_newmsg_q(ns_newmsg *handle, ns_nname_ct qname,
	    ns_type qtype, ns_class qclass)
{
	ns_msg *msg = &handle->msg;
	u_char *t;
	int n;

	if (msg->_sect != ns_s_qd) {
		errno = ENODEV;
		return (-1);
	}
	t = (u_char *) (unsigned long) msg->_msg_ptr;
	if (msg->_rrnum == 0)
		msg->_sections[ns_s_qd] = t;
	n = ns_name_pack(qname, t, msg->_eom - t,
			 handle->dnptrs, handle->lastdnptr);
	if (n < 0)
		return (-1);
	t += n;
	if (t + QFIXEDSZ >= msg->_eom) {
		errno = EMSGSIZE;
		return (-1);
	}
	NS_PUT16(qtype, t);
	NS_PUT16(qclass, t);
	msg->_msg_ptr = t;
	msg->_counts[ns_s_qd] = ++msg->_rrnum;
	return (0);
}

/* Add an RR to a "newmsg" object.
 */
int
ns_newmsg_rr(ns_newmsg *handle, ns_sect sect,
	     ns_nname_ct name, ns_type type,
	     ns_class rr_class, u_int32_t ttl,
	     u_int16_t rdlen, const u_char *rdata)
{
	ns_msg *msg = &handle->msg;
	u_char *t;
	int n;

	if (sect < msg->_sect) {
		errno = ENODEV;
		return (-1);
	}
	t = (u_char *) (unsigned long) msg->_msg_ptr;
	if (sect > msg->_sect) {
		msg->_sect = sect;
		msg->_sections[sect] = t;
		msg->_rrnum = 0;
	}
	n = ns_name_pack(name, t, msg->_eom - t,
			 handle->dnptrs, handle->lastdnptr);
	if (n < 0)
		return (-1);
	t += n;
	if (t + RRFIXEDSZ + rdlen >= msg->_eom) {
		errno = EMSGSIZE;
		return (-1);
	}
	NS_PUT16(type, t);
	NS_PUT16(rr_class, t);
	NS_PUT32(ttl, t);
	msg->_msg_ptr = t;
	if (rdcpy(handle, type, rdata, rdlen) < 0)
		return (-1);
	msg->_counts[sect] = ++msg->_rrnum;
	return (0);
}

/* Complete a "newmsg" object and return its size for use in write().
 * (Note: the "newmsg" object is also made ready for ns_parserr() etc.)
 */
size_t
ns_newmsg_done(ns_newmsg *handle) {
	ns_msg *msg = &handle->msg;
	ns_sect sect;
	u_char *t;

	t = (u_char *) (unsigned long) msg->_msg;
	NS_PUT16(msg->_id, t);
	NS_PUT16(msg->_flags, t);
	for (sect = 0; sect < ns_s_max; sect++)
		NS_PUT16(msg->_counts[sect], t);
	msg->_eom = msg->_msg_ptr;
	msg->_sect = ns_s_max;
	msg->_rrnum = -1;
	msg->_msg_ptr = NULL;
	return (msg->_eom - msg->_msg);
}

/* Private. */

/* Copy an RDATA, using compression pointers where RFC1035 permits.
 */
static int
rdcpy(ns_newmsg *handle, ns_type type, const u_char *rdata, size_t rdlen) {
	ns_msg *msg = &handle->msg;
	u_char *p = (u_char *) (unsigned long) msg->_msg_ptr;
	u_char *t = p + NS_INT16SZ;
	u_char *s = t;
	int n;

	switch (type) {
	case ns_t_soa:
		/* MNAME. */
		n = ns_name_pack(rdata, t, msg->_eom - t,
				 handle->dnptrs, handle->lastdnptr);
		if (n < 0)
			return (-1);
		t += n;
		if (ns_name_skip(&rdata, msg->_eom) < 0)
			return (-1);

		/* ANAME. */
		n = ns_name_pack(rdata, t, msg->_eom - t,
				 handle->dnptrs, handle->lastdnptr);
		if (n < 0)
			return (-1);
		t += n;
		if (ns_name_skip(&rdata, msg->_eom) < 0)
			return (-1);

		/* Serial, Refresh, Retry, Expiry, and Minimum. */
		if ((msg->_eom - t) < (NS_INT32SZ * 5)) {
			errno = EMSGSIZE;
			return (-1);
		}
		memcpy(t, rdata, NS_INT32SZ * 5);
		t += (NS_INT32SZ * 5);
		break;
	case ns_t_ptr:
	case ns_t_cname:
	case ns_t_ns:
		/* PTRDNAME, CNAME, or NSDNAME. */
		n = ns_name_pack(rdata, t, msg->_eom - t,
				 handle->dnptrs, handle->lastdnptr);
		if (n < 0)
			return (-1);
		t += n;
		break;
	default:
		memcpy(t, rdata, rdlen);
		t += rdlen;
	}
	NS_PUT16(t - s, p);
	msg->_msg_ptr = t;
	return (0);
}

