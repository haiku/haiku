/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */

/**	Server-side counterpart to BPicture */


#include "ServerPicture.h"
#include "ServerTokenSpace.h"


ServerPicture::ServerPicture()
{
	fToken = gTokenSpace.NewToken(B_SERVER_TOKEN, this);

	int8 *ptr;
	fArea = create_area("ServerPicture", (void**)&ptr, B_ANY_ADDRESS,
		B_PAGE_SIZE, B_NO_LOCK,B_READ_AREA | B_WRITE_AREA);
}


ServerPicture::~ServerPicture()
{
}
