/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellBoolRenderer.h"

#include "BoolValue.h"
#include "TableCellValueRendererUtils.h"


static inline const char*
bool_value_string(BoolValue* value)
{
	return value->GetValue() ? "true" : "false";
}


void
TableCellBoolRenderer::RenderValue(Value* _value, BRect rect, BView* targetView)
{
	BoolValue* value = dynamic_cast<BoolValue*>(_value);
	if (value == NULL)
		return;

	TableCellValueRendererUtils::DrawString(targetView, rect,
		bool_value_string(value), B_ALIGN_RIGHT, true);
}


float
TableCellBoolRenderer::PreferredValueWidth(Value* _value, BView* targetView)
{
	BoolValue* value = dynamic_cast<BoolValue*>(_value);
	if (value == NULL)
		return 0;

	return TableCellValueRendererUtils::PreferredStringWidth(targetView,
		bool_value_string(value));
}
