/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (c) 1996,1999 by Internet Software Consortium.
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

#include "port_before.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <irs.h>
#include <isc/memcluster.h>

#include "port_after.h"

#include "irs_p.h"
#include "lcl_p.h"

#define MAXALIASES      35

struct private_data {
	struct protoent	proto;
	char *proto_aliases[MAXALIASES];
	int index;
};

// TODO: for now, this should later be exported dynamically by the stack 
const struct protocol {
	const char *name;
	const char *alias;
	int number;
} kProtocols[] =  {
	{"ip", "IP", 0},		// internet protocol, pseudo protocol number
	{"icmp", "ICMP", 1},	// internet control message protocol
	{"tcp", "TCP", 6},		// transmission control protocol
	{"udp", "UDP", 17},		// user datagram protocol"
};
const size_t kProtocolsCount = sizeof(kProtocols) / sizeof(kProtocols[0]);


static void
pr_close(struct irs_pr *this)
{
	struct private_data *data = (struct private_data *)this->private;

	memput(data, sizeof *data);
	memput(this, sizeof *this);
}


static void
pr_rewind(struct irs_pr *this)
{
	struct private_data *data = (struct private_data *)this->private;
	data->index = 0;
}


static struct protoent *
pr_next(struct irs_pr *this)
{
	struct private_data *data = (struct private_data *)this->private;
	struct protoent *entry = &data->proto;
	size_t i = data->index++;
	if (i >= kProtocolsCount)
		return NULL;

	data->proto_aliases[0] = (char *)kProtocols[i].alias;
	data->proto_aliases[1] = NULL;

	entry->p_proto = kProtocols[i].number;
	entry->p_name = (char *)kProtocols[i].name;
	entry->p_aliases = data->proto_aliases;

	return entry;
}


static struct protoent *
pr_byname(struct irs_pr *this, const char *name)
{
	struct protoent *p;
	char **cp;

	pr_rewind(this);
	while ((p = pr_next(this))) {
		if (!strcmp(p->p_name, name))
			return p;
		for (cp = p->p_aliases; *cp; cp++) {
			if (!strcmp(*cp, name))
				return p;
		}
	}

	return NULL;
}


static struct protoent *
pr_bynumber(struct irs_pr *this, int proto)
{
	struct protoent *p;

	pr_rewind(this);
	while ((p = pr_next(this))) {
		if (p->p_proto == proto)
			break;
	}

	return p;
}


static void
pr_minimize(struct irs_pr *this)
{
}


//	#pragma mark -


struct irs_pr *
irs_lcl_pr(struct irs_acc *this)
{
	struct irs_pr *pr;
	struct private_data *data;

	if (!(pr = memget(sizeof *pr))) {
		errno = ENOMEM;
		return (NULL);
	}
	if (!(data = memget(sizeof *data))) {
		memput(pr, sizeof *this);
		errno = ENOMEM;
		return (NULL);
	}
	memset(data, 0, sizeof *data);

	pr->private = data;
	pr->close = pr_close;
	pr->byname = pr_byname;
	pr->bynumber = pr_bynumber;
	pr->next = pr_next;
	pr->rewind = pr_rewind;
	pr->minimize = pr_minimize;
	pr->res_get = NULL;
	pr->res_set = NULL;
	return pr;
}

