/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_SYSCALL_H
#define STRACE_SYSCALL_H

#include <string>
#include <vector>

#include <SupportDefs.h>

#include "TypeHandler.h"

// Type
class Type {
public:
	Type(TypeHandler *handler) : fHandler(handler) {}

	void SetHandler(TypeHandler *handler)
	{
		delete fHandler;
		fHandler = handler;
	}

	TypeHandler	*Handler() const	{ return fHandler; }

private:
	TypeHandler	*fHandler;
};

// Parameter
class Parameter : public Type {
public:
	Parameter(string name, int32 offset, TypeHandler *handler)
		: Type(handler),
		  fName(name),
		  fOffset(offset)
	{
	}

	const string &Name() const		{ return fName; }
	int32 Offset() const			{ return fOffset; }

private:
	string	fName;
	int32	fOffset;
};

// Syscall
class Syscall {
public:
	Syscall(string name, TypeHandler *returnTypeHandler)
		: fName(name), fReturnType(new Type(returnTypeHandler)) {}

	const string &Name() const
	{
		return fName;
	}

	Type *ReturnType() const
	{
		return fReturnType;
	}

	void AddParameter(Parameter *parameter)
	{
		fParameters.push_back(parameter);
	}

	void AddParameter(string name, int32 offset, TypeHandler *handler)
	{
		AddParameter(new Parameter(name, offset, handler));
	}

	int32 CountParameters() const
	{
		return fParameters.size();
	}

	Parameter *ParameterAt(int32 index) const
	{
		return fParameters[index];
	}

	Parameter *GetParameter(string name) const
	{
		int32 count = CountParameters();
		for (int32 i = 0; i < count; i++) {
			Parameter *parameter = ParameterAt(i);
			if (parameter->Name() == name)
				return parameter;
		}

		return NULL;
	}

private:
	string				fName;
	Type				*fReturnType;
	vector<Parameter*>	fParameters;
};

#endif	// STRACE_SYSCALL_H
