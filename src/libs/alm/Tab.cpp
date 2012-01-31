/*
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <Tab.h>


#include <ALMLayout.h>


using std::nothrow;
using BALM::TabBase;


struct TabBase::BALMLayoutList {
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


TabBase::TabBase()
	:
	fLayouts(NULL)
{
}


TabBase::TabBase(BMessage* archive)
	:
	BArchivable(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver(archive).Finish(B_OK);
}


TabBase::~TabBase()
{
}


bool
TabBase::IsInLayout(BALMLayout* layout)
{
	return fLayouts ? fLayouts->HasLayout(layout) : false;
}


bool
TabBase::AddedToLayout(BALMLayout* layout)
{
	BALMLayoutList* newHead = new (nothrow) BALMLayoutList(layout, fLayouts);
	if (newHead == NULL)
		return false;
	fLayouts = newHead;
	return true;
}


void
TabBase::LayoutLeaving(BALMLayout* layout)
{
	fLayouts = fLayouts->Remove(layout);
}


bool
TabBase::IsSuitableFor(BALMLayout* layout)
{
	return (fLayouts->layout->Solver() == layout->Solver());
}


XTab::XTab(BALMLayout* layout)
	:
	Variable(layout->Solver())
{
	AddedToLayout(layout);
}


XTab::XTab(BMessage* archive)
	:
	Variable(NULL),
	TabBase(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver(archive).Finish(B_OK);
}


XTab::~XTab()
{
	TabBase::BALMLayoutList* layouts = fLayouts;
	while (layouts) {
		layouts->layout->fXTabList.RemoveItem(this);
		layouts = layouts->next;
	}
	delete fLayouts;
}


BArchivable*
XTab::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BALM::XTab"))
		return new XTab(archive);
	return NULL;
}


YTab::YTab(BALMLayout* layout)
	:
	Variable(layout->Solver())
{
	AddedToLayout(layout);
}


YTab::YTab(BMessage* archive)
	:
	Variable(NULL),
	TabBase(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver(archive).Finish(B_OK);
}


YTab::~YTab()
{
	TabBase::BALMLayoutList* layouts = fLayouts;
	while (layouts) {
		layouts->layout->fYTabList.RemoveItem(this);
		layouts = layouts->next;
	}
	delete fLayouts;
}


BArchivable*
YTab::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BALM::YTab"))
		return new YTab(archive);
	return NULL;
}
