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

class _EXPORT BMailChain;

namespace MailInternal {
	status_t WriteMessageFile(const BMessage& archive, const BPath& path, const char* name);
}

#include <MailSettings.h>
#include <ChainRunner.h>
#include <status.h>

BMailChain::BMailChain(uint32 i)
	: id(i), meta_data(NULL), _err(B_OK), direction(inbound), settings_ct(0), addons_ct(0) 
{
	name[0] = 0;
	Reload();
}

BMailChain::BMailChain(BMessage* settings)
	: id(settings->FindInt32("id")), meta_data(NULL), _err(B_OK), direction(inbound), settings_ct(0), addons_ct(0) 
{
	name[0] = 0;
	Load(settings);
}


BMailChain::~BMailChain() {
	if (meta_data != NULL)
		delete meta_data;
		
	for (int32 i = 0; filter_settings.ItemAt(i); i++)
		delete (BMessage *)filter_settings.ItemAt(i);
		
	for (int32 i = 0; filter_addons.ItemAt(i); i++)
		delete (entry_ref *)filter_addons.ItemAt(i);
}

status_t BMailChain::Load(BMessage* settings)
{
	if (meta_data != NULL)
		delete meta_data;
		
	meta_data = new BMessage;
	if (settings->HasMessage("meta_data"))
		settings->FindMessage("meta_data",meta_data);
	
	const char* n;
	status_t ret = settings->FindString("name",&n);
	if (ret == B_OK) strncpy(name,n,sizeof(name));
	else name[0]='\0';

	type_code t;
	settings->GetInfo("filter_settings",&t,(int32 *)(&settings_ct));
	settings->GetInfo("filter_addons",&t,(int32 *)(&addons_ct));
	if (settings_ct!=addons_ct) return B_MISMATCHED_VALUES;

	for (int i = 0;;i++)
	{
		BMessage *filter = new BMessage();
		entry_ref *ref = new entry_ref();
		char *addon_path;

		if (settings->FindMessage("filter_settings",i,filter) < B_OK
			|| (settings->FindString("filter_addons",i,(const char **)&addon_path) < B_OK
				|| get_ref_for_path(addon_path,ref) < B_OK)
			&& settings->FindRef("filter_addons",i,ref) < B_OK)
		{
			delete filter;
			delete ref;
			break;
		}
		
		if (!filter_settings.AddItem(filter) || !filter_addons.AddItem(ref))
			break;
	}
	
	if (filter_settings.CountItems()!=settings_ct
	||  filter_addons.CountItems()!=addons_ct)
		return B_NO_MEMORY;
	else
		return B_OK;
}

status_t BMailChain::InitCheck() const
{
	if (settings_ct!=addons_ct)
		return B_MISMATCHED_VALUES;
	if (filter_settings.CountItems()!=settings_ct
	||  filter_addons.CountItems()!=addons_ct)
		return B_NO_MEMORY;
	if (_err < B_OK)
		return _err;
		
	return B_OK;
}


status_t BMailChain::Archive(BMessage* archive, bool deep) const
{
	status_t ret = B_OK;
	
	ret = InitCheck();
	if ((ret!=B_OK) && (ret != B_FILE_ERROR)) return ret;
	
	ret = BArchivable::Archive(archive,deep);
	if (ret!=B_OK) return ret;
	
	ret = archive->AddString("class","BMailChain");
	if (ret!=B_OK) return ret;
	
	
	ret = archive->AddInt32("id",id);
	if (ret!=B_OK) return ret;
	
	ret = archive->AddString("name",name);
	if (ret!=B_OK) return ret;
	
	ret = archive->AddMessage("meta_data",meta_data);
	if (ret!=B_OK) return ret;
	
	if (ret==B_OK && deep)
	{
		BMessage* settings;
		entry_ref* ref;
		
		int32 i;
		for (i = 0;((settings = (BMessage*)filter_settings.ItemAt(i)) != NULL)
					&& ((ref = (entry_ref*)filter_addons.ItemAt(i)) != NULL);
			++i)
		{
			ret = archive->AddMessage("filter_settings",settings);
			if (ret < B_OK)
				return ret;
			
			BPath path(ref);
			if ((ret = path.InitCheck()) < B_OK)
				return ret;
			ret = archive->AddString("filter_addons",path.Path());
			if (ret < B_OK)
				return ret;
		}
		if (i != settings_ct)
			return B_MISMATCHED_VALUES;
	}
	
	return B_OK;
}

BArchivable* BMailChain::Instantiate(BMessage* archive)
{
	return validate_instantiation(archive, "BMailChain")?
		new BMailChain(archive) : NULL;
}

status_t BMailChain::Path(BPath *path) const
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY,path);
	if (status < B_OK)
	{
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(status));
		return status;
	}

	path->Append("Mail/chains");

	if (ChainDirection() == outbound)
		path->Append("outbound");
	else
		path->Append("inbound");

	BString leaf;
	leaf << id;
	path->Append(leaf.String());

	return B_OK;
}

