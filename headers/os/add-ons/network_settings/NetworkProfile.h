/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_PROFILE_H
#define _NETWORK_PROFILE_H


#include <Entry.h>
#include <Path.h>


namespace BNetworkKit {


class BNetworkProfile {
public:
								BNetworkProfile();
								BNetworkProfile(const char* path);
								BNetworkProfile(const entry_ref& ref);
								BNetworkProfile(const BEntry& entry);
	virtual 					~BNetworkProfile();

			status_t			SetTo(const char* path);
			status_t			SetTo(const entry_ref& ref);
			status_t			SetTo(const BEntry& entry);

			bool				Exists();

			const char*			Name();
			status_t			SetName(const char* name);

			bool				IsDefault();
			bool				IsCurrent();

			status_t			MakeCurrent();
			status_t			Delete();

	static	BNetworkProfile*	Default();
	static	BNetworkProfile*	Current();

private:
			BEntry				fEntry;
			BPath				fPath;
			bool				fIsDefault;
			bool				fIsCurrent;
			const char*			fName;

	static	BDirectory*			fProfilesRoot;
};


}	// namespace BNetworkKit


#endif // _NETWORK_PROFILE_H
