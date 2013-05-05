/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RESOURCE_ROW_H
#define RESOURCE_ROW_H


#include "DefaultTypes.h"

#include <ColumnListView.h>


class ResourceRow : public BRow {
public:
					ResourceRow();
					~ResourceRow();

	void			SetResourceID(int32 id);
	void			SetResourceStringID(const char* id);
	void			SetResourceName(const char* name);
	void			SetResourceType(const char* type);
	void			SetResourceCode(type_code code);
	void			SetResourceStringCode(const char* code);
	void			SetResourceData(const char* data);
	void			SetResourceSize(off_t size);

	int32			ResourceID();
	const char*		ResourceStringID();
	const char*		ResourceName();
	const char*		ResourceType();
	type_code		ResourceCode();
	const char*		ResourceStringCode();
	const char*		ResourceData();
	off_t			ResourceSize();

	ResourceRow*	Parent;
	int32			ActionIndex;
private:
	type_code		fCode;
	char			fTypeString[8];

};


#endif
