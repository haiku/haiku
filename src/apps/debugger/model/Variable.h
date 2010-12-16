/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLE_H
#define VARIABLE_H


#include <String.h>

#include <Referenceable.h>


class ObjectID;
class Type;
class ValueLocation;


class Variable : public BReferenceable {
public:
								Variable(ObjectID* id, const BString& name,
									Type* type, ValueLocation* location);
								~Variable();

			ObjectID*			ID() const			{ return fID; }
			const BString&		Name() const		{ return fName; }
			Type*				GetType() const		{ return fType; }
			ValueLocation*		Location() const	{ return fLocation; }

private:
			ObjectID*			fID;
			BString				fName;
			Type*				fType;
			ValueLocation*		fLocation;
};


#endif	// VARIABLE_H
