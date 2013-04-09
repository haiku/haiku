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

	virtual	status_t			AddRepository(BSolverRepository* repository);

	virtual	status_t			Install(
									const BSolverPackageSpecifierList&
										packages);
	virtual	status_t			VerifyInstallation();

	virtual	int32				CountProblems() const;
	virtual	BSolverProblem*		ProblemAt(int32 index) const;

	virtual	status_t			GetResult(BSolverResult& _result);

private:
			struct SolvQueue;
			struct RepositoryInfo;
			struct Problem;
			struct Solution;

			typedef BObjectList<RepositoryInfo> RepositoryInfoList;
			typedef BObjectList<Problem> ProblemList;
			typedef std::map<Solvable*, BSolverPackage*> SolvableMap;

private:
			void				_Cleanup();

			status_t			_AddRepositories();
			RepositoryInfo*		_GetRepositoryInfo(
									BSolverRepository* repository) const;
			BSolverPackage*		_GetPackage(Solvable* solvable) const;
			BSolverPackage*		_GetPackage(Id solvableId) const;

			status_t			_AddProblem(Id problemId);
			status_t			_AddSolution(Problem* problem, Id solutionId);
			status_t			_AddSolutionElement(Solution* solution,
									Id sourceId, Id targetId);
			status_t			_AddSolutionElement(Solution* solution,
									BSolverProblemSolutionElement::BType type,
									Solvable* sourceSolvable,
									Solvable* targetSolvable,
									const char* selectionString);
			status_t			_GetResolvableExpression(Id id,
									BPackageResolvableExpression& _expression)
									const;

			status_t			_Solve(Queue& jobs);
			void				_SetJobsSolverMode(Queue& jobs, int solverMode);

private:
			Pool*				fPool;
			Solver*				fSolver;
			RepositoryInfoList	fRepositoryInfos;
			RepositoryInfo*		fInstalledRepository;
			SolvableMap			fSolvablePackages;
			ProblemList			fProblems;
};


#endif // HAIKU_LIBSOLV_SOLVER_H
