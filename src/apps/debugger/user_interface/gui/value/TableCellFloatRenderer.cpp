/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellFloatRenderer.h"

#include <stdio.h>

#include "FloatValue.h"
#include "TableCellValueRendererUtils.h"


void
TableCellFloatRenderer::RenderValue(Value* _value, bool valueChanged,
	BRect rect, BView* targetView)
{
	FloatValue* value = dynamic_cast<FloatValue*>(_value);
	if (value == NULL)
		return;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%g", value->GetValue());

	TableCellValueRendererUtils::DrawString(targetView, rect, buffer,
		valueChanged, B_ALIGN_RIGHT, true);
}


float
TableCellFloatRenderer::PreferredValueWidth(Value* _value, BView* targetView)
{
	FloatValue* value = dynamic_cast<FloatValue*>(_value);
	if (value == NULL)
		return 0;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%g", value->GetValue());

	return TableCellValueRendererUtils::PreferredStringWidth(targetView,
		buffer);
}
