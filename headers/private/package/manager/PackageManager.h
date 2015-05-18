/*
 * Copyright 2013-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */
#ifndef _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_
#define _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_


#include <map>
#include <string>

#include <Directory.h>
#include <ObjectList.h>
#include <package/Context.h>
#include <package/PackageDefs.h>
#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverRepository.h>

#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>
#include <package/Job.h>


namespace BPackageKit {


class BCommitTransactionResult;


namespace BManager {

namespace BPrivate {


using BPackageKit::BPrivate::BActivationTransaction;
using BPackageKit::BPrivate::BDaemonClient;


class BPackageManager : protected BSupportKit::BJobStateListener {
public:
			class RemoteRepository;
			class LocalRepository;
			class MiscLocalRepository;
			class InstalledRepository;
			class Transaction;
			class InstallationInterface;
			class ClientInstallationInterface;
			class UserInteractionHandler;

			typedef BObjectList<RemoteRepository> RemoteRepositoryList;
			typedef BObjectList<InstalledRepository> InstalledRepositoryList;
			typedef BObjectList<BSolverPackage> PackageList;
			typedef BObjectList<Transaction> TransactionList;

			enum {
				B_ADD_INSTALLED_REPOSITORIES	= 0x01,
				B_ADD_REMOTE_REPOSITORIES		= 0x02,
				B_REFRESH_REPOSITORIES			= 0x04,
			};

public:
								BPackageManager(
									BPackageInstallationLocation location,
									InstallationInterface*
										installationInterface,
									UserInteractionHandler*
										userInteractionHandler);
	virtual						~BPackageManager();

			void				Init(uint32 flags);

			void				SetDebugLevel(int32 level);
									// 0 - 10 (passed to libsolv)

			BSolver*			Solver() const
									{ return fSolver; }

			const InstalledRepository* SystemRepository() const
									{ return fSystemRepository; }
			const InstalledRepository* HomeRepository() const
									{ return fHomeRepository; }
			const InstalledRepositoryList& InstalledRepositories() const
									{ return fInstalledRepositories; }
			const RemoteRepositoryList& OtherRepositories() const
									{ return fOtherRepositories; }

			void				Install(const char* const* packages,
									int packageCount);
			void				Install(const BSolverPackageSpecifierList&
									packages);
			void				Uninstall(const char* const* packages,
									int packageCount);
			void				Uninstall(const BSolverPackageSpecifierList&
									packages);
			void				Update(const char* const* packages,
									int packageCount);
			void				Update(const BSolverPackageSpecifierList&
									packages);
			void				FullSync();

			void				VerifyInstallation();


	virtual	status_t			DownloadPackage(const BString& fileURL,
									const BEntry& targetEntry,
									const BString& checksum);
	virtual	status_t			RefreshRepository(
									const BRepositoryConfig& repoConfig);

protected:
			InstalledRepository& InstallationRepository();

protected:
			// BJobStateListener
	virtual	void				JobStarted(BSupportKit::BJob* job);
	virtual	void				JobProgress(BSupportKit::BJob* job);
	virtual	void				JobSucceeded(BSupportKit::BJob* job);

private:
			void				_HandleProblems();
			void				_AnalyzeResult();
			void				_ConfirmChanges(bool fromMostSpecific = false);
			void				_ApplyPackageChanges(
									bool fromMostSpecific = false);
			void				_PreparePackageChanges(
									InstalledRepository&
										installationRepository);
			void				_CommitPackageChanges(Transaction& transaction);

			void				_ClonePackageFile(
									LocalRepository* repository,
									BSolverPackage* package,
							 		const BEntry& entry);
			int32				_FindBasePackage(const PackageList& packages,
									const BPackageInfo& info);

			void				_AddInstalledRepository(
									InstalledRepository* repository);
			void				_AddRemoteRepository(BPackageRoster& roster,
									const char* name, bool refresh);
			status_t			_GetRepositoryCache(BPackageRoster& roster,
									const BRepositoryConfig& config,
									bool refresh, BRepositoryCache& _cache);

