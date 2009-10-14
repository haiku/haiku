/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_ADDRESS_TABLE_COLUMN_H
#define TARGET_ADDRESS_TABLE_COLUMN_H


#include "table/TableColumns.h"


class TargetAddressTableColumn : public StringTableColumn {
public:
								TargetAddressTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;
	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);
};


#endif	// TARGET_ADDRESS_TABLE_COLUMN_H
