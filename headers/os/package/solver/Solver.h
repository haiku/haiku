/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_H_
#define _PACKAGE__SOLVER_H_


#include <SupportDefs.h>


namespace BPackageKit {


class BSolverPackageSpecifierList;
class BSolverProblem;
class BSolverRepository;
class BSolverResult;


class BSolver {
public:
	virtual						~BSolver();

	static	status_t			Create(BSolver*& _solver);

	virtual	status_t			Init() = 0;

	virtual	status_t			AddRepository(
									BSolverRepository* repository) = 0;

	virtual	status_t			Install(
									const BSolverPackageSpecifierList&
										packages) = 0;
	virtual	status_t			VerifyInstallation() = 0;

			bool				HasProblems() const
									{ return CountProblems() > 0; }
	virtual	int32				CountProblems() const = 0;
	virtual	BSolverProblem*		ProblemAt(int32 index) const = 0;

	virtual	status_t			GetResult(BSolverResult& _result) = 0;

protected:
								BSolver();
};


// function exported by the libsolv based add-on
extern "C" BSolver* create_solver();


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_H_
