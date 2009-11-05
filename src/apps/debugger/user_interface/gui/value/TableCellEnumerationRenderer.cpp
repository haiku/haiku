/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellEnumerationRenderer.h"

#include "EnumerationValue.h"
#include "TableCellValueRendererUtils.h"
#include "Type.h"


TableCellEnumerationRenderer::TableCellEnumerationRenderer(Config* config)
	:
	TableCellIntegerRenderer(config)
{
}


void
TableCellEnumerationRenderer::RenderValue(Value* _value, BRect rect,
	BView* targetView)
{
	Config* config = GetConfig();
	if (config != NULL && config->IntegerFormat() == INTEGER_FORMAT_DEFAULT) {
		EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
		if (value == NULL)
			return;

		if (EnumeratorValue* enumValue
				= value->GetType()->ValueFor(value->GetValue())) {
			TableCellValueRendererUtils::DrawString(targetView, rect,
				enumValue->Name(), B_ALIGN_RIGHT, true);
			return;
		}
	}

	TableCellIntegerRenderer::RenderValue(_value, rect, targetView);
}


float
TableCellEnumerationRenderer::PreferredValueWidth(Value* _value,
	BView* targetView)
{
	Config* config = GetConfig();
	if (config != NULL && config->IntegerFormat() == INTEGER_FORMAT_DEFAULT) {
		EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
		if (value == NULL)
			return 0;

		if (EnumeratorValue* enumValue
				= value->GetType()->ValueFor(value->GetValue())) {
			return TableCellValueRendererUtils::PreferredStringWidth(targetView,
				enumValue->Name());
		}
	}

	return TableCellIntegerRenderer::PreferredValueWidth(_value, targetView);
}
