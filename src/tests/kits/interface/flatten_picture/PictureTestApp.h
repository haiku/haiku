/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _PICTURE_TEST_APP_H
#define _PICTURE_TEST_APP_H

#include <Application.h>
#include <Message.h>
#include <Rect.h>

#define APPLICATION_SIGNATURE	"application/x-vnd.haiku-picturetest"

class PictureTestApp : public BApplication
{
	typedef BApplication Inherited;

public:
	PictureTestApp();

	void ReadyToRun();

private:	
};

#endif
