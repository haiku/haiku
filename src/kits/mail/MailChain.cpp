/* BMailChain - the mail account's inbound and outbound chain
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>

#include <string.h>
#include <stdio.h>

class BMailChain;

namespace MailInternal {
	status_t WriteMessageFile(const BMessage& archive,
		const BPath& path, const char* name);
}

#include <MailSettings.h>
#include <ChainRunner.h>
#include <status.h>


BMailChain::BMailChain(uint32 i)
	:
	fId(i),
	fMetaData(NULL),
	fStatus(B_OK),
	fDirection(inbound),
	fSettingsCount(0),
	fAddonsCount(0) 
{
	fName[0] = 0;
	Reload();
}


BMailChain::BMailChain(BMessage* settings)
	:
	fId(settings->FindInt32("id")),
	fMetaData(NULL),
	fStatus(B_OK),
	fDirection(inbound),
	fSettingsCount(0),
	fAddonsCount(0) 
{
	fName[0] = 0;
	Load(settings);
}


BMailChain::~BMailChain()
{
	delete fMetaData;
		
	for (int32 i = 0; fFilterSettings.ItemAt(i); i++)
		delete (BMessage*)fFilterSettings.ItemAt(i);
		
	for (int32 i = 0; fFilterAddons.ItemAt(i); i++)
		delete (entry_ref*)fFilterAddons.ItemAt(i);
}


status_t
BMailChain::Load(BMessage* settings)
{
	delete fMetaData;
		
	fMetaData = new BMessage;
	if (settings->HasMessage("meta_data"))
		settings->FindMessage("meta_data", fMetaData);
	
	const char* n;
	status_t ret = settings->FindString("name", &n);
	if (ret == B_OK)
		strncpy(fName, n, sizeof(fName));
	else
		fName[0] = '\0';

	type_code t;
	settings->GetInfo("filter_settings", &t, (int32*)(&fSettingsCount));
	settings->GetInfo("filter_addons", &t, (int32*)(&fAddonsCount));
	if (fSettingsCount != fAddonsCount)
		return B_MISMATCHED_VALUES;

	for (int i = 0;;i++) {
		BMessage* filter = new BMessage();
		entry_ref* ref = new entry_ref();
		char* addon_path;

		if (settings->FindMessage("filter_settings", i, filter) < B_OK
			|| ((settings->FindString("filter_addons", i, (const char**)&addon_path) < B_OK
				|| get_ref_for_path(addon_path, ref) < B_OK)
			&& settings->FindRef("filter_addons", i, ref) < B_OK)) {
			delete filter;
			delete ref;
			return B_NO_MEMORY;
		}
		
		if (!fFilterSettings.AddItem(filter)) {
			delete filter;
			delete ref;
			return B_NO_MEMORY;
		}

		if (!fFilterAddons.AddItem(ref)) {
			fFilterSettings.RemoveItem(filter);
			delete filter;
			delete ref;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
BMailChain::InitCheck() const
{
	if (fSettingsCount != fAddonsCount)
		return B_MISMATCHED_VALUES;
	if (fFilterSettings.CountItems() != fSettingsCount
		|| fFilterAddons.CountItems() != fAddonsCount)
		return B_NO_MEMORY;
	if (fStatus < B_OK)
		return fStatus;
		
	return B_OK;
}


status_t
BMailChain::Archive(BMessage* archive, bool deep) const
{
	status_t ret = InitCheck();
	
	if (ret != B_OK && ret != B_FILE_ERROR)
		return ret;
	
	ret = BArchivable::Archive(archive, deep);
	if (ret != B_OK)
		return ret;
	
	ret = archive->AddString("class", "BMailChain");
	if (ret != B_OK)
		return ret;	
	
	ret = archive->AddInt32("id", fId);
	if (ret != B_OK)
		return ret;
	
	ret = archive->AddString("name", fName);
	if (ret != B_OK)
		return ret;
	
	ret = archive->AddMessage("meta_data", fMetaData);
	if (ret != B_OK)
		return ret;
	
	if (ret == B_OK && deep) {
		BMessage* settings;
		entry_ref* ref;
		
		int32 i;
		for (i = 0;((settings = (BMessage*)fFilterSettings.ItemAt(i)) != NULL)
					&& ((ref = (entry_ref*)fFilterAddons.ItemAt(i)) != NULL);
			++i) {
			ret = archive->AddMessage("filter_settings", settings);
			if (ret < B_OK)
				return ret;
			
			BPath path(ref);
			if ((ret = path.InitCheck()) < B_OK)
				return ret;
			ret = archive->AddString("filter_addons", path.Path());
			if (ret < B_OK)
				return ret;
		}
		if (i != fSettingsCount)
			return B_MISMATCHED_VALUES;
	}
	
	return B_OK;
}


BArchivable* 
BMailChain::Instantiate(BMessage* archive)
{
	return validate_instantiation(archive, "BMailChain") ?
		new BMailChain(archive) : NULL;
}


status_t
BMailChain::GetPath(BPath& path) const
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK) {
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(status));
		return status;
	}

	path.Append("Mail/chains");

	if (ChainDirection() == outbound)
		path.Append("outbound");
	else
		path.Append("inbound");

	BString leaf;
	leaf << fId;
	path.Append(leaf.String());

	return B_OK;
}


status_t
BMailChain::Save(bigtime_t /*timeout*/)
{
	BMessage archive;
	status_t ret = Archive(&archive, true);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't archive chain %ld: %s\n",
			fId, strerror(ret));
		return ret;
	}

	BPath path;
	if ((ret = GetPath(path)) < B_OK)
		return ret;

	BPath directory;
	if ((ret = path.GetParent(&directory)) < B_OK)
		return ret;

	return MailInternal::WriteMessageFile(archive, directory,
		path.Leaf()/*, timeout*/);
}


