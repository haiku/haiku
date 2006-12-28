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
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
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

#include "irs_p.h"
#include "lcl_p.h"

#include <isc/memcluster.h>

#include "port_after.h"

#include <File.h>
#include <Resources.h>
#include <TypeConstants.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define IRS_SV_MAXALIASES	35

struct service_private {
	FILE*	file;
	char	line[BUFSIZ+1];
	char*	resource_file;
	char*	resource_end;
	char*	resource_line;
	struct servent servent;
	char*	aliases[IRS_SV_MAXALIASES];
};


static status_t
find_own_image(image_info* _info)
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (((uint32)info.text <= (uint32)find_own_image
			&& (uint32)info.text + (uint32)info.text_size > (uint32)find_own_image)) {
			// found us
			*_info = info;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


char *
get_next_line(struct service_private* service)
{
	if (service->file != NULL)
		return fgets(service->line, BUFSIZ, service->file);
	if (service->resource_file == NULL)
		return NULL;

	char* line = service->resource_line;
	while (line < service->resource_end
		&& line[0]
		&& line[0] != '\n')
		line++;

	if (service->resource_line >= service->resource_end
		|| service->resource_line == line)
		return NULL;

	strlcpy(service->line, service->resource_line,
		min_c(line + 1 - service->resource_line, BUFSIZ));
	service->resource_line = line + 1;

	return service->line;
}


//	#pragma mark -


static void
sv_close(struct irs_sv *sv)
{
	struct service_private *service = (struct service_private *)sv->private_data;

	if (service->file)
		fclose(service->file);

	free(service->resource_file);
	memput(service, sizeof *service);
	memput(sv, sizeof *sv);
}


static void
sv_rewind(struct irs_sv *sv)
{
	struct service_private *service = (struct service_private *)sv->private_data;

	if (service->file) {
		if (fseek(service->file, 0L, SEEK_SET) == 0)
			return;
		fclose(service->file);
		service->file = NULL;
	}

	if ((service->file = fopen(_PATH_SERVICES, "r")) != NULL) {
		if (fcntl(fileno(service->file), F_SETFD, 1) == 0)
			return;

		fclose(service->file);
		service->file = NULL;
	}

	// opening the standard file has file has failed, use resources

	if (service->resource_file != NULL) {
		service->resource_line = service->resource_file;
		return;
	}

	image_info info;
	if (find_own_image(&info) < B_OK)
		return;

	BFile file;
	if (file.SetTo(info.name, B_READ_ONLY) < B_OK)
		return;

	BResources resources(&file);
	if (resources.InitCheck() < B_OK)
		return;

	size_t size;
	const void* data = resources.LoadResource(B_STRING_TYPE, "services", &size);
	if (data == NULL)
		return;

	service->resource_file = (char *)malloc(size);
	if (service->resource_file == NULL)
		return;

	memcpy(service->resource_file, data, size);
	service->resource_line = service->resource_file;
	service->resource_end = service->resource_file + size;
}


static struct servent *
sv_next(struct irs_sv *sv)
{
	struct service_private *service = (struct service_private *)sv->private_data;
	char *p, *cp, **q;

	if (service->file == NULL && service->resource_file == NULL)
		sv_rewind(sv);

	if (service->file == NULL && service->resource_file == NULL)
		return NULL;

again:
	if ((p = get_next_line(service)) == NULL)
		return NULL;

	if (*p == '#')
		goto again;

	service->servent.s_name = p;
	while (*p && *p != '\n' && *p != ' ' && *p != '\t' && *p != '#')
		++p;
	if (*p == '\0' || *p == '#' || *p == '\n')
		goto again;
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\0' || *p == '#' || *p == '\n')
		goto again;
	service->servent.s_port = htons((u_short)strtol(p, &cp, 10));
	if (cp == p || (*cp != '/' && *cp != ','))
		goto again;
	p = cp + 1;
	service->servent.s_proto = p;

	q = service->servent.s_aliases = service->aliases;

	while (*p && *p != '\n' && *p != ' ' && *p != '\t' && *p != '#')
		++p;

	while (*p == ' ' || *p == '\t') {
		*p++ = '\0';
		while (*p == ' ' || *p == '\t')
			++p;
		if (*p == '\0' || *p == '#' || *p == '\n')
			break;
		if (q < &service->aliases[IRS_SV_MAXALIASES - 1])
			*q++ = p;
		while (*p && *p != '\n' && *p != ' ' && *p != '\t' && *p != '#')
			++p;
	}

	*p = '\0';
	*q = NULL;
	return &service->servent;
}


static struct servent *
sv_byname(struct irs_sv *sv, const char *name, const char *protocol)
{
	struct servent *servent;

	sv_rewind(sv);

	while ((servent = sv_next(sv))) {
		char **alias;

		if (!strcmp(name, servent->s_name))
			goto gotname;
		for (alias = servent->s_aliases; *alias; alias++) {
			if (!strcmp(name, *alias))
				goto gotname;
		}
		continue;
gotname:
		if (protocol == NULL || strcmp(servent->s_proto, protocol) == 0)
			break;
	}
	return servent;
}


static struct servent *
sv_byport(struct irs_sv *sv, int port, const char *protocol)
{
	struct servent *servent;

	sv_rewind(sv);
	while ((servent = sv_next(sv))) {
		if (servent->s_port != port)
			continue;
		if (protocol == NULL || strcmp(servent->s_proto, protocol) == 0)
			break;
	}
	return servent;
}


static void
sv_minimize(struct irs_sv *sv)
{
	struct service_private *service = (struct service_private *)sv->private_data;

	if (service->file != NULL) {
		fclose(service->file);
		service->file = NULL;
	}
}


//	#pragma mark -


extern "C" struct irs_sv *
irs_lcl_sv(struct irs_acc */*acc*/)
{
	struct service_private *service;
	struct irs_sv *sv;

	if ((sv = (irs_sv *)memget(sizeof *sv)) == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if ((service = (service_private *)memget(sizeof *service)) == NULL) {
		memput(sv, sizeof *sv);
		errno = ENOMEM;
		return NULL;
	}

	memset(service, 0, sizeof *service);

	sv->private_data = service;
	sv->close = sv_close;
	sv->next = sv_next;
	sv->byname = sv_byname;
	sv->byport = sv_byport;
	sv->rewind = sv_rewind;
	sv->minimize = sv_minimize;
	sv->res_get = NULL;
	sv->res_set = NULL;

	return sv;
}
