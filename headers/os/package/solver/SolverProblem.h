/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PROBLEM_H_
#define _PACKAGE__SOLVER_PROBLEM_H_


#include <ObjectList.h>
#include <package/PackageResolvableExpression.h>


namespace BPackageKit {


class BSolverPackage;
class BSolverProblemSolution;


class BSolverProblem {
public:
			enum BType {
				B_UNSPECIFIED,
				B_NOT_IN_DISTUPGRADE_REPOSITORY,
				B_INFERIOR_ARCHITECTURE,
				B_INSTALLED_PACKAGE_PROBLEM,
				B_CONFLICTING_REQUESTS,
				B_REQUESTED_RESOLVABLE_NOT_PROVIDED,
				B_REQUESTED_RESOLVABLE_PROVIDED_BY_SYSTEM,
				B_DEPENDENCY_PROBLEM,
				B_PACKAGE_NOT_INSTALLABLE,
				B_DEPENDENCY_NOT_PROVIDED,
				B_PACKAGE_NAME_CLASH,
				B_PACKAGE_CONFLICT,
				B_PACKAGE_OBSOLETES_RESOLVABLE,
				B_INSTALLED_PACKAGE_OBSOLETES_RESOLVABLE,
				B_PACKAGE_IMPLICITLY_OBSOLETES_RESOLVABLE,
				B_DEPENDENCY_NOT_INSTALLABLE,
				B_SELF_CONFLICT
			};

public:
								BSolverProblem(BType type,
									BSolverPackage* sourcePackage,
									BSolverPackage* targetPackage = NULL);
								BSolverProblem(BType type,
									BSolverPackage* sourcePackage,
									BSolverPackage* targetPackage,
									const BPackageResolvableExpression&
										dependency);
								~BSolverProblem();

			BType				Type() const;
			BSolverPackage*		SourcePackage() const;
			BSolverPackage*		TargetPackage() const;
			const BPackageResolvableExpression& Dependency() const;

			int32				CountSolutions() const;
			const BSolverProblemSolution* SolutionAt(int32 index) const;

			bool				AppendSolution(
									BSolverProblemSolution* solution);

			BString				ToString() const;

private:
			typedef BObjectList<BSolverProblemSolution> SolutionList;

private:
			BType				fType;
			BSolverPackage*		fSourcePackage;
			BSolverPackage*		fTargetPackage;
			BPackageResolvableExpression fDependency;
			SolutionList		fSolutions;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PROBLEM_H_
