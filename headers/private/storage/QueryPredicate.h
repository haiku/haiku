//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file QueryPredicate.h
	BQuery predicate helper classes interface declaration.
*/
#ifndef _QUERY_PREDICATE_H
#define _QUERY_PREDICATE_H

#include <stdio.h>

#include <List.h>
#include <Query.h>
#include <String.h>

namespace BPrivate {
namespace Storage {

// QueryNode
class QueryNode {
public:
	QueryNode();
	virtual ~QueryNode();

	virtual uint32 Arity() const = 0;
	virtual status_t SetChildAt(QueryNode *child, int32 index) = 0;
	virtual QueryNode *ChildAt(int32 index) = 0;

	virtual status_t GetString(BString &predicate) = 0;
};

// LeafNode
class LeafNode : public QueryNode {
public:
	LeafNode();
	virtual ~LeafNode();

	virtual uint32 Arity() const;
	virtual status_t SetChildAt(QueryNode *child, int32 index);
	virtual QueryNode *ChildAt(int32 index);
};

// UnaryNode
class UnaryNode : public QueryNode {
public:
	UnaryNode();
	virtual ~UnaryNode();

	virtual uint32 Arity() const;
	virtual status_t SetChildAt(QueryNode *child, int32 index);
	virtual QueryNode *ChildAt(int32 index);

protected:
	QueryNode	*fChild;
};

// BinaryNode
class BinaryNode : public QueryNode {
public:
	BinaryNode();
	virtual ~BinaryNode();

	virtual uint32 Arity() const;
	virtual status_t SetChildAt(QueryNode *child, int32 index);
	virtual QueryNode *ChildAt(int32 index);

protected:
	QueryNode	*fChild1;
	QueryNode	*fChild2;
};

// AttributeNode
class AttributeNode : public LeafNode {
public:
	AttributeNode(const char *attribute);

	virtual status_t GetString(BString &predicate);

private:
	BString	fAttribute;
};

// StringNode
class StringNode : public LeafNode {
public:
	StringNode(const char *value, bool caseInsensitive = false);

	virtual status_t GetString(BString &predicate);

	inline const char *Value() const { return fValue.String(); }

private:
	BString	fValue;
};

// DateNode
class DateNode : public LeafNode {
public:
	DateNode(const char *value);

	virtual status_t GetString(BString &predicate);

private:
	BString	fValue;
};

// ValueNode
template<typename ValueType>
class ValueNode : public LeafNode {
public:
	ValueNode(const ValueType &value);

	virtual status_t GetString(BString &predicate);

private:
	ValueType	fValue;
};

// constructor
template<typename ValueType>
ValueNode<ValueType>::ValueNode(const ValueType &value)
					: LeafNode(),
					  fValue(value)
{
}

// GetString
template<typename ValueType>
status_t
ValueNode<ValueType>::GetString(BString &predicate)
{
	predicate.SetTo("");
	predicate << fValue;
	return B_OK;
}

// specializations for float and double
template<> status_t ValueNode<float>::GetString(BString &predicate);
template<> status_t ValueNode<double>::GetString(BString &predicate);


// short hands
typedef ValueNode<int32>	Int32ValueNode;
typedef ValueNode<uint32>	UInt32ValueNode;
typedef ValueNode<int64>	Int64ValueNode;
typedef ValueNode<uint64>	UInt64ValueNode;
typedef ValueNode<float>	FloatValueNode;
typedef ValueNode<double>	DoubleValueNode;


// SpecialOpNode
class SpecialOpNode : public LeafNode {
public:
	SpecialOpNode(query_op op);

	virtual status_t GetString(BString &predicate);

private:
	query_op	fOp;
};

// UnaryOpNode
class UnaryOpNode : public UnaryNode {
public:
	UnaryOpNode(query_op op);

	virtual status_t GetString(BString &predicate);

private:
	query_op	fOp;
};

// BinaryOpNode
class BinaryOpNode : public BinaryNode {
public:
	BinaryOpNode(query_op op);

	virtual status_t GetString(BString &predicate);

private:
	query_op	fOp;
};


// QueryStack
class QueryStack {
public:
	QueryStack();
	virtual ~QueryStack();

	status_t PushNode(QueryNode *node);
	QueryNode *PopNode();

	status_t ConvertToTree(QueryNode *&rootNode);

private:
	status_t _GetSubTree(QueryNode *&rootNode);

private:
	BList	fNodes;
};

};	// namespace Storage
};	// namespace BPrivate

#endif	// _QUERY_PREDICATE_H


