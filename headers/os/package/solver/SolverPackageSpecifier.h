/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_
#define _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_


#include <String.h>


namespace BPackageKit {


class BSolverPackage;


class BSolverPackageSpecifier {
public:
			enum BType {
				B_UNSPECIFIED,
				B_PACKAGE,
				B_SELECT_STRING
			};

public:
								BSolverPackageSpecifier();
	explicit					BSolverPackageSpecifier(
									BSolverPackage* package);
	explicit					BSolverPackageSpecifier(
									const BString& selectString);
								BSolverPackageSpecifier(
									const BSolverPackageSpecifier& other);
								~BSolverPackageSpecifier();

			BType				Type() const;
			BSolverPackage*		Package() const;
			const BString&		SelectString() const;

			BSolverPackageSpecifier& operator=(
									const BSolverPackageSpecifier& other);

private:
			BType				fType;
			BSolverPackage*		fPackage;
			BString				fSelectString;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PACKAGE_SPECIFIER_H_
