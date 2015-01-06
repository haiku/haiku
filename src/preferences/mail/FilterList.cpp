/*
 * Copyright 2011-2015, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "FilterList.h"

#include <set>
#include <stdio.h>

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>


FilterList::FilterList(direction dir)
	:
	fDirection(dir)
{
}


FilterList::~FilterList()
{
	_MakeEmpty();
}


void
FilterList::Reload()
{
	_MakeEmpty();

	std::set<BString> knownNames;

	directory_which which[] = {B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY};
	for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath path;
		status_t status = find_directory(which[i], &path);
		if (status != B_OK)
			continue;

		path.Append("mail_daemon");
		if (fDirection == kIncoming)
			path.Append("inbound_filters");
		else
			path.Append("outbound_filters");

		BDirectory dir(path.Path());
		if (dir.InitCheck() != B_OK)
			continue;

		BEntry entry;
		while (dir.GetNextEntry(&entry) == B_OK) {
			// Ignore entries we already had before (ie., user add-ons are
			// overriding system add-ons)
			if (knownNames.find(entry.Name()) != knownNames.end())
				continue;

			if (_LoadAddOn(entry) == B_OK)
				knownNames.insert(entry.Name());
		}
	}
}


int32
FilterList::CountInfos() const
{
	return fList.size();
}


const FilterInfo&
FilterList::InfoAt(int32 index) const
{
	return fList[index];
}


int32
FilterList::InfoIndexFor(const entry_ref& ref) const
{
	for (size_t i = 0; i < fList.size(); i++) {
		const FilterInfo& info = fList[i];
		if (info.ref == ref)
			return i;
	}
	return -1;
}


BString
FilterList::SimpleName(int32 index,
	const BMailAccountSettings& accountSettings) const
{
	return DescriptiveName(index, accountSettings, NULL);
}


BString
FilterList::SimpleName(const entry_ref& ref,
	const BMailAccountSettings& accountSettings) const
{
	return DescriptiveName(InfoIndexFor(ref), accountSettings, NULL);
}


BString
FilterList::DescriptiveName(int32 index,
	const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* settings) const
{
	if (index < 0 || index >= CountInfos())
		return "-";

	const FilterInfo& info = InfoAt(index);
	return info.name(accountSettings, settings);
}


BString
FilterList::DescriptiveName(const entry_ref& ref,
	const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* settings) const
{
	return DescriptiveName(InfoIndexFor(ref), accountSettings, settings);
}


BMailSettingsView*
FilterList::CreateSettingsView(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings)
{
	const entry_ref& ref = settings.AddOnRef();
	int32 index = InfoIndexFor(ref);
	if (index < 0)
		return NULL;

	const FilterInfo& info = InfoAt(index);
	return info.instantiateSettingsView(accountSettings, settings);
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


status_t
FilterList::_LoadAddOn(BEntry& entry)
{
	FilterInfo info;

	BPath path(&entry);
	info.image = load_add_on(path.Path());
	if (info.image < 0)
		return info.image;

	status_t status = get_image_symbol(info.image,
		"instantiate_filter_settings_view", B_SYMBOL_TYPE_TEXT,
		(void**)&info.instantiateSettingsView);
	if (status == B_OK) {
		status = get_image_symbol(info.image, "filter_name", B_SYMBOL_TYPE_TEXT,
			(void**)&info.name);
	}
	if (status != B_OK) {
		fprintf(stderr, "Filter \"%s\" misses required hooks!\n", path.Path());
		unload_add_on(info.image);
		return B_NAME_NOT_FOUND;
	}

	entry.GetRef(&info.ref);
	fList.push_back(info);
	return B_OK;
}
