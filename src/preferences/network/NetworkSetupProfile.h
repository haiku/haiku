/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_SETUP_PROFILE_H
#define NETWORK_SETUP_PROFILE_H


#include <StorageKit.h>


class NetworkSetupProfile {
public:
								NetworkSetupProfile();
								NetworkSetupProfile(const char* path);
								NetworkSetupProfile(const entry_ref* ref);
								NetworkSetupProfile(BEntry* entry);

	virtual 					~NetworkSetupProfile();

			status_t			SetTo(const char* path);
			status_t			SetTo(const entry_ref* ref);
			status_t			SetTo(BEntry* entry);

			bool				Exists();

			const char*			Name();
			status_t			SetName(const char* name);

			bool				IsDefault();
			bool				IsCurrent();

			status_t			MakeCurrent();
			status_t			Delete();

	static	NetworkSetupProfile*
								Default();
	static	NetworkSetupProfile*
								Current();

private:
			BEntry*				fRoot;
			BPath*				fPath;
			bool				fIsDefault;
			bool				fIsCurrent;
			const char*			fName;

	static	BDirectory*			fProfilesRoot;
};


#endif // NETWORK_SETUP_PROFILE_H
