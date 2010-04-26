/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef EDITOR_H
#define EDITOR_H

#include <Window.h>
#include <Handler.h>

class ResourceData;

#define M_UPDATE_RESOURCE 'uprs'

class Editor : public BWindow
{
public:
						Editor(const BRect &frame, ResourceData *data,
								BHandler *owner);
	virtual				~Editor(void);
						
	ResourceData *		GetData(void) const { return fResData; }
	BHandler *			GetOwner(void) const { return fOwner; }
private:
	ResourceData		*fResData;
	BHandler			*fOwner;
};

#endif
