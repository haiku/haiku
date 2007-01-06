/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef EDITOR_H
#define EDITOR_H

class Editor
{
public:
						Editor(void);
	virtual				~Editor(void);
	virtual	BView *		GetConfigView(void);
	virtual void		LoadData(unsigned char *data, const size_t &length);
	virtual unsigned char *	SaveData(size_t *data);
};

#endif
