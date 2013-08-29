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
			typedef BObjectList<BSolverPackage> PackageList;

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
			void				_HandleProblems();
			void				_AnalyzeResult();
			void				_PrintResult(bool fromMostSpecific = false);
			void				_PrintResult(
									InstalledRepository&
										installationRepository);
			void				_ApplyPackageChanges(
									bool fromMostSpecific = false);
			void				_ApplyPackageChanges(
									InstalledRepository&
										installationRepository);

			void				_ClonePackageFile(
									InstalledRepository* repository,
									const BString& fileName,
							 		const BEntry& entry) const;
			int32				_FindBasePackage(const PackageList& packages,
									const BPackageInfo& info) const;

			InstalledRepository& _InstallationRepository();

			void				_AddInstalledRepository(
									InstalledRepository* repository);
			bool				_NextSpecificInstallationLocation();

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


class PackageManager::RemoteRepository : public BSolverRepository {
public:
								RemoteRepository();

			status_t			Init(BPackageRoster& roster, BContext& context,
									const char* name, bool refresh);

			const BRepositoryConfig& Config() const;

private:
			BRepositoryConfig	fConfig;
};


class PackageManager::InstalledRepository : public BSolverRepository {
public:
			typedef BObjectList<BSolverPackage> PackageList;

public:
								InstalledRepository(const char* name,
									BPackageInstallationLocation location,
									int32 priority);

			BPackageInstallationLocation Location() const
									{ return fLocation; }
			const char*			InitialName() const
									{ return fInitialName; }
			int32				InitialPriority() const
									{ return fInitialPriority; }

			void				DisablePackage(BSolverPackage* package);

			PackageList&		PackagesToActivate()
									{ return fPackagesToActivate; }
			PackageList&		PackagesToDeactivate()
									{ return fPackagesToDeactivate; }

			bool				HasChanges() const;
			void				ApplyChanges();

private:
			PackageList			fDisabledPackages;
			PackageList			fPackagesToActivate;
			PackageList			fPackagesToDeactivate;
			const char*			fInitialName;
			BPackageInstallationLocation fLocation;
			int32				fInitialPriority;
};


#endif	// PACKAGE_MANAGER_H
