/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARRAY_VALUE_NODE_H
#define ARRAY_VALUE_NODE_H


#include <ObjectList.h>

#include "ValueNode.h"


class AbstractArrayValueNodeChild;
class ArrayType;


class AbstractArrayValueNode : public ValueNode {
public:
								AbstractArrayValueNode(
									ValueNodeChild* nodeChild, ArrayType* type,
									int32 dimension);
	virtual						~AbstractArrayValueNode();

			ArrayType*			GetArrayType() const
									{ return fType; }
			int32				Dimension() const
									{ return fDimension; }

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

			// locking required

	virtual	status_t			CreateChildren();
	virtual	int32				CountChildren() const;
	virtual	ValueNodeChild*		ChildAt(int32 index) const;

protected:
			typedef BObjectList<AbstractArrayValueNodeChild> ChildList;

protected:
			ArrayType*			fType;
			ChildList			fChildren;
			int32				fDimension;
};


// TODO: Are ArrayValueNode and InternalArrayValueNode still needed?

class ArrayValueNode : public AbstractArrayValueNode {
public:
								ArrayValueNode(ValueNodeChild* nodeChild,
									ArrayType* type);
	virtual						~ArrayValueNode();
};


class InternalArrayValueNode : public AbstractArrayValueNode {
public:
								InternalArrayValueNode(
									ValueNodeChild* nodeChild,
									ArrayType* type, int32 dimension);
	virtual						~InternalArrayValueNode();
};


class AbstractArrayValueNodeChild : public ValueNodeChild {
public:
								AbstractArrayValueNodeChild(
									AbstractArrayValueNode* parent,
									const BString& name, int64 elementIndex);
	virtual						~AbstractArrayValueNodeChild();

			AbstractArrayValueNode* ArrayParent() const	{ return fParent; }
			int32				ElementIndex() const { return fElementIndex; }

	virtual	const BString&		Name() const;
	virtual	ValueNode*			Parent() const;

protected:
			AbstractArrayValueNode* fParent;
			BString				fName;
			int64				fElementIndex;
};


class ArrayValueNodeChild : public AbstractArrayValueNodeChild {
public:
								ArrayValueNodeChild(
									AbstractArrayValueNode* parent,
									const BString& name, int64 elementIndex,
									Type* type);
	virtual						~ArrayValueNodeChild();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
			Type*				fType;
};


class InternalArrayValueNodeChild : public AbstractArrayValueNodeChild {
public:
								InternalArrayValueNodeChild(
									AbstractArrayValueNode* parent,
									const BString& name, int64 elementIndex,
									ArrayType* type);
	virtual						~InternalArrayValueNodeChild();

	virtual	Type*				GetType() const;

	virtual	bool				IsInternal() const;
	virtual	status_t			CreateInternalNode(ValueNode*& _node);

	virtual	status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
			ArrayType*			fType;
};


#endif	// ARRAY_VALUE_NODE_H
