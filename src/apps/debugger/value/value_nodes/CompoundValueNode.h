/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMPOUND_VALUE_NODE_H
#define COMPOUND_VALUE_NODE_H


#include <ObjectList.h>

#include "ValueNode.h"


class CompoundType;


class CompoundValueNode : public ValueNode {
public:
								CompoundValueNode(ValueNodeChild* nodeChild,
									CompoundType* type);
	virtual						~CompoundValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

			// locking required

	virtual	status_t			CreateChildren();
	virtual	int32				CountChildren() const;
	virtual	ValueNodeChild*		ChildAt(int32 index) const;

private:
			class Child;
			class BaseTypeChild;
			class MemberChild;

			// for GCC2
			friend class BaseTypeChild;
			friend class MemberChild;

			typedef BObjectList<Child> ChildList;

private:
			CompoundType*		fType;
			ChildList			fChildren;
};


#endif	// ADDRESS_VALUE_NODE_H
