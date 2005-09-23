/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Directory.h>
#include <View.h>
#include "PackageViews.h"

Package::Package()
	: fName(NULL),
	fGroup(NULL),
	fDescription(NULL),
	fSize(0),
	fIcon(NULL)
{

}


Package::~Package()
{
	free(fName);
	free(fGroup);
	free(fDescription);
	free(fIcon);	
}


Package *
Package::PackageFromEntry(BEntry &entry)
{
	BDirectory directory(&entry);
	if (directory.InitCheck()!=B_OK)
		return NULL;
	Package *package = new Package();
	return package;
}


Group::Group()
{

}

Group::~Group()
{
}


PackageCheckBox::PackageCheckBox(BRect rect, Package &item) 
	: BCheckBox(rect, "pack_cb", item.Name(), NULL),
	fPackage(item)
{
}


PackageCheckBox::~PackageCheckBox()
{
}


void 
PackageCheckBox::Draw(BRect update)
{
	BCheckBox::Draw(update);
}


GroupView::GroupView(BRect rect, Group &group)
	: BStringView(rect, "group", group.Name()),
	fGroup(group)
{

}

GroupView::~GroupView()
{
}


PackagesView::PackagesView(BRect rect, const char* name)
	: BView(rect, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FRAME_EVENTS)
{

}


PackagesView::~PackagesView()
{

}


void 
PackagesView::AddPackage(Package &package)
{
	BRect rect = Bounds();
	rect.top = rect.bottom;
	rect.bottom += 15;
	PackageCheckBox *checkBox = new PackageCheckBox(rect, package);
	AddChild(checkBox);
	Invalidate();
}


void 
PackagesView::AddGroup(Group &group)
{
	BRect rect = Bounds();
	rect.top = rect.bottom;
	rect.bottom += 15;
	GroupView *view = new GroupView(rect, group);
	AddChild(view);
	Invalidate();
}


void
PackagesView::Clean()
{

}

