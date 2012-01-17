/*
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#include <Tab.h>


#include <ALMLayout.h>


XTab::XTab(BALMLayout* layout)
	:
	Variable(layout->Solver()),
	fALMLayout(layout)
{
}


XTab::~XTab()
{
	fALMLayout->fXTabList.RemoveItem(this);
}


YTab::YTab(BALMLayout* layout)
	:
	Variable(layout->Solver()),
	fALMLayout(layout)
{
}


YTab::~YTab()
{
	fALMLayout->fYTabList.RemoveItem(this);
}
