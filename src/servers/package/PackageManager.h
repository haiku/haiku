/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H


#include <map>
#include <set>

#include <package/Context.h>
#include <package/Job.h>

#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>


using BPackageKit::BContext;
using BPackageKit::BDecisionProvider;
using BPackageKit::BJob;
using BPackageKit::BJobStateListener;
using BPackageKit::BPackageInstallationLocation;
using BPackageKit::BRepositoryConfig;
using BPackageKit::BSolverPackage;

using BPackageKit::BPrivate::BDaemonClient;
using BPackageKit::BManager::BPrivate::BPackageManager;


class Package;
class ProblemWindow;
class Root;
class Volume;


class PackageManager : public BPackageManager,
	private BPackageManager::InstallationInterface,
	private BPackageManager::RequestHandler,
	private BPackageManager::UserInteractionHandler,
	private BDecisionProvider, private BJobStateListener {
public:
								PackageManager(Root* root, Volume* volume);
								~PackageManager();

			void				HandleUserChanges();

private:
	// InstallationInterface
	virtual	void				InitInstalledRepository(
									InstalledRepository& repository);
	virtual	void				ResultComputed(InstalledRepository& repository);

	virtual	status_t			PrepareTransaction(Transaction& transaction);
	virtual	status_t			CommitTransaction(Transaction& transaction,
									BDaemonClient::BCommitTransactionResult&
										_result);

private:
	// RequestHandler
	virtual	status_t			RefreshRepository(
									const BRepositoryConfig& repoConfig);
	virtual	status_t			DownloadPackage(const BString& fileURL,
                                    const BEntry& targetEntry,
                                    const BString& checksum);

private:
	// UserInteractionHandler
	virtual	void				HandleProblems();
	virtual	void				ConfirmChanges(bool fromMostSpecific);

	virtual	void				Warn(status_t error, const char* format, ...);
	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository);
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const char* transactionDirectoryName);
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository);

private:
	// BDecisionProvider
	virtual	bool				YesNoDecisionNeeded(const BString& description,
									const BString& question,
									const BString& yes, const BString& no,
									const BString& defaultChoice);

private:
	// BJobStateListener
	virtual	void				JobFailed(BJob* job);
	virtual	void				JobAborted(BJob* job);

private:
			typedef std::set<BSolverPackage*> SolverPackageSet;
			typedef std::map<Package*, BSolverPackage*> SolverPackageMap;

private:
			bool				_GetResultText(InstalledRepository& repository,
									BString& _text);

			BSolverPackage*		_SolverPackageFor(Package* package) const;

			void				_InitGui();

private:
			Root*				fRoot;
			Volume*				fVolume;
			BContext			fContext;
			SolverPackageMap	fSolverPackages;
			SolverPackageSet	fPackagesAddedByUser;
			SolverPackageSet	fPackagesRemovedByUser;
			ProblemWindow*		fProblemWindow;
};


#endif	// PACKAGE_MANAGER_H
