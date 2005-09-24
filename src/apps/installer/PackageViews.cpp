/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Directory.h>
#include <ScrollBar.h>
#include <String.h>
#include <stdio.h>
#include <View.h>
#include "PackageViews.h"

Package::Package()
	: Group(),
	fName(NULL),
	fDescription(NULL),
	fSize(0),
	fIcon(NULL)
{

}


Package::~Package()
{
	free(fName);
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
	char name[255];
	char group[255];
	char description[255];
	bool alwaysOn;
	bool onByDefault;
	int32 size;
	if (directory.ReadAttr("INSTALLER PACKAGE: NAME", B_STRING_TYPE, 0, name, 255)<0)
		goto err;
	if (directory.ReadAttr("INSTALLER PACKAGE: GROUP", B_STRING_TYPE, 0, group, 255)<0)
		goto err;
	if (directory.ReadAttr("INSTALLER PACKAGE: DESCRIPTION", B_STRING_TYPE, 0, description, 255)<0)
		goto err;
	if (directory.ReadAttr("INSTALLER PACKAGE: ON_BY_DEFAULT", B_BOOL_TYPE, 0, &onByDefault, sizeof(onByDefault))<0)
		goto err;
	if (directory.ReadAttr("INSTALLER PACKAGE: ALWAYS_ON", B_BOOL_TYPE, 0, &alwaysOn, sizeof(alwaysOn))<0)
		goto err;
	if (directory.ReadAttr("INSTALLER PACKAGE: SIZE", B_INT32_TYPE, 0, &size, sizeof(size))<0)
		goto err;
	package->SetName(name);
	package->SetGroupName(group);
	package->SetDescription(description);
	package->SetSize(size);
	package->SetAlwaysOn(alwaysOn);
	package->SetOnByDefault(onByDefault);
	return package;
err:
	delete package;
	return NULL;
}


void 
Package::GetSizeAsString(char *string)
{
	float kb = fSize / 1024.0;
	if (kb < 1.0) {
		sprintf(string, "%ld B", fSize);
		return;
	}
	float mb = kb / 1024.0;
	if (mb < 1.0) {
		sprintf(string, "%3.1f KB", kb);
		return;
	}
	sprintf(string, "%3.1f MB", mb);
	
}


Group::Group()
	: fGroup(NULL)
{

}

Group::~Group()
{
	free(fGroup);
}


PackageCheckBox::PackageCheckBox(BRect rect, Package &item) 
	: BCheckBox(rect.OffsetBySelf(7,0), "pack_cb", item.Name(), NULL),
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
	char string[255];
	fPackage.GetSizeAsString(string);
	float width = StringWidth(string);
	DrawString(string, BPoint(Bounds().right - width - 8, 11));
}

GroupView::GroupView(BRect rect, Group &group)
	: BStringView(rect, "group", group.GroupName()),
	fGroup(group)
{
	SetFont(be_bold_font);
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
PackagesView::Clean()
{
	
}


void
PackagesView::AddPackages(BList &packages)
{
	int32 count = packages.CountItems();
	BRect rect = Bounds();
	BRect bounds = rect;
	rect.left = 1;
	rect.bottom = 15;
	BString lastGroup = "";
	for (int32 i=0; i<count; i++) {
		void *item = packages.ItemAt(i);
		Package *package = static_cast<Package *> (item);
		if (lastGroup != BString(package->GroupName())) {
			rect.OffsetBy(0, 1);
			lastGroup = package->GroupName();
			Group *group = new Group();
			group->SetGroupName(package->GroupName());
			GroupView *view = new GroupView(rect, *group);
			AddChild(view);
			rect.OffsetBy(0, 17);
		}
		PackageCheckBox *checkBox = new PackageCheckBox(rect, *package);
		checkBox->SetValue(package->OnByDefault() ? B_CONTROL_ON : B_CONTROL_OFF);
		checkBox->SetEnabled(!package->AlwaysOn());
		AddChild(checkBox);
		rect.OffsetBy(0, 20);
	}
	ResizeTo(bounds.Width(), rect.bottom);

	BScrollBar *vertScroller = ScrollBar(B_VERTICAL);

	if (bounds.Height() > rect.top) {
		vertScroller->SetRange(0.0f, 0.0f);
		vertScroller->SetValue(0.0f);
	} else {
		vertScroller->SetRange(0.0f, rect.top - bounds.Height());
		vertScroller->SetProportion(bounds.Height () / rect.top);
        }

	vertScroller->SetSteps(15, bounds.Height());
	
	Invalidate();
}
