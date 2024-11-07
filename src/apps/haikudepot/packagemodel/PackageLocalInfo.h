/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LOCAL_INFO_H
#define PACKAGE_LOCAL_INFO_H


#include <sys/types.h>

#include <set>

#include <Referenceable.h>
#include <String.h>


typedef std::set<int32> PackageInstallationLocationSet;


enum PackageState {
	NONE		= 0,
	INSTALLED	= 1,
	DOWNLOADING	= 2,
	ACTIVATED	= 3,
	UNINSTALLED	= 4,
	PENDING		= 5,
};


class PackageLocalInfo : public BReferenceable
{
public:
								PackageLocalInfo();
								PackageLocalInfo(const PackageLocalInfo& other);
	virtual						~PackageLocalInfo();

			PackageLocalInfo&	operator=(const PackageLocalInfo& other);
			bool				operator==(const PackageLocalInfo& other) const;
			bool				operator!=(const PackageLocalInfo& other) const;

			bool				IsLocalFile() const;

			void				SetViewed();
			bool				Viewed() const
									{ return fViewed; }

			void				SetLocalFilePath(const char* path);
			const BString&		LocalFilePath() const
									{ return fLocalFilePath; }

			void				SetFileName(const BString& value);
			const BString&		FileName() const
									{ return fFileName; }

			void				SetSize(off_t size);
			off_t				Size() const
									{ return fSize; }

			void				SetFlags(int32 value);
			int32				Flags() const
									{ return fFlags; }

			bool				IsSystemPackage() const;

			void				SetSystemDependency(bool isDependency);
			bool				IsSystemDependency() const
									{ return fSystemDependency; }

			void				SetState(PackageState state);
			PackageState		State() const
									{ return fState; }

			void				AddInstallationLocation(int32 location);
			const PackageInstallationLocationSet&
								InstallationLocations() const
									{ return fInstallationLocations; }
			void				ClearInstallationLocations();

			void				SetDownloadProgress(float progress);
			float				DownloadProgress() const
									{ return fDownloadProgress; }

private:
			bool				fViewed;
			BString				fLocalFilePath;
			BString				fFileName;
			off_t				fSize;
			int32				fFlags;
			bool				fSystemDependency;
			PackageState		fState;
			PackageInstallationLocationSet
								fInstallationLocations;
			float				fDownloadProgress;
};


typedef BReference<PackageLocalInfo> PackageLocalInfoRef;


#endif // PACKAGE_LOCAL_INFO_H
