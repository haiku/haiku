/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HAIKU_LIBSOLV_SOLVER_H
#define HAIKU_LIBSOLV_SOLVER_H


#include <map>

#include <ObjectList.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverProblemSolution.h>

#include <solv/pool.h>
#include <solv/solver.h>


using namespace BPackageKit;


namespace BPackageKit {
	class BPackageResolvableExpression;
	class BSolverPackage;
}


class LibsolvSolver : public BSolver {
public:
								LibsolvSolver();
	virtual						~LibsolvSolver();

	virtual	status_t			Init();

	virtual	void				SetDebugLevel(int32 level);

	virtual	status_t			AddRepository(BSolverRepository* repository);

	virtual	status_t			FindPackages(const char* searchString,
									uint32 flags,
									BObjectList<BSolverPackage>& _packages);
	virtual	status_t			FindPackages(
									const BSolverPackageSpecifierList& packages,
									uint32 flags,
									BObjectList<BSolverPackage>& _packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL);

	virtual	status_t			Install(
									const BSolverPackageSpecifierList& packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL);
	virtual	status_t			Uninstall(
									const BSolverPackageSpecifierList& packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL);
	virtual	status_t			Update(
									const BSolverPackageSpecifierList& packages,
									bool installNotYetInstalled,
									const BSolverPackageSpecifier** _unmatched
										= NULL);
	virtual	status_t			FullSync();
	virtual	status_t			VerifyInstallation(uint32 flags = 0);

	virtual	int32				CountProblems() const;
	virtual	BSolverProblem*		ProblemAt(int32 index) const;

	virtual	status_t			SelectProblemSolution(
									BSolverProblem* problem,
									const BSolverProblemSolution* solution);
	virtual	status_t			SolveAgain();

	virtual	status_t			GetResult(BSolverResult& _result);

private:
			struct SolvQueue;
			struct SolvDataIterator;
			struct RepositoryInfo;
			struct Problem;
			struct Solution;

			typedef BObjectList<RepositoryInfo> RepositoryInfoList;
			typedef BObjectList<Problem> ProblemList;
			typedef std::map<Id, BSolverPackage*> SolvableMap;
			typedef std::map<BSolverPackage*, Id> PackageMap;

private:
			status_t			_InitPool();
			status_t			_InitJobQueue();
			void				_InitSolver();
			void				_Cleanup();
			void				_CleanupPool();
			void				_CleanupJobQueue();
			void				_CleanupSolver();

			bool				_HaveRepositoriesChanged() const;
			status_t			_AddRepositories();
			RepositoryInfo*		_InstalledRepository() const;
			RepositoryInfo*		_GetRepositoryInfo(
									BSolverRepository* repository) const;
			BSolverPackage*		_GetPackage(Id solvableId) const;
			Id					_GetSolvable(BSolverPackage* package) const;

			status_t			_AddSpecifiedPackages(
									const BSolverPackageSpecifierList& packages,
									const BSolverPackageSpecifier** _unmatched,
									int additionalFlags);

			status_t			_AddProblem(Id problemId);
			status_t			_AddSolution(Problem* problem, Id solutionId);
			status_t			_AddSolutionElement(Solution* solution,
									Id sourceId, Id targetId);
			status_t			_AddSolutionElement(Solution* solution,
									BSolverProblemSolutionElement::BType type,
									Id sourceSolvableId, Id targetSolvableId,
									const char* selectionString);
			status_t			_GetResolvableExpression(Id id,
									BPackageResolvableExpression& _expression)
									const;

			status_t			_GetFoundPackages(SolvQueue& selection,
									uint32 flags,
									BObjectList<BSolverPackage>& _packages);

			status_t			_Solve();
			void				_SetJobsSolverMode(int solverMode);

private:
			Pool*				fPool;
			Solver*				fSolver;
			SolvQueue*			fJobs;
			RepositoryInfoList	fRepositoryInfos;
			RepositoryInfo*		fInstalledRepository;
									// valid only after _AddRepositories()
			SolvableMap			fSolvablePackages;
			PackageMap			fPackageSolvables;
			ProblemList			fProblems;
			int32				fDebugLevel;
};


#endif // HAIKU_LIBSOLV_SOLVER_H
