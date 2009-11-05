/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_INTEGER_RENDERER_H
#define TABLE_CELL_INTEGER_RENDERER_H


#include <Referenceable.h>

#include "IntegerFormatter.h"
#include "TableCellValueRenderer.h"


class TableCellIntegerRenderer : public TableCellValueRenderer {
public:
	class Config;

public:
								TableCellIntegerRenderer(Config* config);
	virtual						~TableCellIntegerRenderer();

			Config*				GetConfig() const
									{ return fConfig; }

	virtual	Settings*			GetSettings() const;

	virtual	void				RenderValue(Value* value, BRect rect,
									BView* targetView);
	virtual	float				PreferredValueWidth(Value* value,
									BView* targetView);

private:
			Config*				fConfig;
};


class TableCellIntegerRenderer::Config : public BReferenceable {
public:
	virtual						~Config();

	virtual	Settings*			GetSettings() const = 0;
	virtual	integer_format		IntegerFormat() const = 0;
};


#endif	// TABLE_CELL_INTEGER_RENDERER_H
