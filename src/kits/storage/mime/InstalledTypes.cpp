// mime/InstalledTypes.cpp

#include "mime/InstalledTypes.h"

#include <Message.h>

#include <stdio.h>

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

InstalledTypes::InstalledTypes()
	: fCachedMessage(NULL)
{
}

InstalledTypes::~InstalledTypes()
{
	delete fCachedMessage;
}

/*
MimeDatabaseBackend::MimeDatabaseBackend()
	: fCachedInstalledTypesMessage(NULL)
	, fCachedInstalledSupertypesMessage(NULL)
{
}

MimeDatabaseBackend::~MimeDatabaseBackend()
{
	delete fCachedInstalledTypesMessage;
	delete fCachedInstalledSupertypesMessage;
}

status_t
MimeDatabaseBackend::GetInstalledTypes(BMessage *types)
{
	DBG(OUT("MimeDatabaseBackend::GetInstalledTypes(1)\n"));
	status_t err = types ? B_OK : B_BAD_VALUE;
	if (!err) {
		if (!fCachedInstalledTypesMessage) {
			if (fInstalledTypes.begin() == fInstalledTypes.end())
				err = BuildInstalledTypesList(&fCachedInstalledTypesMessage);
			else
				err = BuildInstalledTypesMessage(&fCachedInstalledTypesMessage);
		}
		*types = *fCachedInstalledTypesMessage;	
	}
	DBG(OUT("MimeDatabaseBackend::GetInstalledTypes(1) done\n"));
	return err;		
}

status_t
MimeDatabaseBackend::GetInstalledTypes(const char *supertype, BMessage *types)
{
	DBG(OUT("MimeDatabaseBackend::GetInstalledTypes(2)\n"));
	status_t err = supertype && types && BMimeType::IsValid(supertype) ? B_OK : B_BAD_VALUE;
	
	// Types are not case sensitive, so we always use lowercase
	char lowertype[B_PATH_NAME_LENGTH+1];
	BPrivate::Storage::to_lower(supertype, lowertype);
	installed_subtypes_info *info;
	
	// See if we need to build the main list
	if (!err && fSupertypesMap.empty())
		err = BuildInstalledTypesList();
	// See if the supertype even exists
	if (!err) {
		std::map<std::string, installed_subtypes_info>::iterator i = fSupertypesMap.find(supertype);		
		err =  i != fSupertypesMap.end() ? B_OK : B_BAD_VALUE;
		if (!err)
			info = &(i->second);
	}
	// Make sure there's a cached message waiting for us
	if (!err && !info->cached_message) 
		err = BuildInstalledTypesMessage(info->subtypes, &(info->cached_message));
	// Copy the message into the result
	if (!err)		
		*types = *info->cached_message;
	DBG(OUT("MimeDatabaseBackend::GetInstalledTypes(2) done\n"));
	return err;		
}

status_t
MimeDatabaseBackend::GetInstalledSupertypes(BMessage *types)
{
	DBG(OUT("MimeDatabaseBackend::GetInstalledSupertypes()\n"));
	status_t err = types ? B_OK : B_BAD_VALUE;
	
	// See if we need to build the main list
	if (!err && fSupertypesMap.empty())
		err = BuildInstalledTypesList();
	// Make sure there's a cached message waiting for us
	if (!err && !fCachedInstalledSupertypesMessage) 
		err = BuildInstalledSupertypesMessage(fSupertypesMap, &fCachedInstalledSupertypesMessage);
	// Copy the message into the result
	if (!err)		
		*types = *fCachedInstalledSupertypesMessage;
	DBG(OUT("MimeDatabaseBackend::GetInstalledSupertypes() done\n"));
	return err;		
}

status_t
MimeDatabaseBackend::GetSupportingApps(const char *type, BMessage *signatures)
{
	return B_ERROR;	// not implemented
}

void
MimeDatabaseBackend::InstallType(const char *type)
{
	DBG(OUT("MimeDatabaseBackend::InstallType()\n"));
	if (fInstalledTypes.empty())
		return;
	// Remove any cached messages
	if (fCachedInstalledTypesMessage) {
		delete fCachedInstalledTypesMessage;
		fCachedInstalledTypesMessage = NULL;
	}
	if (fCachedInstalledSupertypesMessage) {
		delete fCachedInstalledSupertypesMessage;
		fCachedInstalledSupertypesMessage = NULL;
	}
	char lowertype[B_PATH_NAME_LENGTH+1];
	char supertype[B_PATH_NAME_LENGTH+1];
	Storage::to_lower(type, lowertype);
	fInstalledTypes.insert(lowertype);
	BMimeType mime(lowertype);
	BMimeType super;
	status_t err = mime.InitCheck();
	if (!err)
		err = mime.GetSupertype(&super);
	if (!err) {
		Storage::to_lower(super.Type(), supertype);
		installed_subtypes_info &info = fSupertypesMap[supertype];
		delete info.cached_message;
		info.cached_message = NULL;
		info.subtypes.insert(lowertype);
	}		
	DBG(OUT("MimeDatabaseBackend::InstallType() done\n"));
}

void
MimeDatabaseBackend::DeleteType(const char *type)
{
	if (fInstalledTypes.empty())
		return;
	printf("fuCK\n");
	// Remove any cached messages
	if (fCachedInstalledTypesMessage) {
		delete fCachedInstalledTypesMessage;
		fCachedInstalledTypesMessage = NULL;
	}
	printf("fuCK\n");
	if (fCachedInstalledSupertypesMessage) {
		delete fCachedInstalledSupertypesMessage;
		fCachedInstalledSupertypesMessage = NULL;
	}
	printf("fuCK\n");
	char lowertype[B_PATH_NAME_LENGTH+1];
	char supertype[B_PATH_NAME_LENGTH+1];
	printf("fuCK\n");
	Storage::to_lower(type, lowertype);
	fInstalledTypes.erase(lowertype);
	printf("fuCK\n");
	BMimeType mime(lowertype);
	BMimeType super;
	printf("fuCK\n");
	status_t err = mime.InitCheck();
	printf("fuCK\n");
	if (!err)
		err = mime.GetSupertype(&super);
	printf("fuCK\n");
	if (!err) {
		Storage::to_lower(super.Type(), supertype);
		installed_subtypes_info &info = fSupertypesMap[supertype];
	printf("fuCK\n");
		delete info.cached_message;
	printf("fuCK\n");
		info.cached_message = NULL;
	printf("fuCK\n");
		info.subtypes.erase(lowertype);
	}		
	printf("fuCK\n");
}

// BuildInstalledTypesList()
//! Builds an initial installed types list and (optionally) an installed types BMessage
status_t
MimeDatabaseBackend::BuildInstalledTypesList(BMessage **types)
{
	BMessage *msg;
	if (types) {
		*types = new BMessage;
		msg = *types;
	} else
		msg = NULL;

	BDirectory root;
	status_t err = root.SetTo(MimeDatabase::kDatabaseDir.c_str());
	if (!err) {
		root.Rewind();
		while (true) {		
			BEntry entry;
			err = root.GetNextEntry(&entry);
			if (err) {
				// If we've come to the end of list, it's not an error
				if (err == B_ENTRY_NOT_FOUND) 
					err = B_OK;
				break;
			} else {
				// Check that this entry is both a directory and a valid MIME string
				char supertype[B_PATH_NAME_LENGTH+1];	
				if (entry.IsDirectory()
				      && entry.GetName(supertype) == B_OK
				         && BMimeType::IsValid(supertype))
				{
					// Make sure our string is all lowercase
					BPrivate::Storage::to_lower(supertype);
					
					// Add this supertype 
					fInstalledTypes.insert(supertype);
					fSupertypesMap[supertype] = installed_subtypes_info();
					installed_subtypes_info &info = fSupertypesMap[supertype];
					if (msg)
						msg->AddString("types", supertype);
					
					// Now iterate through this supertype directory and add
					// all of its subtypes
					BDirectory dir;
					if (dir.SetTo(&entry) == B_OK) {
						dir.Rewind();
						while (true) {
							BEntry subEntry;						
							err = dir.GetNextEntry(&subEntry);
							if (err) {
								// If we've come to the end of list, it's not an error
								if (err == B_ENTRY_NOT_FOUND) 
									err = B_OK;
								break;
							} else {																	
								// Get the subtype's name
								char subtype[B_PATH_NAME_LENGTH+1];
								char mimetype[B_PATH_NAME_LENGTH+1];
								if (subEntry.GetName(subtype) == B_OK) {
									BPrivate::Storage::to_lower(subtype);
									sprintf(mimetype, "%s/%s", supertype, subtype);
								
									// Add the subtype
									if (BMimeType::IsValid(mimetype)) {
										fInstalledTypes.insert(mimetype);
										info.subtypes.insert(mimetype);
										if (msg)
											msg->AddString("types", mimetype);
									}
								}
							}	
						}
					} else {
						DBG(OUT("MimeDatabaseBackend::BuildInstalledTypesList(): "
						          "Failed opening supertype directory '%s'\n",
						            supertype));
					}					
				}
			}			
		}			
	} else {
		DBG(OUT("MimeDatabaseBackend::BuildInstalledTypesList(): "
		          "Failed opening mime database directory '%s'\n",
		            MimeDatabase::kDatabaseDir.c_str()));
	}
	return err;
}

status_t
MimeDatabaseBackend::BuildInstalledTypesMessage(BMessage **result)
{
	status_t err = result ? B_OK : B_BAD_VALUE;
	// Alloc the message
	if (!err) {
		*result = new BMessage;
		if (!(*result))
			err = B_NO_MEMORY;
	}
	// Fill with types
	if (!err) {
		BMessage &msg = **result;
		std::set<std::string>::const_iterator i;
		for (i = fInstalledTypes.begin(); i != fInstalledTypes.end(); i++)
			msg.AddString("types", (*i).c_str());
	}
	return err;		
}

status_t
MimeDatabaseBackend::BuildInstalledTypesMessage(const std::set<std::string> &set, BMessage **result)
{
	status_t err = result ? B_OK : B_BAD_VALUE;
	// Alloc the message
	if (!err) {
		*result = new BMessage;
		if (!(*result))
			err = B_NO_MEMORY;
	}
	// Fill with types
	if (!err) {
		BMessage &msg = **result;
		std::set<std::string>::const_iterator i;
		for (i = set.begin(); i != set.end(); i++)
			msg.AddString("types", (*i).c_str());
	}
	return err;		
}

status_t
MimeDatabaseBackend::BuildInstalledSupertypesMessage(const std::map<std::string, installed_subtypes_info> &map, BMessage **result)
{
	status_t err = result ? B_OK : B_BAD_VALUE;
	// Alloc the message
	if (!err) {
		*result = new BMessage;
		if (!(*result))
			err = B_NO_MEMORY;
	}
	// Fill with types
	if (!err) {
		BMessage &msg = **result;
		std::map<std::string, installed_subtypes_info>::const_iterator i;
		for (i = map.begin(); i != map.end(); i++)
			msg.AddString("super_types", (i->first).c_str());
	}
	return err;		
}


status_t
MimeDatabaseBackend::BuildInstalledTypesList(const char *supertype, BMessage **types)
{
	return B_ERROR;
	// I'm not going to work on this function anymore until we
	// get to optimization, as BuildInstalledTypesList(Bmessage**)
	// handles this already (just not as efficiently if we only
	// want to build the types for a single supertype). I have
	// no idea if this function works, as I've never tried it.

	BMessage *msg;
	if (types) {
		*types = new BMessage;
		msg = *types;
	} else
		msg = NULL;

	status_t err = supertype && BMimeType::IsValid(supertype) ? B_OK : B_BAD_VALUE;

	// Check that this entry is both a directory and a valid MIME string
	if (!err)
	{
		// Make sure our string is all lowercase
		BPrivate::Storage::to_lower(supertype);
					
		// Clear any previous subtype info struct
		fSupertypesMap[supertype] = installed_subtypes_info();
		installed_subtypes_info &info = fSupertypesMap[supertype];
					
		// Iterate through the supertype directory and add
		// all of its subtypes
		BDirectory super;
		if (super.SetTo((MimeDatabase::kDatabaseDir + supertype).c_str()) == B_OK) {
			super.Rewind();
			while (true) {
				BEntry entry;						
				err = super.GetNextEntry(&entry);
				if (err) {
					// If we've come to the end of list, it's not an error
					if (err == B_ENTRY_NOT_FOUND) 
						err = B_OK;
					break;
				} else {																	
					// Get the subtype's name
					char subtype[B_PATH_NAME_LENGTH+1];
					char mimetype[B_PATH_NAME_LENGTH+1];
					if (entry.GetName(subtype) == B_OK) {
						BPrivate::Storage::to_lower(subtype);
						sprintf(mimetype, "%s/%s", supertype, subtype);
					
						// Add the subtype
						if (BMimeType::IsValid(mimetype)) {
							fInstalledTypes.insert(mimetype);
							info.subtypes.insert(mimetype);
							if (msg)
								msg->AddString("types", mimetype);
						}
					}
				}	
			}
		} else {
			DBG(OUT("MimeDatabaseBackend::BuildInstalledTypesList(): "
			          "Failed opening supertype directory '%s'\n",
			            supertype));
		}
	}
}
*/


} // namespace Mime
} // namespace Storage
} // namespace BPrivate

