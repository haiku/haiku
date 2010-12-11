/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellStringRenderer.h"

#include <stdio.h>

#include <String.h>

#include "TableCellValueRendererUtils.h"
#include "Value.h"


void
TableCellStringRenderer::RenderValue(Value* value, BRect rect,
	BView* targetView)
{
	BString string = "\"";
	BString tempString;
	if (!value->ToString(tempString))
		return;
	for (int32 i = 0; i < tempString.Length(); i++) {
		if (tempString[i] < 31) {
			switch (tempString[i]) {
				case '\0':
					string << "\\0";
					break;
				case '\a':
					string << "\\a";
					break;
				case '\b':
					string << "\\b";
					break;
				case '\t':
					string << "\\t";
					break;
				case '\r':
					string << "\\r";
					break;
				case '\n':
					string << "\\n";
					break;
				case '\f':
					string << "\\f";
					break;
				default:
				{
					char buffer[5];
					snprintf(buffer, sizeof(buffer), "\\x%x",
						tempString.String()[i]);
					string << buffer;
					break;
				}
			}
		} else if (tempString[i] == '\"')
			string << "\\\"";
		else
			string << tempString[i];
	}

	string += "\"";

	TableCellValueRendererUtils::DrawString(targetView, rect, string,
		B_ALIGN_RIGHT, true);
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
