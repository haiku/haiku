/*
 * Copyright 2011-2012, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "FilterList.h"

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>


FilterList::FilterList(direction dir, bool loadOnStart)
	:
	fDirection(dir)
{
	if (loadOnStart)
		Reload();
}


FilterList::~FilterList()
{
	_MakeEmpty();
}


void
FilterList::Reload()
{
	_MakeEmpty();

	BPath path;
	status_t status = find_directory(B_SYSTEM_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return;
	path.Append("mail_daemon");
	if (fDirection == kIncoming)
		path.Append("inbound_filters");
	else
		path.Append("outbound_filters");

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK)
		return;
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK)
		_LoadAddOn(entry);
}


int32
FilterList::CountInfos()
{
	return fList.size();
}


FilterInfo&
FilterList::InfoAt(int32 index)
{
	return fList[index];
}


bool
FilterList::GetDescriptiveName(int32 index, BString& name)
{
	if (index < 0)
		return false;

	FilterInfo& info = InfoAt(index);

	BString (*descriptive_name)();
	if (get_image_symbol(info.image, "descriptive_name", B_SYMBOL_TYPE_TEXT,
		(void **)&descriptive_name) == B_OK) {
		name = (*descriptive_name)();
	} else
		name = info.ref.name;
	return true;
}


bool
FilterList::GetDescriptiveName(const entry_ref& ref, BString& name)
{
	int32 index = InfoIndexFor(ref);
	return GetDescriptiveName(index, name);
}


BView*
FilterList::CreateConfigView(BMailAddOnSettings& settings)
{
	const entry_ref& ref = settings.AddOnRef();
	int32 index = InfoIndexFor(ref);
	if (index < 0)
		return NULL;
	FilterInfo& info = InfoAt(index);

	BView* (*instantiateFilterConfigPanel)(BMailAddOnSettings&);
	if (get_image_symbol(info.image, "instantiate_filter_config_panel",
			B_SYMBOL_TYPE_TEXT, (void **)&instantiateFilterConfigPanel) != B_OK)
		return NULL;
	return (*instantiateFilterConfigPanel)(settings);
}


int32
FilterList::InfoIndexFor(const entry_ref& ref)
{
	for (size_t i = 0; i < fList.size(); i++) {
		FilterInfo& info = fList[i];
		if (info.ref == ref)
			return i;
	}
	return -1;
}


void
FilterList::_MakeEmpty()
{
	for (size_t i = 0; i < fList.size(); i++) {
		FilterInfo& info = fList[i];
		unload_add_on(info.image);
	}
	fList.clear();
}


void
FilterList::_LoadAddOn(BEntry& entry)
{
	FilterInfo info;

	BPath path(&entry);
	info.image = load_add_on(path.Path());
	if (info.image < 0)
		return;

	BView* (*instantiateFilterConfigPanel)(BMailProtocolSettings&);
	if (get_image_symbol(info.image, "instantiate_filter_config_panel",
			B_SYMBOL_TYPE_TEXT, (void **)&instantiateFilterConfigPanel)
				!= B_OK) {
		unload_add_on(info.image);
		return;
	}

	entry.GetRef(&info.ref);

	fList.push_back(info);
}
