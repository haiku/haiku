/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PACKAGE_SPECIFIER_LIST_H_
#define _PACKAGE__SOLVER_PACKAGE_SPECIFIER_LIST_H_


#include <SupportDefs.h>


namespace BPackageKit {


class BSolverPackageSpecifier;


class BSolverPackageSpecifierList {
public:
								BSolverPackageSpecifierList();
								BSolverPackageSpecifierList(
									const BSolverPackageSpecifierList& other);
								~BSolverPackageSpecifierList();

			bool				IsEmpty() const;
			int32				CountSpecifiers() const;
			const BSolverPackageSpecifier* SpecifierAt(int32 index) const;

			bool				AppendSpecifier(
									const BSolverPackageSpecifier& specifier);
			void				MakeEmpty();

			BSolverPackageSpecifierList& operator=(
									const BSolverPackageSpecifierList& other);

private:
			class Vector;

private:
			Vector*				fSpecifiers;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PACKAGE_SPECIFIER_LIST_H_
