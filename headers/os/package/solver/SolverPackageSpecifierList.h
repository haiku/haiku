/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PACKAGE_SPECIFIER_LIST_H_
#define _PACKAGE__SOLVER_PACKAGE_SPECIFIER_LIST_H_


#include <String.h>


namespace BPackageKit {


class BSolverPackage;
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
			bool				AppendSpecifier(BSolverPackage* package);
			bool				AppendSpecifier(const BString& selectString);
			bool				AppendSpecifiers(
									const char* const* selectStrings,
									int32 count);
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
