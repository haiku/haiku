/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEINFO_H
#define PACKAGEINFO_H

#include "PackageItem.h"
#include <List.h>
#include <String.h>
#include <File.h>
#include <DataIO.h>
#include <Path.h>


struct pkg_profile;


class PackageInfo {
	public:
		PackageInfo();
		PackageInfo(const entry_ref *ref);
		~PackageInfo();

		const char *GetName() { return fName.String(); }
		const char *GetDescription() { return fDescription.String(); }
		const char *GetShortDescription() { return fShortDesc.String(); }
		const char *GetVersion() { return fVersion.String(); }
		const char *GetDisclaimer() { return fDisclaimer.String(); }
		BMallocIO *GetSplashScreen() { return fHasImage ? &fImage : NULL; }
		int32 GetProfileCount() { return fProfiles.CountItems(); }
		pkg_profile *GetProfile(int32 num) {
			return static_cast<pkg_profile *>(fProfiles.ItemAt(num)); }
		int32 GetScriptCount() { return fScripts.CountItems(); }
		PackageScript *GetScript(int32 num) {
			return static_cast<PackageScript *>(fScripts.ItemAt(num)); }
		
		status_t Parse();
		status_t InitCheck() { return fStatus; }

	private:
		void _AddItem(PackageItem *item, uint64 size, uint32 groups,
			uint32 path, uint32 cust);

		status_t fStatus;

		BFile *fPackageFile;
		BString fName;
		BString fDescription;
		BList fProfiles;

		BString fShortDesc;
		BString fDeveloper;
		BString fVersion;
		BString fDisclaimer;
		BMallocIO fImage;
		bool fHasImage;

		BList fFiles; // Holds all files in the package
		BList fScripts;
};


// #pragma mark -


struct pkg_profile {
	pkg_profile() : items(10), space_needed(0), path_type(P_SYSTEM_PATH) {}
	~pkg_profile() {} 

	BString name;
	BString description;
	BList items;
	uint64 space_needed;

	uint8 path_type;
};

#endif

