/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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


class PackageLocalInfoBuilder;


/*!	Instances of this class should not be created directly; instead use the
	PackageLocalInfoBuilder class as a builder-constructor.
*/
class PackageLocalInfo : public BReferenceable
{
friend class PackageLocalInfoBuilder;

public:
								PackageLocalInfo();
								PackageLocalInfo(const PackageLocalInfo& other);
	virtual						~PackageLocalInfo();

			bool				operator==(const PackageLocalInfo& other) const;
			bool				operator!=(const PackageLocalInfo& other) const;

			bool				IsLocalFile() const;
			bool				Viewed() const;
			const BString&		LocalFilePath() const;
			const BString&		FileName() const;
			off_t				Size() const;
			int32				Flags() const;
			bool				IsSystemPackage() const;
			bool				IsSystemDependency() const;
			PackageState		State() const;
			bool				HasInstallationLocation(int32 location) const;
			int32				CountInstallationLocations() const;
			const PackageInstallationLocationSet&
								InstallationLocations() const;
			float				DownloadProgress() const;

private:
			void				SetViewed();
			void				SetLocalFilePath(const char* path);
			void				SetFileName(const BString& value);
			void				SetSize(off_t size);
			void				SetFlags(int32 value);
			void				SetSystemDependency(bool isDependency);
			void				SetState(PackageState state);
			void				AddInstallationLocation(int32 location);
			void				SetDownloadProgress(float progress);

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


class PackageLocalInfoBuilder
{
public:
								PackageLocalInfoBuilder();
								PackageLocalInfoBuilder(const PackageLocalInfoRef& value);
	virtual						~PackageLocalInfoBuilder();

			PackageLocalInfoRef	BuildRef();

			PackageLocalInfoBuilder&
								WithViewed();
			PackageLocalInfoBuilder&
								WithLocalFilePath(const char* path);
			PackageLocalInfoBuilder&
								WithFileName(const BString& value);
			PackageLocalInfoBuilder&
								WithSize(off_t size);
			PackageLocalInfoBuilder&
								WithFlags(int32 value);
			PackageLocalInfoBuilder&
								WithSystemDependency(bool isDependency);
			PackageLocalInfoBuilder&
								WithState(PackageState state);
			PackageLocalInfoBuilder&
								WithDownloadProgress(float progress);

			PackageLocalInfoBuilder&
								AddInstallationLocation(int32 location);
			PackageLocalInfoBuilder&
								ClearInstallationLocations();

private:
			void				_InitFromSource();
			void				_Init(const PackageLocalInfo* value);

private:
			PackageLocalInfoRef	fSource;
			bool				fViewed;
			BString				fLocalFilePath;
			BString				fFileName;
			off_t				fSize;
			int32				fFlags;
			bool				fSystemDependency;
				// When true, this value indicates that the package is a dependency of the
				// system and so cannot be uninstalled.
			PackageState		fState;
			PackageInstallationLocationSet
								fInstallationLocations;
			float				fDownloadProgress;
};


#endif // PACKAGE_LOCAL_INFO_H
