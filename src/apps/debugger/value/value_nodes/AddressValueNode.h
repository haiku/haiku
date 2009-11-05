/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADDRESS_VALUE_NODE_H
#define ADDRESS_VALUE_NODE_H


#include "ValueNode.h"


class AddressValueNodeChild;
class AddressType;


class AddressValueNode : public ValueNode {
public:
								AddressValueNode(ValueNodeChild* nodeChild,
									AddressType* type);
	virtual						~AddressValueNode();

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
			AddressType*		fType;
			AddressValueNodeChild* fChild;
};


class AddressValueNodeChild : public ValueNodeChild {
public:
								AddressValueNodeChild(AddressValueNode* parent,
									const BString& name, Type* type);
	virtual						~AddressValueNodeChild();

	virtual	const BString&		Name() const;
	virtual	Type*				GetType() const;
	virtual	ValueNode*			Parent() const;

	virtual	status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
			AddressValueNode*	fParent;
			BString				fName;
			Type*				fType;
};


#endif	// ADDRESS_VALUE_NODE_H
