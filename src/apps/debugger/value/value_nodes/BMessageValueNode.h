/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BMESSAGE_VALUE_NODE_H
#define BMESSAGE_VALUE_NODE_H

#include <Message.h>
#include <ObjectList.h>

#include "ValueNode.h"


class BMessageValueNode : public ValueNode {
public:
									BMessageValueNode(ValueNodeChild* nodeChild,
										Type* type);
	virtual							~BMessageValueNode();

	virtual	Type*					GetType() const;

	virtual	status_t				ResolvedLocationAndValue(
										ValueLoader* valueLoader,
										ValueLocation*& _location,
										Value*& _value);

	virtual status_t				CreateChildren();
	virtual int32					CountChildren() const;
	virtual ValueNodeChild*			ChildAt(int32 index) const;

private:
			class BMessageFieldNode;
			class BMessageFieldNodeChild;
			class BMessageFieldHeaderNode;
			class BMessageFieldHeaderNodeChild;

			// for GCC2
			friend class BMessageFieldNode;
			friend class BMessageFieldNodeChild;
			friend class BMessageFieldHeaderNode;
			friend class BMessageFieldHeaderNodeChild;
private:
			Type*					fType;
			BMessage				fMessage;
			BMessageFieldNodeChild*	fFields;
};


class BMessageValueNode::BMessageFieldHeaderNode : public ValueNode {
public:
									BMessageFieldHeaderNode(
										BMessageFieldHeaderNodeChild *child,
										BMessageValueNode* parent,
										const BString& name,
										type_code type, int32 count);

	virtual 						~BMessageFieldHeaderNode();

	virtual Type* 					GetType() const;

	virtual	status_t 				CreateChildren();
	virtual	int32 					CountChildren() const;
	virtual	ValueNodeChild* 		ChildAt(int32 index) const;

	virtual status_t 				ResolvedLocationAndValue(
										ValueLoader* loader,
										ValueLocation *& _location,
										Value*& _value);

private:
			BString 				fName;
			Type*					fType;
			BMessageValueNode* 		fParent;
			type_code				fFieldType;
			int32					fFieldCount;
};


class BMessageValueNode::BMessageFieldHeaderNodeChild : public ValueNodeChild {
public:
									BMessageFieldHeaderNodeChild(
										BMessageFieldNode* parent,
										BMessageValueNode* messageNode,
										const BString &name,
										type_code type, int32 count);

	virtual 						~BMessageFieldHeaderNodeChild();

	virtual const BString& 			Name() const;
	virtual Type* 					GetType() const;
	virtual ValueNode* 				Parent() const;

	virtual bool 					IsInternal() const;
	virtual status_t 				CreateInternalNode(
										ValueNode*& _node);

	virtual	status_t 				ResolveLocation(ValueLoader* valueLoader,
										ValueLocation*& _location);

	virtual status_t 				CreateChildren();
	virtual int32 					CountChildren() const;
	virtual ValueNodeChild* 		ChildAt(int32 index) const;

private:
			BString 				fName;
			Type*					fType;
			BMessageValueNode* 		fMessageNode;
			BMessageFieldNode*		fParent;
			type_code				fFieldType;
			int32					fFieldCount;
};


class BMessageValueNode::BMessageFieldNode : public ValueNode {
public:
									BMessageFieldNode(
										BMessageFieldNodeChild *child,
										BMessageValueNode* parent);

	virtual 						~BMessageFieldNode();

	virtual Type* 					GetType() const;

	virtual	status_t 				CreateChildren();
	virtual	int32 					CountChildren() const;
	virtual	ValueNodeChild* 		ChildAt(int32 index) const;

	virtual status_t 				ResolvedLocationAndValue(
										ValueLoader* loader,
										ValueLocation *& _location,
										Value*& _value);

private:
			typedef BObjectList<BMessageFieldHeaderNodeChild>
				FieldHeaderNodeList;

private:
	Type*							fType;
	BMessageValueNode*				fParent;
	FieldHeaderNodeList				fChildren;
};


class BMessageValueNode::BMessageFieldNodeChild : public ValueNodeChild {
public:
									BMessageFieldNodeChild(
										BMessageValueNode* parent);
	virtual							~BMessageFieldNodeChild();

	virtual const BString& 			Name() const;
	virtual Type* 					GetType() const;
	virtual ValueNode* 				Parent() const;

	virtual bool 					IsInternal() const;
	virtual status_t 				CreateInternalNode(ValueNode*& _node);

	virtual	status_t 				ResolveLocation(ValueLoader* valueLoader,
										ValueLocation*& _location);

private:
	Type*							fType;
	BMessageValueNode*				fParent;
	BString							fName;
};

#endif	// BMESSAGE_VALUE_NODE_H
