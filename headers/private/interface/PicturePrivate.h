/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PICTURE_PRIVATE_H
#define _PICTURE_PRIVATE_H


#include <Picture.h>
#include <OS.h>


void reconnect_pictures_to_app_server();


class BPicture::Private {
public:
								Private(BPicture* picture);
			void				ReconnectToAppServer();
private:
			BPicture*			fPicture;
};


#endif // _PICTURE_PRIVATE_H
