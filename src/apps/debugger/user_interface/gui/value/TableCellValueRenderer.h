/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_VALUE_RENDERER_H
#define TABLE_CELL_VALUE_RENDERER_H


#include <Rect.h>

#include <Referenceable.h>


class BView;
class Settings;
class Value;


class TableCellValueRenderer : public BReferenceable {
public:
	virtual						~TableCellValueRenderer();

	virtual	Settings*			GetSettings() const;
									// returns NULL, if no settings

	virtual	void				RenderValue(Value* value, BRect rect,
									BView* targetView) = 0;
	virtual	float				PreferredValueWidth(Value* value,
									BView* targetView) = 0;
};


#endif	// TABLE_CELL_VALUE_RENDERER_H
