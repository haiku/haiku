/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageViews.h"

#include <stdio.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <GroupLayout.h>
#include <LayoutUtils.h>
#include <Locale.h>
#include <Messenger.h>
#include <package/PackageInfo.h>
#include <Path.h>
#include <ScrollBar.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "InstallerDefs.h"
#include "StringForSize.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackagesView"

#define ICON_ATTRIBUTE "INSTALLER PACKAGE: ICON"


Package::Package(const BPath &path)
	:
	Group(),
	fSize(0),
	fIcon(NULL)
{
	SetPath(path);
}


Package::~Package()
{
	delete fIcon;
}


Package *
Package::PackageFromEntry(BEntry &entry)
{
	BPath path;
	entry.GetPath(&path);

	BPackageKit::BPackageInfo info;
	info.ReadFromPackageFile(path.Path());

	if (info.InitCheck() != B_OK)
		return NULL;

	Package *package = new Package(path);
	package->fName = info.Name();
	package->fDescription = info.Summary();

	bool alwaysOn = false;
	bool onByDefault = true;
	off_t size = 0;
	entry.GetSize(&size);
	char group[64];
	memset(group, 0, 64);

	BNode node(&entry);
	// FIXME enable these when the attributes on the packages are actually
	// populated by the buildsystem. For now, assume everything is
	// on-by-default and optional, and have no groups.
#if 0
	if (node.ReadAttr("INSTALLER PACKAGE: GROUP", B_STRING_TYPE, 0,
		group, 64) < 0) {
		goto err;
	}
	if (node.ReadAttr("INSTALLER PACKAGE: ON_BY_DEFAULT", B_BOOL_TYPE, 0,
		&onByDefault, sizeof(onByDefault)) < 0) {
		goto err;
	}
	if (node.ReadAttr("INSTALLER PACKAGE: ALWAYS_ON", B_BOOL_TYPE, 0,
		&alwaysOn, sizeof(alwaysOn)) < 0) {
		goto err;
	}
#endif
	package->SetGroupName(group);
	package->SetSize(size);
	package->SetAlwaysOn(alwaysOn);
	package->SetOnByDefault(onByDefault);

	attr_info attributes;
	if (node.GetAttrInfo(ICON_ATTRIBUTE, &attributes) == B_OK) {
		char buffer[attributes.size];
		BMessage msg;
		if ((node.ReadAttr(ICON_ATTRIBUTE, attributes.type, 0, buffer,
				attributes.size) == attributes.size)
			&& (msg.Unflatten(buffer) == B_OK)) {
			package->SetIcon(new BBitmap(&msg));
		}
	}
	return package;
#if 0
err:
#endif
	delete package;
	return NULL;
}


void
Package::GetSizeAsString(char* string, size_t stringSize)
{
	string_for_size(fSize, string, stringSize);
}


Group::Group()
{

}

Group::~Group()
{
}


PackageCheckBox::PackageCheckBox(Package *item)
	:
	BCheckBox("pack_cb", item->Name(), NULL),
	fPackage(item)
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
	SetIcon(fPackage->Icon());
}


PackageCheckBox::~PackageCheckBox()
{
	delete fPackage;
}


void
PackageCheckBox::Draw(BRect update)
{
	BCheckBox::Draw(update);

	// Draw the label
	char string[15];
	fPackage->GetSizeAsString(string, sizeof(string));
	float width = StringWidth(string);
	BRect sizeRect = Bounds();
	sizeRect.left = sizeRect.right - width;
	be_control_look->DrawLabel(this, string, NULL, sizeRect, update,
		ui_color(B_DOCUMENT_BACKGROUND_COLOR), be_control_look->Flags(this));
}


void
PackageCheckBox::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW) {
		BMessage msg(MSG_STATUS_MESSAGE);
		msg.AddString("status", fPackage->Description());
		BMessenger(NULL, Window()).SendMessage(&msg);
	} else if (transit == B_EXITED_VIEW) {
		BMessage msg(MSG_STATUS_MESSAGE);
		BMessenger(NULL, Window()).SendMessage(&msg);
	}
}


GroupView::GroupView(Group *group)
	:
	BStringView("group", group->GroupName()),
	fGroup(group)
{
	SetFont(be_bold_font);
}


GroupView::~GroupView()
{
	delete fGroup;
}


// #pragma mark -


PackagesView::PackagesView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	layout->SetSpacing(0);
	layout->SetInsets(B_USE_SMALL_SPACING, 0);
	SetLayout(layout);

	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	SetExplicitMinSize(BSize(B_SIZE_UNSET, 80));
}


PackagesView::~PackagesView()
{

}


void
PackagesView::Clean()
{
	BView* view;
	while ((view = ChildAt(0))) {
		if (dynamic_cast<GroupView*>(view)
			|| dynamic_cast<PackageCheckBox*>(view)) {
			RemoveChild(view);
			delete view;
		}
	}
	ScrollTo(0, 0);

	BView* parent = Parent();
	BRect r = parent->Bounds();
	parent->FrameResized(r.Width(), r.Height());
}


void
PackagesView::AddPackages(BList& packages, BMessage* msg)
{
	int32 count = packages.CountItems();
	BString lastGroup = "";
	for (int32 i = 0; i < count; i++) {
		void* item = packages.ItemAt(i);
		Package* package = static_cast<Package*>(item);
		if (lastGroup != BString(package->GroupName())) {
			lastGroup = package->GroupName();
			Group* group = new Group();
			group->SetGroupName(package->GroupName());
			GroupView *view = new GroupView(group);
			AddChild(view);
		}
		PackageCheckBox* checkBox = new PackageCheckBox(package);
		checkBox->SetValue(package->OnByDefault()
			? B_CONTROL_ON : B_CONTROL_OFF);
		checkBox->SetEnabled(!package->AlwaysOn());
		checkBox->SetMessage(new BMessage(*msg));
		AddChild(checkBox);
	}
	Invalidate();

	// Force scrollbars to update
	BView* parent = Parent();
	BRect r = parent->Bounds();
	parent->FrameResized(r.Width(), r.Height());
}


void
PackagesView::GetTotalSizeAsString(char* string, size_t stringSize)
{
	int32 count = CountChildren();
	int32 size = 0;
	for (int32 i = 0; i < count; i++) {
		PackageCheckBox* cb = dynamic_cast<PackageCheckBox*>(ChildAt(i));
		if (cb && cb->Value())
			size += cb->GetPackage()->Size();
	}
	string_for_size(size, string, stringSize);
}


void
PackagesView::GetPackagesToInstall(BList* list, int32* size)
{
	int32 count = CountChildren();
	*size = 0;
	for (int32 i = 0; i < count; i++) {
		PackageCheckBox* cb = dynamic_cast<PackageCheckBox*>(ChildAt(i));
		if (cb && cb->Value()) {
			list->AddItem(cb->GetPackage());
			*size += cb->GetPackage()->Size();
		}
	}
}


void
PackagesView::Draw(BRect updateRect)
{
	if (CountChildren() > 0)
		return;

	be_control_look->DrawLabel(this,
		B_TRANSLATE("No optional packages available."),
		Bounds(), updateRect, ViewColor(), BControlLook::B_DISABLED,
		BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
}
