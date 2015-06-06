/*
 * Copyright 2014-2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellFormattedValueRenderer.h"

#include <String.h>

#include "TableCellValueRendererUtils.h"
#include "ValueFormatter.h"


TableCellFormattedValueRenderer::TableCellFormattedValueRenderer(
	ValueFormatter* formatter)
	:
	fValueFormatter(formatter)
{
	fValueFormatter->AcquireReference();
}


TableCellFormattedValueRenderer::~TableCellFormattedValueRenderer()
{
	fValueFormatter->ReleaseReference();
}


Settings*
TableCellFormattedValueRenderer::GetSettings() const
{
	return fValueFormatter->GetSettings();
}


void
TableCellFormattedValueRenderer::RenderValue(Value* value, bool valueChanged,
	BRect rect, BView* targetView)
{
	BString output;

	if (fValueFormatter->FormatValue(value, output) != B_OK)
		return;

	// render
	TableCellValueRendererUtils::DrawString(targetView, rect, output,
		valueChanged, B_ALIGN_RIGHT, true);
}


float
TableCellFormattedValueRenderer::PreferredValueWidth(Value* value, BView* targetView)
{
	BString output;

	if (fValueFormatter->FormatValue(value, output) != B_OK)
		return 0;

	// render
	return TableCellValueRendererUtils::PreferredStringWidth(targetView,
		output);
}
