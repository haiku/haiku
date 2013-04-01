/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_H_
#define _PACKAGE__SOLVER_H_


#include <SupportDefs.h>


namespace BPackageKit {


class BSolverPackageSpecifierList;
class BSolverRepository;


class BSolver {
public:
								BSolver();
								~BSolver();

			status_t			Init();

			status_t			AddRepository(BSolverRepository* repository);

			status_t			Install(
									const BSolverPackageSpecifierList&
										packages);

private:
			class Implementation;

private:
			Implementation*		fImplementation;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_H_
