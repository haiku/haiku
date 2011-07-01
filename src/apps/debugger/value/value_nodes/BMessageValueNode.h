/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BMESSAGE_VALUE_NODE_H
#define BMESSAGE_VALUE_NODE_H

#include <Message.h>
#include <ObjectList.h>

#include "ValueNode.h"


class CompoundType;


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

	virtual bool					ChildCreationNeedsValue() const
										{ return true; }
	virtual status_t				CreateChildren();
	virtual int32					CountChildren() const;
	virtual ValueNodeChild*			ChildAt(int32 index) const;

			CompoundType*			GetMessageType() const
										{ return fMessageType; }

private:
			class BMessageFieldNode;
			class BMessageFieldNodeChild;

			// for GCC2
			friend class BMessageFieldNode;
			friend class BMessageFieldNodeChild;

			typedef BObjectList<ValueNodeChild> ChildNodeList;

private:
			Type*					fType;
			CompoundType*			fMessageType;
			BMessage				fMessage;
			ChildNodeList			fChildren;
};


class BMessageValueNode::BMessageFieldNode : public ValueNode {
public:
									BMessageFieldNode(
										BMessageFieldNodeChild *child,
										BMessageValueNode* parent,
										const BString& name,
										type_code type, int32 count);

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
			BString 				fName;
			Type*					fType;
			BMessageValueNode* 		fParent;
			type_code				fFieldType;
			int32					fFieldCount;
};


class BMessageValueNode::BMessageFieldNodeChild : public ValueNodeChild {
public:
									BMessageFieldNodeChild(
										BMessageValueNode* parent,
										const BString &name,
										type_code type, int32 count);

	virtual 						~BMessageFieldNodeChild();

	virtual const BString& 			Name() const;
	virtual Type* 					GetType() const;
	virtual ValueNode* 				Parent() const;

	virtual bool 					IsInternal() const;
	virtual status_t 				CreateInternalNode(
										ValueNode*& _node);

	virtual	status_t 				ResolveLocation(ValueLoader* valueLoader,
										ValueLocation*& _location);

private:
			BString 				fName;
			Type*					fType;
			BMessageValueNode* 		fParent;
			type_code				fFieldType;
			int32					fFieldCount;
};

#endif	// BMESSAGE_VALUE_NODE_H
