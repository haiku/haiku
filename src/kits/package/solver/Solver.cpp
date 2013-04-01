/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/Solver.h>

#include <new>

#include <solv/pool.h>
#include <solv/poolarch.h>
#include <solv/repo.h>
#include <solv/repo_haiku.h>
#include <solv/selection.h>
#include <solv/solver.h>

#include <package/RepositoryCache.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverRepository.h>

#include <ObjectList.h>


// TODO: libsolv doesn't have any helpful out-of-memory handling. It just just
// abort()s. Obviously that isn't good behavior for a library.


namespace BPackageKit {


// #pragma mark - BSolver::Implementation


class BSolver::Implementation {
public:
								Implementation();
								~Implementation();

			status_t			Init();

			status_t			AddRepository(BSolverRepository* repository);

			status_t			Install(
									const BSolverPackageSpecifierList&
										packages);

private:
			struct SolvQueue;
			struct RepositoryInfo;

			typedef BObjectList<RepositoryInfo> RepositoryInfoList;

private:
			status_t			_AddRepositories();
			RepositoryInfo*		_GetRepositoryInfo(
									BSolverRepository* repository) const;

private:
			Pool*				fPool;
			RepositoryInfoList	fRepositoryInfos;
			RepositoryInfo*		fInstalledRepository;
};


struct BSolver::Implementation::SolvQueue : Queue {
	SolvQueue()
	{
		queue_init(this);
	}

	~SolvQueue()
	{
		queue_free(this);
	}
};


struct BSolver::Implementation::RepositoryInfo {
	RepositoryInfo(BSolverRepository* repository)
		:
		fRepository(repository),
		fSolvRepo(NULL)
	{
	}

	BSolverRepository* Repository() const
	{
		return fRepository;
	}

	Repo* SolvRepo()
	{
		return fSolvRepo;
	}

