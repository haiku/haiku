#ifndef NETWORKSETUPPROFILE_H
#define NETWORKSETUPPROFILE_H

#include <StorageKit.h>

class NetworkSetupProfile {
	public:
		NetworkSetupProfile();
		NetworkSetupProfile(const char *path);
		NetworkSetupProfile(const entry_ref *ref);
		NetworkSetupProfile(BEntry *entry);
		virtual ~NetworkSetupProfile();
		
		status_t	SetTo(const char *path);
		status_t	SetTo(const entry_ref *ref);
		status_t	SetTo(BEntry *entry);
		
		void		Unset();
		
		bool		Exists();
		
		const char *	Name();
		status_t		SetName(const char *name);

		status_t	Create();
		status_t	Delete();
		
		virtual bool		IsDefault();
		virtual bool		IsActive();
		virtual status_t	MakeActive();

	private:
		BEntry *		root;
		BPath *			path;
		bool			is_default;
		bool			is_active;
		const char *	name;
};


#endif // ifdef NETWORKSETUPPROFILE_H