status_t
BMailChain::Delete() const
{
	status_t status;
	BPath path;
	if ((status = GetPath(path)) < B_OK)
		return status;

	BEntry entry(path.Path());
	if ((status = entry.InitCheck()) < B_OK)
		return status;

	return entry.Remove();
}


b_mail_chain_direction
BMailChain::ChainDirection() const
{
	return fDirection;
}


void
BMailChain::SetChainDirection(b_mail_chain_direction dir)
{
	fDirection = dir;
}


status_t
BMailChain::Reload()
{
	BPath path;
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(ret));
		fStatus = ret;
		return ret;
	}
	
	path.Append("Mail/chains");
	{	
		//---Determine whether chain is outbound or inbound and glean full path.
		BPath working = path;
		working.Append("inbound");
		BString leaf;
		leaf << fId;
		
		//puts(path.Path());
		//puts(leaf.String());
		
		if (BDirectory(working.Path()).Contains(leaf.String())) {
			path = working;
			fDirection = inbound;
		} else {
			working = path;
			working.Append("outbound");
			if (BDirectory(working.Path()).Contains(leaf.String())) {
				path = working;
				fDirection = outbound;
			}
		}
		
		//puts(path.Path());
		
		path.Append(leaf.String());
		
		//puts(path.Path());
	}
	
	// open
	BFile settings(path.Path(),B_READ_ONLY);
	ret = settings.InitCheck();
	
	if (ret != B_OK) {
		BMessage empty;
		fprintf(stderr, "Couldn't open chain settings file '%s': %s\n",
			path.Path(), strerror(ret));
		Load(&empty);
		fStatus = B_FILE_ERROR;
		return ret;
	}
	
	// read settings
	BMessage tmp;
	ret = tmp.Unflatten(&settings);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(ret));
		Load(&tmp);
		fStatus = ret;
		return ret;
	}
	
	// clobber old settings
	fStatus = ret = Load(&tmp);
	return ret;
}


uint32
BMailChain::ID() const
{
	return fId;
}


const char*
BMailChain::Name() const
{
	return fName;
}


status_t
BMailChain::SetName(const char* n)
{
	if (n)
		strncpy(fName, n, sizeof(fName));
	else
		fName[0] = '\0';
	
	return B_OK;
}


BMessage*
BMailChain::MetaData() const
{
	return fMetaData;
}


int32
BMailChain::CountFilters() const
{
	return fFilterSettings.CountItems();
}


status_t
BMailChain::GetFilter(int32 index, BMessage* out_settings,
	entry_ref* addon) const
{
	if (index >= fFilterSettings.CountItems())
		return B_BAD_INDEX;
	
	BMessage* settings = (BMessage*)fFilterSettings.ItemAt(index);
	if (settings)
		*out_settings = *settings;
	else
		return B_BAD_INDEX;
	
	if (addon) {
		entry_ref* ref = (entry_ref*)fFilterAddons.ItemAt(index);
		if (ref)
			*addon = *ref;
		else
			return B_BAD_INDEX;
	}
	return B_OK;
}


status_t
BMailChain::SetFilter(int32 index, const BMessage& s,
	const entry_ref& addon)
{
	BMessage* settings = (BMessage*)fFilterSettings.ItemAt(index);
	if (settings)
		*settings = s;
	else
		return B_BAD_INDEX;
	
	entry_ref* ref = (entry_ref*)fFilterAddons.ItemAt(index);
	if (ref)
		*ref = addon;
	else
		return B_BAD_INDEX;
	
	return B_OK;
}


status_t
BMailChain::AddFilter(const BMessage& settings, const entry_ref& addon)
{
	BMessage* s = new BMessage(settings);
	entry_ref* a = new entry_ref(addon);
	
	if (!fFilterSettings.AddItem(s)) {
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	
	if (!fFilterAddons.AddItem(a)) {
		fFilterSettings.RemoveItem(fSettingsCount);
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	// else
	
	++fSettingsCount;
	++fAddonsCount;
	
	return B_OK;
}


status_t
BMailChain::AddFilter(int32 index, const BMessage& settings,
	const entry_ref& addon)
{
	BMessage* s = new BMessage(settings);
	entry_ref* a = new entry_ref(addon);
	
	if (!fFilterSettings.AddItem(s, index)) {
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	
	if (!fFilterAddons.AddItem(a, index)) {
		fFilterSettings.RemoveItem(index);
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	++fSettingsCount;
	++fAddonsCount;

	return B_OK;
}


status_t
BMailChain::RemoveFilter(int32 index)
{
	BMessage* s = (BMessage*)fFilterSettings.RemoveItem(index);
	delete s;

	entry_ref* a = (entry_ref*)fFilterAddons.RemoveItem(index);
	delete a;

	--fSettingsCount;
	--fAddonsCount;

	return s || a ? B_OK : B_BAD_INDEX;
}


void
BMailChain::RunChain(BMailStatusWindow *window, bool async,
	bool save_when_done, bool delete_when_done)
{
	(new BMailChainRunner(this, window, true, save_when_done,
		delete_when_done))->RunChain(async);
}
