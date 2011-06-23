/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESOLVABLE_H
#define RESOLVABLE_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>


class Package;
class Version;


class Resolvable : public BReferenceable,
	public DoublyLinkedListLinkImpl<Resolvable> {
public:
								Resolvable(Package* package);
	virtual						~Resolvable();

			status_t			Init(const char* name, Version* version);
									// version is optional; object takes over
									// ownership (even in case of error)

private:
			Package*			fPackage;
			char*				fName;
			Version*			fVersion;
};


typedef DoublyLinkedList<Resolvable> ResolvableList;


#endif	// RESOLVABLE_H
