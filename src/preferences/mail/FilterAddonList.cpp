/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "FilterAddonList.h"

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>


FilterAddonList::FilterAddonList(direction dir, bool loadOnStart)
	:
	fDirection(dir)
{
	if (loadOnStart)
		Reload();
}


FilterAddonList::~FilterAddonList()
{
	_MakeEmpty();
}


void
FilterAddonList::Reload()
{
	_MakeEmpty();

	BPath path;
	status_t status = find_directory(B_SYSTEM_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return;
	path.Append("mail_daemon");
	if (fDirection == kIncomming)
		path.Append("inbound_filters");
	else
		path.Append("outbound_filters");

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK)
		return;
	BEntry entry;
	while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND)
		_LoadAddon(entry);
}


int32
FilterAddonList::CountFilterAddons()
{
	return fFilterAddonList.size();
}


FilterAddonInfo&
FilterAddonList::FilterAddonAt(int32 index)
{
	return fFilterAddonList[index];
}


bool
FilterAddonList::GetDescriptiveName(int32 index, BString& name)
{
	if (index < 0)
		return false;

	FilterAddonInfo& info = FilterAddonAt(index);

	BString (*descriptive_name)();
	if (get_image_symbol(info.image, "descriptive_name", B_SYMBOL_TYPE_TEXT,
		(void **)&descriptive_name) == B_OK) {
		name = (*descriptive_name)();
	} else
		name = info.ref.name;
	return true;
}


bool
FilterAddonList::GetDescriptiveName(const entry_ref& ref, BString& name)
{
	int32 index = FindInfo(ref);
	return GetDescriptiveName(index, name);
}


BView*
FilterAddonList::CreateConfigView(AddonSettings& settings)
{
	const entry_ref& ref = settings.AddonRef();
	int32 index = FindInfo(ref);
	if (index < 0)
		return NULL;
	FilterAddonInfo& info = FilterAddonAt(index);

	BView* (*instantiate_filter_config_panel)(AddonSettings&);
	if (get_image_symbol(info.image, "instantiate_filter_config_panel",
		B_SYMBOL_TYPE_TEXT, (void **)&instantiate_filter_config_panel) != B_OK)
		return NULL;
	return (*instantiate_filter_config_panel)(settings);
}


void
FilterAddonList::_MakeEmpty()
{
	for (unsigned int i = 0; i < fFilterAddonList.size(); i++) {
		FilterAddonInfo& info = fFilterAddonList[i];
		unload_add_on(info.image);
	}
	fFilterAddonList.clear();
}


int32
FilterAddonList::FindInfo(const entry_ref& ref)
{
	for (unsigned int i = 0; i < fFilterAddonList.size(); i++) {
		FilterAddonInfo& currentInfo = fFilterAddonList[i];
		if (currentInfo.ref == ref)
			return i;
	}
	return -1;
}


void
FilterAddonList::_LoadAddon(BEntry& entry)
{
	FilterAddonInfo info;

	BPath path(&entry);
	info.image = load_add_on(path.Path());
	if (info.image < 0)
		return;

	BView* (*instantiate_filter_config_panel)(MailAddonSettings&);
	if (get_image_symbol(info.image, "instantiate_filter_config_panel",
		B_SYMBOL_TYPE_TEXT, (void **)&instantiate_filter_config_panel)
			!= B_OK) {
		unload_add_on(info.image);
		return;
	}

	entry.GetRef(&info.ref);

	fFilterAddonList.push_back(info);
}
