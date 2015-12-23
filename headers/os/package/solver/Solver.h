/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_H_
#define _PACKAGE__SOLVER_H_


#include <ObjectList.h>
#include <SupportDefs.h>


namespace BPackageKit {


class BSolverPackage;
class BSolverPackageSpecifier;
class BSolverPackageSpecifierList;
class BSolverProblem;
class BSolverProblemSolution;
class BSolverRepository;
class BSolverResult;


class BSolver {
public:
			// FindPackages() flags
			enum {
				B_FIND_CASE_INSENSITIVE		= 0x01,
				B_FIND_IN_NAME				= 0x02,
				B_FIND_IN_SUMMARY			= 0x04,
				B_FIND_IN_DESCRIPTION		= 0x08,
				B_FIND_IN_PROVIDES			= 0x10,
				B_FIND_INSTALLED_ONLY		= 0x20,
				B_FIND_IN_REQUIRES			= 0x40,
			};

			// VerifyInstallation() flags
			enum {
				B_VERIFY_ALLOW_UNINSTALL	= 0x01,
			};

public:
	virtual						~BSolver();

	static	status_t			Create(BSolver*& _solver);

	virtual	status_t			Init() = 0;

	virtual	void				SetDebugLevel(int32 level) = 0;

	virtual	status_t			AddRepository(
									BSolverRepository* repository) = 0;

	virtual	status_t			FindPackages(const char* searchString,
									uint32 flags,
									BObjectList<BSolverPackage>& _packages) = 0;
	virtual	status_t			FindPackages(
									const BSolverPackageSpecifierList& packages,
									uint32 flags,
									BObjectList<BSolverPackage>& _packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL) = 0;

	virtual	status_t			Install(
									const BSolverPackageSpecifierList& packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL) = 0;
	virtual	status_t			Uninstall(
									const BSolverPackageSpecifierList& packages,
									const BSolverPackageSpecifier** _unmatched
										= NULL) = 0;
	virtual	status_t			Update(
									const BSolverPackageSpecifierList& packages,
									bool installNotYetInstalled,
									const BSolverPackageSpecifier** _unmatched
										= NULL) = 0;
	virtual	status_t			FullSync() = 0;
	virtual	status_t			VerifyInstallation(uint32 flags = 0) = 0;

			bool				HasProblems() const
									{ return CountProblems() > 0; }
	virtual	int32				CountProblems() const = 0;
	virtual	BSolverProblem*		ProblemAt(int32 index) const = 0;

	virtual	status_t			SelectProblemSolution(
									BSolverProblem* problem,
									const BSolverProblemSolution* solution) = 0;
	virtual	status_t			SolveAgain() = 0;

	virtual	status_t			GetResult(BSolverResult& _result) = 0;

protected:
								BSolver();
};


// function exported by the libsolv based add-on
extern "C" BSolver* create_solver();


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_H_
