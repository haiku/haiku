/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEPENDENCY_H
#define DEPENDENCY_H


#include <package/PackageResolvableOperator.h>

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "String.h"


class DependencyFamily;
class Package;
class Resolvable;
class Version;


using BPackageKit::BPackageResolvableOperator;


class Dependency : public BReferenceable,
	public DoublyLinkedListLinkImpl<Dependency> {
public:
								Dependency(::Package* package);
	virtual						~Dependency();

			status_t			Init(const char* name);
			void				SetVersionRequirement(
									BPackageResolvableOperator op,
									Version* version);
									// version is optional; object takes over
									// ownership

			::Package*			Package() const
									{ return fPackage; }

			void				SetFamily(DependencyFamily* family)
									{ fFamily = family; }
			DependencyFamily*	Family() const
									{ return fFamily; }

			void				SetResolvable(::Resolvable* resolvable)
									{ fResolvable = resolvable; }
			::Resolvable*		Resolvable() const
									{ return fResolvable; }
			bool				ResolvableVersionMatches(
									Version* resolvableVersion) const;
			bool				ResolvableCompatibleVersionMatches(
									Version* resolvableVersion) const;

			const String&		Name() const		{ return fName; }
			const String&		FileName() const	{ return fFileName; }

private:
			::Package*			fPackage;
			DependencyFamily*	fFamily;
			::Resolvable*		fResolvable;
			String				fName;
			String				fFileName;
									// fName with ':' replaced by '~'
			Version*			fVersion;
			BPackageResolvableOperator fVersionOperator;

public:	// conceptually package private
			DoublyLinkedListLink<Dependency> fFamilyListLink;
			DoublyLinkedListLink<Dependency> fResolvableListLink;
};


typedef DoublyLinkedList<Dependency> DependencyList;

typedef DoublyLinkedList<Dependency,
	DoublyLinkedListMemberGetLink<Dependency,
		&Dependency::fFamilyListLink> > FamilyDependencyList;

typedef DoublyLinkedList<Dependency,
	DoublyLinkedListMemberGetLink<Dependency,
		&Dependency::fResolvableListLink> > ResolvableDependencyList;


#endif	// DEPENDENCY_H
