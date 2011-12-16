/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESOLVABLE_H
#define RESOLVABLE_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "Dependency.h"


class Package;
class ResolvableFamily;
class Version;


class Resolvable : public BReferenceable,
	public DoublyLinkedListLinkImpl<Resolvable> {
public:
								Resolvable(::Package* package);
	virtual						~Resolvable();

			status_t			Init(const char* name, ::Version* version,
									::Version* compatibleVersion);
									// version and compatibleVersion are
									// optional; object takes over ownership
									// (even in case of error)

			::Package*			Package() const	{ return fPackage; }

			void				SetFamily(ResolvableFamily* family)
									{ fFamily = family; }
			ResolvableFamily*	Family() const
									{ return fFamily; }

			const char*			Name() const	{ return fName; }
			::Version*			Version() const	{ return fVersion; }
			::Version*			CompatibleVersion() const
									{ return fCompatibleVersion; }

			void				AddDependency(Dependency* dependency);
			void				RemoveDependency(Dependency* dependency);
			void				MoveDependencies(
									ResolvableDependencyList& dependencies);

private:
			::Package*			fPackage;
			ResolvableFamily*	fFamily;
			char*				fName;
			::Version*			fVersion;
			::Version*			fCompatibleVersion;
			ResolvableDependencyList fDependencies;

public:	// conceptually package private
			DoublyLinkedListLink<Resolvable> fFamilyListLink;
};


typedef DoublyLinkedList<Resolvable> ResolvableList;

typedef DoublyLinkedList<Resolvable,
	DoublyLinkedListMemberGetLink<Resolvable,
		&Resolvable::fFamilyListLink> > FamilyResolvableList;


#endif	// RESOLVABLE_H
