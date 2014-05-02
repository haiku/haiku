/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__INSTALLATION_LOCATION_INFO_H_
#define _PACKAGE__INSTALLATION_LOCATION_INFO_H_


#include <Node.h>
#include <String.h>

#include <package/PackageDefs.h>
#include <package/PackageInfoSet.h>


namespace BPackageKit {


class BInstallationLocationInfo {
public:
								BInstallationLocationInfo();
								~BInstallationLocationInfo();

			void				Unset();

			BPackageInstallationLocation Location() const;
			void				SetLocation(
									BPackageInstallationLocation location);

			const node_ref&		BaseDirectoryRef() const;
			status_t			SetBaseDirectoryRef(const node_ref& ref);

			const node_ref&		PackagesDirectoryRef() const;
			status_t			SetPackagesDirectoryRef(const node_ref& ref);

			const BPackageInfoSet& LatestActivePackageInfos() const;
			void				SetLatestActivePackageInfos(
									const BPackageInfoSet& infos);

			const BPackageInfoSet& LatestInactivePackageInfos() const;
			void				SetLatestInactivePackageInfos(
									const BPackageInfoSet& infos);

			const BPackageInfoSet& CurrentlyActivePackageInfos() const;
			void				SetCurrentlyActivePackageInfos(
									const BPackageInfoSet& infos);

			const BString&		OldStateName() const;
			void				SetOldStateName(const BString& name);

			int64				ChangeCount() const;
			void				SetChangeCount(int64 changeCount);

private:
			BPackageInstallationLocation fLocation;
			node_ref			fBaseDirectoryRef;
			node_ref			fPackageDirectoryRef;
			BPackageInfoSet		fLatestActivePackageInfos;
			BPackageInfoSet		fLatestInactivePackageInfos;
			BPackageInfoSet		fCurrentlyActivePackageInfos;
			BString				fOldStateName;
			int64				fChangeCount;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__INSTALLATION_LOCATION_INFO_H_
