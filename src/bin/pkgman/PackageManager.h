/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H


#include <ObjectList.h>
#include <package/Context.h>
#include <package/PackageDefs.h>
#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverRepository.h>

#include "DecisionProvider.h"
#include "JobStateListener.h"


using namespace BPackageKit;


class PackageManager {
public:
			struct RemoteRepository;
			struct InstalledRepository;
			typedef BObjectList<RemoteRepository> RemoteRepositoryList;
			typedef BObjectList<InstalledRepository> InstalledRepositoryList;

			enum {
				ADD_INSTALLED_REPOSITORIES	= 0x01,
				ADD_REMOTE_REPOSITORIES		= 0x02,
				REFRESH_REPOSITORIES		= 0x04,
			};

public:
								PackageManager(
									BPackageInstallationLocation location,
									uint32 flags);
								~PackageManager();

			BSolver*			Solver() const
									{ return fSolver; }

			const InstalledRepository* SystemRepository() const
									{ return fSystemRepository; }
			const InstalledRepository* CommonRepository() const
									{ return fCommonRepository; }
			const InstalledRepository* HomeRepository() const
									{ return fHomeRepository; }
			const InstalledRepositoryList& InstalledRepositories() const
									{ return fInstalledRepositories; }
			const RemoteRepositoryList& OtherRepositories() const
									{ return fOtherRepositories; }

			void				Install(const char* const* packages,
									int packageCount);
			void				Uninstall(const char* const* packages,
									int packageCount);
			void				Update(const char* const* packages,
									int packageCount);

private:
			typedef BObjectList<BSolverPackage> PackageList;

private:
			void				_HandleProblems();
			void				_AnalyzeResult();
			void				_PrintResult();
			void				_ApplyPackageChanges();

			void				_ClonePackageFile(
									InstalledRepository* repository,
									const BString& fileName,
							 		const BEntry& entry) const;
			int32				_FindBasePackage(const PackageList& packages,
									const BPackageInfo& info) const;

private:
			BPackageInstallationLocation fLocation;
			BSolver*			fSolver;
			InstalledRepository* fSystemRepository;
			InstalledRepository* fCommonRepository;
			InstalledRepository* fHomeRepository;
			InstalledRepositoryList fInstalledRepositories;
			RemoteRepositoryList fOtherRepositories;
			DecisionProvider	fDecisionProvider;
			JobStateListener	fJobStateListener;
			BContext			fContext;
			PackageList			fPackagesToActivate;
			PackageList			fPackagesToDeactivate;
};


struct PackageManager::RemoteRepository : public BSolverRepository {
								RemoteRepository();

			status_t			Init(BPackageRoster& roster, BContext& context,
									const char* name, bool refresh);

			const BRepositoryConfig& Config() const;

private:
			BRepositoryConfig	fConfig;
};


struct PackageManager::InstalledRepository : public BSolverRepository {
								InstalledRepository();

			void				DisablePackage(BSolverPackage* package);

private:
			typedef BObjectList<BSolverPackage> PackageList;

private:
			PackageList			fDisabledPackages;
};


#endif	// PACKAGE_MANAGER_H
