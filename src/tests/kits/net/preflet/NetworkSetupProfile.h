#ifndef NETWORKSETUPPROFILE_H
#define NETWORKSETUPPROFILE_H

#include <StorageKit.h>

class NetworkSetupProfile {
public:
						NetworkSetupProfile();
						NetworkSetupProfile(const char *path);
						NetworkSetupProfile(const entry_ref *ref);
						NetworkSetupProfile(BEntry *entry);

virtual 				~NetworkSetupProfile();
		
		status_t		SetTo(const char *path);
		status_t		SetTo(const entry_ref *ref);
		status_t		SetTo(BEntry *entry);
		
		bool			Exists();
		
		const char *	Name();
		status_t		SetName(const char *name);
		
		
		bool			IsDefault();
		bool			IsCurrent();

		status_t		MakeCurrent();
		status_t		Delete();

static	NetworkSetupProfile *	Default();
static	NetworkSetupProfile *	Current();

private:
		BEntry *		root;
		BPath *			path;
		bool			is_default;
		bool			is_current;
		const char *	name;

static	BDirectory *	profiles_root;
};


#endif // ifdef NETWORKSETUPPROFILE_H

