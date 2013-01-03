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
	void			SetResourceName(const char* name);
	void			SetResourceType(const char* type);
	void			SetResourceTypeCode(type_code code);
	void			SetResourceData(const char* data);
	void			SetResourceRawData(const void*);
	void			SetResourceSize(off_t size);

	int32			ResourceID();
	const char*		ResourceName();
	const char*		ResourceType();
	type_code		ResourceTypeCode();
	const char*		ResourceData();
	const void*		ResourceRawData();
	off_t			ResourceSize();

private:
	const void*		fRawData;
	type_code		fTypeCode;
	char			fTypeString[8];

};


#endif
