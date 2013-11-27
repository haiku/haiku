/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PACKAGE_H_
#define _PACKAGE__SOLVER_PACKAGE_H_


#include <package/PackageInfo.h>


namespace BPackageKit {


class BSolverRepository;


class BSolverPackage {
public:
								BSolverPackage(BSolverRepository* repository,
									const BPackageInfo& packageInfo);
								BSolverPackage(const BSolverPackage& other);
								~BSolverPackage();

			BSolverRepository*	Repository() const;
			const BPackageInfo&	Info() const;

			BString				Name() const;
			BString				VersionedName() const;
			const BPackageVersion& Version() const;

			BSolverPackage&		operator=(const BSolverPackage& other);

private:
			BSolverRepository*	fRepository;
			BPackageInfo		fInfo;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PACKAGE_H_
