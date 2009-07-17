/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLE_H
#define VARIABLE_H


#include <String.h>

#include <Referenceable.h>


class Type;
class ValueLocation;


class Variable : public Referenceable {
public:
								Variable(const BString& name, Type* type,
									ValueLocation* location);
								~Variable();

			const BString&		Name() const		{ return fName; }
			Type*				GetType() const		{ return fType; }
			ValueLocation*		Location() const	{ return fLocation; }

private:
			BString				fName;
			Type*				fType;
			ValueLocation*		fLocation;
};


#endif	// VARIABLE_H
