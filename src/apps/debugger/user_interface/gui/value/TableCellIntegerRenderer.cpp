/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellIntegerRenderer.h"

#include <stdio.h>

#include <TypeConstants.h>

#include "TableCellValueRendererUtils.h"
#include "IntegerValue.h"


// #pragma mark - TableCellIntegerRenderer


TableCellIntegerRenderer::TableCellIntegerRenderer(Config* config)
	:
	fConfig(config)
{
	if (fConfig != NULL)
		fConfig->AcquireReference();
}


TableCellIntegerRenderer::~TableCellIntegerRenderer()
{
	if (fConfig != NULL)
		fConfig->ReleaseReference();
}


Settings*
TableCellIntegerRenderer::GetSettings() const
{
	return fConfig != NULL ? fConfig->GetSettings() : NULL;
}


void
TableCellIntegerRenderer::RenderValue(Value* _value, BRect rect,
	BView* targetView)
{
	IntegerValue* value = dynamic_cast<IntegerValue*>(_value);
	if (value == NULL)
		return;

	// format the value
	integer_format format = fConfig != NULL
		? fConfig->IntegerFormat() : INTEGER_FORMAT_DEFAULT;
	char buffer[32];
	if (!IntegerFormatter::FormatValue(value->GetValue(), format,  buffer,
			sizeof(buffer))) {
		return;
	}

	// render
	TableCellValueRendererUtils::DrawString(targetView, rect, buffer,
		B_ALIGN_RIGHT, true);
}


float
TableCellIntegerRenderer::PreferredValueWidth(Value* _value, BView* targetView)
{
	IntegerValue* value = dynamic_cast<IntegerValue*>(_value);
	if (value == NULL)
		return 0;

	// format the value
	integer_format format = fConfig != NULL
		? fConfig->IntegerFormat() : INTEGER_FORMAT_DEFAULT;
	char buffer[32];
	if (!IntegerFormatter::FormatValue(value->GetValue(), format,  buffer,
			sizeof(buffer))) {
		return 0;
	}

	// render
	return TableCellValueRendererUtils::PreferredStringWidth(targetView,
		buffer);
}


// #pragma mark - Config


TableCellIntegerRenderer::Config::~Config()
{
}