			void				_AddPackageSpecifiers(
									const char* const* searchStrings,
									int searchStringCount,
									BSolverPackageSpecifierList& specifierList);
			bool				_IsLocalPackage(const char* fileName);
			BSolverPackage*		_AddLocalPackage(const char* fileName);

			bool				_NextSpecificInstallationLocation();

protected:
			int32				fDebugLevel;
			BPackageInstallationLocation fLocation;
			BSolver*			fSolver;
			InstalledRepository* fSystemRepository;
			InstalledRepository* fHomeRepository;
			InstalledRepositoryList fInstalledRepositories;
			RemoteRepositoryList fOtherRepositories;
			MiscLocalRepository* fLocalRepository;
			TransactionList		fTransactions;

			// must be set by the derived class
			InstallationInterface* fInstallationInterface;
			UserInteractionHandler* fUserInteractionHandler;
};


class BPackageManager::RemoteRepository : public BSolverRepository {
public:
								RemoteRepository(
									const BRepositoryConfig& config);

			const BRepositoryConfig& Config() const;

private:
			BRepositoryConfig	fConfig;
};


class BPackageManager::LocalRepository : public BSolverRepository {
public:
								LocalRepository();
								LocalRepository(const BString& name);

	virtual	void				GetPackagePath(BSolverPackage* package,
									BPath& _path) = 0;
};


class BPackageManager::MiscLocalRepository : public LocalRepository {
public:
								MiscLocalRepository();

			BSolverPackage*		AddLocalPackage(const char* fileName);

	virtual	void				GetPackagePath(BSolverPackage* package,
									BPath& _path);

private:
			typedef std::map<BSolverPackage*, std::string> PackagePathMap;
private:
			PackagePathMap		fPackagePaths;
};


class BPackageManager::InstalledRepository : public LocalRepository {
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

	virtual	void				GetPackagePath(BSolverPackage* package,
									BPath& _path);

			void				DisablePackage(BSolverPackage* package);
									// throws, if already disabled
			bool				EnablePackage(BSolverPackage* package);
									// returns whether it was disabled

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


class BPackageManager::Transaction {
public:
								Transaction(InstalledRepository& repository);
								~Transaction();

			InstalledRepository& Repository()
									{ return fRepository; }
			BActivationTransaction& ActivationTransaction()
									{ return fTransaction; }
			BDirectory&			TransactionDirectory()
									{ return fTransactionDirectory; }

private:
			InstalledRepository& fRepository;
			BActivationTransaction fTransaction;
			BDirectory			fTransactionDirectory;
};


class BPackageManager::InstallationInterface {
public:
	virtual						~InstallationInterface();

	virtual	void				InitInstalledRepository(
									InstalledRepository& repository) = 0;
	virtual	void				ResultComputed(InstalledRepository& repository);

	virtual	status_t			PrepareTransaction(Transaction& transaction)
									= 0;
	virtual	status_t			CommitTransaction(Transaction& transaction,
									BCommitTransactionResult& _result) = 0;
};


class BPackageManager::ClientInstallationInterface
	: public InstallationInterface {
public:
								ClientInstallationInterface();
	virtual						~ClientInstallationInterface();

	virtual	void				InitInstalledRepository(
									InstalledRepository& repository);

	virtual	status_t			PrepareTransaction(Transaction& transaction);
	virtual	status_t			CommitTransaction(Transaction& transaction,
									BCommitTransactionResult& _result);

private:
			BDaemonClient		fDaemonClient;
};


class BPackageManager::UserInteractionHandler {
public:
	virtual						~UserInteractionHandler();

	virtual	void				HandleProblems();
	virtual	void				ConfirmChanges(bool fromMostSpecific);

	virtual	void				Warn(status_t error, const char* format, ...);

	virtual	void				ProgressPackageDownloadStarted(
									const char* packageName);
	virtual	void				ProgressPackageDownloadActive(
									const char* packageName,
									float completionPercentage,
									off_t bytes, off_t totalBytes);
	virtual	void				ProgressPackageDownloadComplete(
									const char* packageName);
	virtual	void				ProgressPackageChecksumStarted(
									const char* title);
	virtual	void				ProgressPackageChecksumComplete(
									const char* title);

	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository);
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const BCommitTransactionResult& result);
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository);
};


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit


#endif	// _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_
