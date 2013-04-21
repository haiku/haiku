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
			struct Repository;
			typedef BObjectList<Repository> RepositoryList;

public:
								PackageManager(
									BPackageInstallationLocation location,
									bool addInstalledRepositories,
									bool addOtherRepositories);
								~PackageManager();

			BSolver*			Solver() const
									{ return fSolver; }

			const BSolverRepository* SystemRepository() const
									{ return &fSystemRepository; }
			const BSolverRepository* CommonRepository() const
									{ return &fCommonRepository; }
			const BSolverRepository* HomeRepository() const
									{ return &fHomeRepository; }
			const BObjectList<BSolverRepository>& InstalledRepositories() const
									{ return fInstalledRepositories; }
			const RepositoryList& OtherRepositories() const
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

private:
			BPackageInstallationLocation fLocation;
			BSolver*			fSolver;
			BSolverRepository	fSystemRepository;
			BSolverRepository	fCommonRepository;
			BSolverRepository	fHomeRepository;
			BObjectList<BSolverRepository> fInstalledRepositories;
			RepositoryList		fOtherRepositories;
			DecisionProvider	fDecisionProvider;
			JobStateListener	fJobStateListener;
			BContext			fContext;
			PackageList			fPackagesToActivate;
			PackageList			fPackagesToDeactivate;
};


struct PackageManager::Repository : public BSolverRepository {
								Repository();

			status_t			Init(BPackageRoster& roster, BContext& context,
									const char* name);

			const BRepositoryConfig& Config() const;

private:
			BRepositoryConfig	fConfig;
};


#endif	// PACKAGE_MANAGER_H
