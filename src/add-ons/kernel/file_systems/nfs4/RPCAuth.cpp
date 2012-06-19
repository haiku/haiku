/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCAuth.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <SupportDefs.h>
#include <util/kernel_cpp.h>


using namespace RPC;


Auth::Auth()
{
}


const Auth*
Auth::CreateNone()
{
	Auth* auth = new(std::nothrow) Auth;
	if (auth == NULL)
		return NULL;

	auth->fStream.AddInt(AUTH_NONE);
	auth->fStream.AddOpaque(NULL, 0);
	if (auth->fStream.Error() != B_OK) {
		delete auth;
		return NULL;
	}

	return auth;
}


const Auth*
Auth::CreateSys()
{
	Auth* auth = new(std::nothrow) Auth;
	if (auth == NULL)
		return NULL;

	XDR::WriteStream xdr;
	xdr.AddUInt(time(NULL));

	char hostname[255];
	if (gethostname(hostname, 255) < 0)
		strcpy(hostname, "unknown");
	xdr.AddString(hostname, 255);

	xdr.AddUInt(getuid());
	xdr.AddUInt(getgid());

	int count = getgroups(0, NULL);
	gid_t* groups = (gid_t*)malloc(count * sizeof(gid_t));
	int len = getgroups(count, groups);
	if (len > 0) {
		len = min_c(len, 16);
		xdr.AddUInt(len);
		for (int i = 0; i < len; i++)
			xdr.AddUInt((uint32)groups[i]);
	} else
		xdr.AddUInt(0);
	free(groups);
	if (xdr.Error() != B_OK) {
		delete auth;
		return NULL;
	}

	auth->fStream.AddInt(AUTH_SYS);
	auth->fStream.AddOpaque(xdr);
	if (auth->fStream.Error() != B_OK) {
		delete auth;
		return NULL;
	}

	return auth;
}

