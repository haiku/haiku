/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BITMAP_PRIVATE_H
#define _BITMAP_PRIVATE_H


#include <Bitmap.h>
#include <OS.h>


// This structure is placed in the client/server shared memory area.

struct overlay_client_data {
	sem_id	lock;
	uint8*	buffer;
};


void reconnect_bitmaps_to_app_server();


class BBitmap::Private {
public:
								Private(BBitmap* bitmap);
			void				ReconnectToAppServer();
private:
			BBitmap*			fBitmap;
};

#endif // _BITMAP_PRIVATE_H
