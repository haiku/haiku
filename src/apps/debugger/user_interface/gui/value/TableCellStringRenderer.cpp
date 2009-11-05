/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellStringRenderer.h"

#include <String.h>

#include "TableCellValueRendererUtils.h"
#include "Value.h"


void
TableCellStringRenderer::RenderValue(Value* value, BRect rect,
	BView* targetView)
{
	BString string;
	if (!value->ToString(string))
		return;

	TableCellValueRendererUtils::DrawString(targetView, rect, string,
		B_ALIGN_LEFT, true);
}


float
TableCellStringRenderer::PreferredValueWidth(Value* value, BView* targetView)
{
	BString string;
	if (!value->ToString(string))
		return 0;

	return TableCellValueRendererUtils::PreferredStringWidth(targetView,
		string);
}
