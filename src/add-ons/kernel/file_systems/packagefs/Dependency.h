/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEPENDENCY_H
#define DEPENDENCY_H


#include <package/PackageResolvableOperator.h>

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>


class Package;
class Resolvable;
class Version;


using BPackageKit::BPackageResolvableOperator;


class Dependency : public BReferenceable,
	public DoublyLinkedListLinkImpl<Dependency> {
public:
								Dependency(Package* package);
	virtual						~Dependency();

			status_t			Init(const char* name);
			void				SetVersionRequirement(
									BPackageResolvableOperator op,
									Version* version);
									// version is optional; object takes over
									// ownership

			void				SetResolvable(::Resolvable* resolvable)
									{ fResolvable = resolvable; }
			::Resolvable*		Resolvable() const
									{ return fResolvable; }

private:
			Package*			fPackage;
			::Resolvable*		fResolvable;
			char*				fName;
			Version*			fVersion;
			BPackageResolvableOperator fVersionOperator;
};


typedef DoublyLinkedList<Dependency> DependencyList;


#endif	// DEPENDENCY_H
