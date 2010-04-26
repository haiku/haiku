/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESROSTER_H
#define RESROSTER_H

#include <List.h>
#include "ColumnTypes.h"
#include "ColumnListView.h"

class Editor;
class ResourceData;

class ResourceRoster
{
public:
					ResourceRoster(void);
					~ResourceRoster(void);
	BField *		MakeFieldForType(const int32 &type, const char *data,
									const size_t &length);
	void			SpawnEditor(ResourceData *data, BHandler *handler);
	
private:
	void			LoadEditors(void);
	
	BList			fList;
};

typedef Editor* create_editor(void);

#endif