status_t BMailChain::Save(bigtime_t /*timeout*/)
{
	status_t ret;
	
	BMessage archive;
	ret = Archive(&archive,true);
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't archive chain %ld: %s\n",
			id, strerror(ret));
		return ret;
	}

	BPath path;
	if ((ret = Path(&path)) < B_OK)
		return ret;

	BPath directory;
	if ((ret = path.GetParent(&directory)) < B_OK)
		return ret;

	return MailInternal::WriteMessageFile(archive,directory,path.Leaf()/*,timeout*/);
}

status_t BMailChain::Delete() const
{
	status_t status;
	BPath path;
	if ((status = Path(&path)) < B_OK)
		return status;

	BEntry entry(path.Path());
	if ((status = entry.InitCheck()) < B_OK)
		return status;

	return entry.Remove();
}

b_mail_chain_direction BMailChain::ChainDirection() const {
	return direction;
}

void BMailChain::SetChainDirection(b_mail_chain_direction dir) {
	direction = dir;
}

status_t BMailChain::Reload()
{
	status_t ret;
	
	BPath path;
	ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(ret));
		_err = ret;
		return ret;
	}
	
	path.Append("Mail/chains");
	{	
		//---Determine whether chain is outbound or inbound and glean full path.
		BPath working = path;
		working.Append("inbound");
		BString leaf;
		leaf << id;
		
		//puts(path.Path());
		//puts(leaf.String());
		
		if (BDirectory(working.Path()).Contains(leaf.String())) {
			path = working;
			direction = inbound;
		} else {
			working = path;
			working.Append("outbound");
			if (BDirectory(working.Path()).Contains(leaf.String())) {
				path = working;
				direction = outbound;
			}
		}
		
		//puts(path.Path());
		
		path.Append(leaf.String());
		
		//puts(path.Path());
	}
	
	// open
	BFile settings(path.Path(),B_READ_ONLY);
	ret = settings.InitCheck();
	
	if (ret != B_OK)
	{
		BMessage empty;
		fprintf(stderr, "Couldn't open chain settings file '%s': %s\n",
			path.Path(), strerror(ret));
		Load(&empty);
		_err = B_FILE_ERROR;
		return ret;
	}
	
	// read settings
	BMessage tmp;
	ret = tmp.Unflatten(&settings);
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(ret));
		Load(&tmp);
		_err = ret;
		return ret;
	}
	
	// clobber old settings
	_err = ret = Load(&tmp);
	return ret;
}

uint32 BMailChain::ID() const { return id; }

const char *BMailChain::Name() const { return name; }
status_t BMailChain::SetName(const char* n)
{
	if (n) strncpy(name,n,sizeof(name));
	else name[0]='\0';
	
	return B_OK;
}

BMessage *BMailChain::MetaData() const {
	return meta_data;
}

int32 BMailChain::CountFilters() const
{
	return filter_settings.CountItems();
}

status_t BMailChain::GetFilter(int32 index, BMessage* out_settings, entry_ref *addon) const
{
	if (index >= filter_settings.CountItems())
		return B_BAD_INDEX;
	
	BMessage *settings = (BMessage *)filter_settings.ItemAt(index);
	if (settings) *out_settings = *settings;
	else return B_BAD_INDEX;
	
	if (addon)
	{
		entry_ref* ref = (entry_ref* )filter_addons.ItemAt(index);
		if (ref) *addon = *ref;
		else return B_BAD_INDEX;
	}
	return B_OK;
}

status_t BMailChain::SetFilter(int32 index, const BMessage& s, const entry_ref& addon)
{
	BMessage *settings = (BMessage *)filter_settings.ItemAt(index);
	if (settings) *settings = s;
	else return B_BAD_INDEX;
	
	entry_ref* ref = (entry_ref* )filter_addons.ItemAt(index);
	if (ref) *ref = addon;
	else return B_BAD_INDEX;
	
	return B_OK;
}

status_t BMailChain::AddFilter(const BMessage& settings, const entry_ref& addon)
{
	BMessage *s = new BMessage(settings);
	entry_ref*a = new entry_ref(addon);
	
	if (!filter_settings.AddItem(s))
	{
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	else if (!filter_addons.AddItem(a))
	{
		filter_settings.RemoveItem(settings_ct);
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	// else
	
	++settings_ct;
	++addons_ct;
	
	return B_OK;
}
status_t BMailChain::AddFilter(int32 index, const BMessage& settings, const entry_ref& addon)
{
	BMessage *s = new BMessage(settings);
	entry_ref*a = new entry_ref(addon);
	
	if (!filter_settings.AddItem(s,index))
	{
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	else if (!filter_addons.AddItem(a,index))
	{
		filter_settings.RemoveItem(index);
		delete s;
		delete a;
		return B_BAD_INDEX;
	}
	++settings_ct;
	++addons_ct;

	return B_OK;
}

status_t BMailChain::RemoveFilter(int32 index)
{
	BMessage* s = (BMessage*)filter_settings.RemoveItem(index);
	delete s;

	entry_ref*a = (entry_ref*)filter_addons.RemoveItem(index);
	delete a;

	--settings_ct;
	--addons_ct;

	return s||a?B_OK:B_BAD_INDEX;
}

void BMailChain::RunChain(BMailStatusWindow *window, bool async, bool save_when_done, bool delete_when_done) {
	(new BMailChainRunner(this,window,true,save_when_done,delete_when_done))->RunChain(async);
}
