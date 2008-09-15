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

using std::string;
using std::vector;

// Type
class Type {
public:
	Type(string typeName, TypeHandler *handler)
		: fTypeName(typeName), fHandler(handler) {}

	const string &TypeName() const	{ return fTypeName; }

	void SetHandler(TypeHandler *handler)
	{
		delete fHandler;
		fHandler = handler;
	}

	TypeHandler	*Handler() const	{ return fHandler; }

private:
	string		fTypeName;
	TypeHandler	*fHandler;
};

// Parameter
class Parameter : public Type {
public:
	Parameter(string name, int32 offset, string typeName, TypeHandler *handler)
		: Type(typeName, handler),
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
	Syscall(string name, string returnTypeName, TypeHandler *returnTypeHandler)
		: fName(name),
		  fReturnType(new Type(returnTypeName, returnTypeHandler))
	{
	}

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

	void AddParameter(string name, int32 offset, string typeName,
		TypeHandler *handler)
	{
		AddParameter(new Parameter(name, offset, typeName, handler));
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
