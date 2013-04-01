/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_
#define _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_


#include <package/PackageResolvableExpression.h>


namespace BPackageKit {


class BSolverRepository;


class BSolverPackageSpecifier {
public:
								BSolverPackageSpecifier();
								BSolverPackageSpecifier(
									const BPackageResolvableExpression&
										expression);
								BSolverPackageSpecifier(
									BSolverRepository* repository,
									const BPackageResolvableExpression&
										expression);
								BSolverPackageSpecifier(
									const BSolverPackageSpecifier& other);
								~BSolverPackageSpecifier();

			BSolverRepository*	Repository() const;
			const BPackageResolvableExpression& Expression() const;

			BSolverPackageSpecifier& operator=(
									const BSolverPackageSpecifier& other);

private:
			BSolverRepository*	fRepository;
			BPackageResolvableExpression fExpression;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_
