/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_VALUE_RENDERER_UTILS_H
#define TABLE_CELL_VALUE_RENDERER_UTILS_H


#include <InterfaceDefs.h>
#include <Rect.h>


class BView;


class TableCellValueRendererUtils {
public:
	static	void				DrawString(BView* view, BRect rect,
									const char* string,
									enum alignment alignment,
									bool truncate = false);
	static	float				PreferredStringWidth(BView* view,
									const char* string);
};


#endif	// TABLE_CELL_VALUE_RENDERER_UTILS_H
