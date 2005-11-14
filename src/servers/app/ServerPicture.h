/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef SERVER_PICTURE_H
#define SERVER_PICTURE_H


#include <OS.h>

class AreaLink;


class ServerPicture {
	public:
		ServerPicture();
		~ServerPicture();

		status_t InitCheck() const { return fArea >= B_OK ? B_OK : fArea; }
		area_id Area() const { return fArea; }
		int32 Token() const { return fToken; }

	private:	
		AreaLink*	fAreaLink;
		area_id		fArea;
		int32		fToken;
};

#endif	/* SERVER_PICTURE_H */
