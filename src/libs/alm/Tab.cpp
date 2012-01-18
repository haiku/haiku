/*
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <Tab.h>


#include <ALMLayout.h>


using std::nothrow;


struct XTab::BALMLayoutList {
	BALMLayoutList(BALMLayout* _layout, BALMLayoutList* _next = NULL)
		:
		next(_next),
		layout(_layout)
	{
	}

	~BALMLayoutList()
	{
		delete next;
	}

	bool HasLayout(BALMLayout* search)
	{
		if (layout == search)
			return true;
		return next ? next->HasLayout(search) : false;
	}

	BALMLayoutList* Remove(BALMLayout* remove)
	{
		if (layout == remove) {
			BALMLayoutList* _next = next;
			delete this;
			return _next;
		}
		if (next)
			next = next->Remove(remove);
		return this;
	}

	BALMLayoutList*		next;
	BALMLayout*			layout;
};


XTab::XTab(BALMLayout* layout)
	:
	Variable(layout->Solver()),
	fLayouts(new BALMLayoutList(layout))
{
}


XTab::~XTab()
{
	BALMLayoutList* layouts = fLayouts;
	while (layouts) {
		layouts->layout->fXTabList.RemoveItem(this);
		layouts = layouts->next;
	}
	delete fLayouts;
}


bool
XTab::IsInLayout(BALMLayout* layout)
{
	return fLayouts->HasLayout(layout);
}


bool
XTab::AddedToLayout(BALMLayout* layout)
{
	BALMLayoutList* newHead = new (nothrow) BALMLayoutList(layout, fLayouts);
	if (newHead == NULL)
		return false;
	fLayouts = newHead;
	return true;
}


void
XTab::LayoutLeaving(BALMLayout* layout)
{
	fLayouts = fLayouts->Remove(layout);
}


bool
XTab::IsSuitableFor(BALMLayout* layout)
{
	return (fLayouts->layout->Solver() == layout->Solver());
}


YTab::YTab(BALMLayout* layout)
	:
	Variable(layout->Solver()),
	fLayouts(new XTab::BALMLayoutList(layout))
{
}


YTab::~YTab()
{
	XTab::BALMLayoutList* layouts = fLayouts;
	while (layouts) {
		layouts->layout->fYTabList.RemoveItem(this);
		layouts = layouts->next;
	}
	delete fLayouts;
}


bool
YTab::IsInLayout(BALMLayout* layout)
{
	return fLayouts->HasLayout(layout);
}


bool
YTab::AddedToLayout(BALMLayout* layout)
{
	XTab::BALMLayoutList* newHead
		= new (nothrow) XTab::BALMLayoutList(layout, fLayouts);
	if (newHead == NULL)
		return false;
	fLayouts = newHead;
	return true;
}


void
YTab::LayoutLeaving(BALMLayout* layout)
{
	fLayouts = fLayouts->Remove(layout);
}


bool
YTab::IsSuitableFor(BALMLayout* layout)
{
	return (fLayouts->layout->Solver() == layout->Solver());
}
