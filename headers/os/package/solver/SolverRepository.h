/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_REPOSITORY_H_
#define _PACKAGE__SOLVER_REPOSITORY_H_


#include <ObjectList.h>
#include <package/PackageDefs.h>
#include <package/PackageInfoSet.h>
#include <String.h>


namespace BPackageKit {


class BPackageInfo;
class BRepositoryCache;
class BRepositoryConfig;
class BSolverPackage;


class BSolverRepository {
public:
			enum BAllInstallationLocations {
				B_ALL_INSTALLATION_LOCATIONS
			};

			typedef BPackageInfoSet::Iterator Iterator;

public:
								BSolverRepository();
								BSolverRepository(const BString& name);
								BSolverRepository(
									BPackageInstallationLocation location);
								BSolverRepository(BAllInstallationLocations);
								BSolverRepository(
									const BRepositoryConfig& config);
	virtual						~BSolverRepository();

			status_t			SetTo(const BString& name);
			status_t			SetTo(BPackageInstallationLocation location);
			status_t			SetTo(BAllInstallationLocations);
			status_t			SetTo(const BRepositoryConfig& config);
			status_t			SetTo(const BRepositoryCache& cache);
			void				Unset();

			status_t			InitCheck();

			bool				IsInstalled() const;
			void				SetInstalled(bool isInstalled);

			BString				Name() const;

			int32				Priority() const;
			void				SetPriority(int32 priority);
									// negative priority is fine

			bool				IsEmpty() const;
			int32				CountPackages() const;
			BSolverPackage*		PackageAt(int32 index) const;

			status_t			AddPackage(const BPackageInfo& info,
									BSolverPackage** _package = NULL);
			status_t			AddPackages(
									BPackageInstallationLocation location);
			bool				RemovePackage(BSolverPackage* package);
			bool				DeletePackage(BSolverPackage* package);

			uint64				ChangeCount() const;

private:
			typedef BObjectList<BSolverPackage> PackageList;

private:
			BString				fName;
			int32				fPriority;
			bool				fIsInstalled;
			PackageList			fPackages;
			uint64				fChangeCount;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_REPOSITORY_H_
