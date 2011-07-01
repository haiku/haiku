/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_TYPE_INFORMATION_H
#define TEAM_TYPE_INFORMATION_H


#include <Referenceable.h>
#include <SupportDefs.h>


class BString;
class Type;
class TypeLookupConstraints;


class TeamTypeInformation {
public:
	virtual						~TeamTypeInformation();


	virtual	status_t			LookupTypeByName(const BString& name,
									const TypeLookupConstraints& constraints,
									Type*& _type) = 0;
									// returns reference
};


#endif	// TEAM_TYPE_INFORMATION_H
