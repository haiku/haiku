/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "AbstractGeneralPage.h"

#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>


AbstractGeneralPage::AbstractGeneralPage()
	:
	BGroupView(B_VERTICAL),
	fDataView(NULL)
{
	SetName("General");

	GroupLayout()->SetInsets(10, 10, 10, 10);

	BGroupLayoutBuilder(this)
		.Add(fDataView = new BGridView(10, 5))
		.AddGlue()
	;
}


AbstractGeneralPage::~AbstractGeneralPage()
{
}


/*!	Throws std::bad_alloc.
 */
TextDataView*
AbstractGeneralPage::AddDataView(const char* label, const char* text)
{
	BGridLayout* layout = fDataView->GridLayout();
	int32 row = layout->CountRows();
	layout->AddView(new LabelView(label), 0, row);

	TextDataView* dataView = new TextDataView(text);
	layout->AddView(dataView, 1, row);

	return dataView;
}
