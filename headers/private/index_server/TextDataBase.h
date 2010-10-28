/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef TEXT_DATA_BASE_H
#define TEXT_DATA_BASE_H


#include <Entry.h>


class TextWriteDataBase {
public:
	virtual						~TextWriteDataBase() {}

	virtual	status_t			InitCheck() = 0;

	virtual	status_t			AddDocument(const entry_ref& ref) = 0;
	virtual	status_t			RemoveDocument(const entry_ref& ref) = 0;
	virtual	status_t			Commit() = 0; 
};


#endif // TEXT_DATA_BASE_H