	void SetSolvRepo(Repo* repo)
	{
		fSolvRepo = repo;
	}

private:
	BSolverRepository*	fRepository;
	Repo*				fSolvRepo;
};


// #pragma mark - BSolver


BSolver::BSolver()
	:
	fImplementation(new(std::nothrow) Implementation)
{
}


BSolver::~BSolver()
{
	delete fImplementation;
}


status_t
BSolver::Init()
{
	return fImplementation != NULL ? fImplementation->Init() : B_NO_MEMORY;
}


status_t
BSolver::AddRepository(BSolverRepository* repository)
{
	return fImplementation != NULL
		? fImplementation->AddRepository(repository) : B_NO_MEMORY;
}


status_t
BSolver::Install(const BSolverPackageSpecifierList& packages)
{
	return fImplementation != NULL
		? fImplementation->Install(packages) : B_NO_MEMORY;
}


// #pragma mark - BSolver::Implementation


BSolver::Implementation::Implementation()
	:
	fPool(NULL),
	fRepositoryInfos(),
	fInstalledRepository(NULL)
{
}


BSolver::Implementation::~Implementation()
{
	if (fPool != NULL)
		pool_free(fPool);
}


status_t
BSolver::Implementation::Init()
{
	// already initialized?
	if (fPool != NULL)
		return B_BAD_VALUE;

	fPool = pool_create();

	// Set the system architecture. We use what uname() returns unless we're on
	// x86 gcc2.
	{
		const char* arch;
		#ifdef __HAIKU_ARCH_X86
			#if (B_HAIKU_ABI & B_HAIKU_ABI_MAJOR) == B_HAIKU_ABI_GCC_2
				arch = "x86_gcc2";
			#else
				arch = "x86";
			#endif
		#else
			struct utsname info;
			if (uname(&info) != 0)
				return errno;
			arch = info.machine;
		#endif

		pool_setarchpolicy(fPool, arch);
	}

	return B_OK;
}


status_t
BSolver::Implementation::AddRepository(BSolverRepository* repository)
{
	if (repository == NULL || repository->InitCheck() != B_OK)
		return B_BAD_VALUE;

	// If the repository represents installed packages, check, if we already
	// have such a repository.
	if (repository->IsInstalled() && fInstalledRepository != NULL)
		return B_BAD_VALUE;

	// add the repository info
	RepositoryInfo* info = new(std::nothrow) RepositoryInfo(repository);
	if (info == NULL)
		return B_NO_MEMORY;

	if (!fRepositoryInfos.AddItem(info)) {
		delete info;
		return B_NO_MEMORY;
	}

	if (repository->IsInstalled())
		fInstalledRepository = info;

	return B_OK;
}


status_t
BSolver::Implementation::Install(const BSolverPackageSpecifierList& packages)
{
	if (packages.IsEmpty())
		return B_BAD_VALUE;

// TODO: Clean up first?

	// add repositories to pool
	status_t error = _AddRepositories();
	if (error != B_OK)
		return error;

	// prepare pool for solving
	pool_createwhatprovides(fPool);

	// add the packages to install to the job queue
	SolvQueue jobs;

	int32 packageCount = packages.CountSpecifiers();
	for (int32 i = 0; i < packageCount; i++) {
		const BSolverPackageSpecifier& specifier = *packages.SpecifierAt(i);

		// find matching packages
		SolvQueue matchingPackages;

		int flags = SELECTION_NAME | SELECTION_PROVIDES | SELECTION_GLOB
			| SELECTION_CANON | SELECTION_DOTARCH | SELECTION_REL;
// TODO: All flags needed/useful?
		/*int matchFlags =*/ selection_make(fPool, &matchingPackages,
			specifier.Expression().Name(), flags);
// TODO: Don't just match the name, but also the version, if given!
		if (matchingPackages.count == 0)
			return B_NAME_NOT_FOUND;

		// restrict to the matching repository
		if (BSolverRepository* repository = specifier.Repository()) {
			RepositoryInfo* repositoryInfo = _GetRepositoryInfo(repository);
			if (repositoryInfo == NULL)
				return B_BAD_VALUE;

			SolvQueue repoFilter;
			queue_push2(&repoFilter,
				SOLVER_SOLVABLE_REPO/* | SOLVER_SETREPO | SOLVER_SETVENDOR*/,
				repositoryInfo->SolvRepo()->repoid);

			selection_filter(fPool, &matchingPackages, &repoFilter);

			if (matchingPackages.count == 0)
				return B_NAME_NOT_FOUND;
		}

		for (int j = 0; j < matchingPackages.count; j++)
			queue_push(&jobs, matchingPackages.elements[j]);
	}

	// add solver mode to job queue elements
	int solverMode = SOLVER_INSTALL;
	for (int i = 0; i < jobs.count; i += 2) {
		jobs.elements[i] |= solverMode;
//		if (cleandeps)
//			jobs.elements[i] |= SOLVER_CLEANDEPS;
//		if (forcebest)
//			jobs.elements[i] |= SOLVER_FORCEBEST;
	}

	// create the solver and solve
	Solver* solver = solver_create(fPool);
	solver_set_flag(solver, SOLVER_FLAG_SPLITPROVIDES, 1);
	solver_set_flag(solver, SOLVER_FLAG_BEST_OBEY_POLICY, 1);

	int problemCount = solver_solve(solver, &jobs);
	solver_free(solver);

// TODO: Problem support!
	return problemCount == 0 ? B_OK : B_BAD_VALUE;
}


status_t
BSolver::Implementation::_AddRepositories()
{
	if (fInstalledRepository == NULL)
		return B_BAD_VALUE;

	int32 repositoryCount = fRepositoryInfos.CountItems();
	for (int32 i = 0; i < repositoryCount; i++) {
		RepositoryInfo* repositoryInfo = fRepositoryInfos.ItemAt(i);
		BSolverRepository* repository = repositoryInfo->Repository();
		Repo* repo = repo_create(fPool, repository->Name());
		repositoryInfo->SetSolvRepo(repo);

		repo->priority = 256 - repository->Priority();
		repo->appdata = (void*)repositoryInfo;

		BRepositoryCache::Iterator it = repository->GetIterator();
		while (const BPackageInfo* packageInfo = it.Next()) {
			repo_add_haiku_package_info(repo, *packageInfo,
				REPO_REUSE_REPODATA | REPO_NO_INTERNALIZE);
		}

		repo_internalize(repo);

		if (repository->IsInstalled())
			pool_set_installed(fPool, repo);
	}

	return B_OK;
}


BSolver::Implementation::RepositoryInfo*
BSolver::Implementation::_GetRepositoryInfo(BSolverRepository* repository) const
{
	int32 repositoryCount = fRepositoryInfos.CountItems();
	for (int32 i = 0; i < repositoryCount; i++) {
		RepositoryInfo* repositoryInfo = fRepositoryInfos.ItemAt(i);
		if (repository == repositoryInfo->Repository())
			return repositoryInfo;
	}

	return NULL;
}


}	// namespace BPackageKit
