/*
 * Copyright 2002-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file QueryPredicate.cpp
	BQuery predicate helper classes implementation.
*/

#include "QueryPredicate.h"

#include <ctype.h>

#include <UnicodeChar.h>


namespace BPrivate {
namespace Storage {

// #pragma mark - QueryNode


QueryNode::QueryNode()
{
}


QueryNode::~QueryNode()
{
}


// #pragma mark - LeafNode


LeafNode::LeafNode()
{
}


LeafNode::~LeafNode()
{
}


uint32
LeafNode::Arity() const
{
	return 0;
}


status_t
LeafNode::SetChildAt(QueryNode *child, int32 index)
{
	return B_BAD_VALUE;
}


QueryNode *
LeafNode::ChildAt(int32 index)
{
	return NULL;
}


// #pragma mark - UnaryNode


UnaryNode::UnaryNode()
	:
	fChild(NULL)
{
}


UnaryNode::~UnaryNode()
{
	delete fChild;
}


uint32
UnaryNode::Arity() const
{
	return 1;
}


status_t
UnaryNode::SetChildAt(QueryNode *child, int32 index)
{
	status_t error = B_OK;
	if (index == 0) {
		delete fChild;
		fChild = child;
	} else
		error = B_BAD_VALUE;
	return error;
}


QueryNode *
UnaryNode::ChildAt(int32 index)
{
	QueryNode *result = NULL;
	if (index == 0)
		result = fChild;
	return result;
}


// #pragma mark - BinaryNode


BinaryNode::BinaryNode()
	:
	fChild1(NULL),
	fChild2(NULL)
{
}


BinaryNode::~BinaryNode()
{
	delete fChild1;
	delete fChild2;
}


uint32
BinaryNode::Arity() const
{
	return 2;
}


status_t
BinaryNode::SetChildAt(QueryNode *child, int32 index)
{
	status_t error = B_OK;
	if (index == 0) {
		delete fChild1;
		fChild1 = child;
	} else if (index == 1) {
		delete fChild2;
		fChild2 = child;
	} else
		error = B_BAD_VALUE;
	return error;
}


QueryNode *
BinaryNode::ChildAt(int32 index)
{
	QueryNode *result = NULL;
	if (index == 0)
		result = fChild1;
	else if (index == 1)
		result = fChild2;
	return result;
}


// #pragma mark - AttributeNode


AttributeNode::AttributeNode(const char *attribute)
	:
	fAttribute(attribute)
{
}


status_t
AttributeNode::GetString(BString &predicate)
{
	predicate.SetTo(fAttribute);
	return B_OK;
}


// #pragma mark - StringNode


StringNode::StringNode(const char *value, bool caseInsensitive)
{
	if (value == NULL)
		return;

	if (caseInsensitive) {
		while (uint32 codePoint = BUnicodeChar::FromUTF8(&value)) {
			char utf8Buffer[4];
			char *utf8 = utf8Buffer;
			if (BUnicodeChar::IsAlpha(codePoint)) {
				uint32 lower = BUnicodeChar::ToLower(codePoint);
				uint32 upper = BUnicodeChar::ToUpper(codePoint);
				if (lower == upper) {
					BUnicodeChar::ToUTF8(codePoint, &utf8);
					fValue.Append(utf8Buffer, utf8 - utf8Buffer);
				} else {
					fValue << "[";
					BUnicodeChar::ToUTF8(lower, &utf8);
					fValue.Append(utf8Buffer, utf8 - utf8Buffer);
					utf8 = utf8Buffer;
					BUnicodeChar::ToUTF8(upper, &utf8);
					fValue.Append(utf8Buffer, utf8 - utf8Buffer);
					fValue << "]";
				}
			} else if (codePoint == L' ') {
				fValue << '*';
			} else {
				BUnicodeChar::ToUTF8(codePoint, &utf8);
				fValue.Append(utf8Buffer, utf8 - utf8Buffer);
			}
		}
	} else {
		fValue = value;
		fValue.ReplaceAll(' ', '*');
	}
}


status_t
StringNode::GetString(BString &predicate)
{
	BString escaped(fValue);
	escaped.CharacterEscape("\"\\'", '\\');
	predicate.SetTo("");
	predicate << "\"" << escaped << "\"";
	return B_OK;
}


// #pragma mark - DateNode


DateNode::DateNode(const char *value)
	:
	fValue(value)
{
}


status_t
DateNode::GetString(BString &predicate)
{
	BString escaped(fValue);
	escaped.CharacterEscape("%\"\\'", '\\');
	predicate.SetTo("");
	predicate << "%" << escaped << "%";
	return B_OK;
}


// #pragma mark - ValueNode


template<>
status_t
ValueNode<float>::GetString(BString &predicate)
{
	char buffer[32];
	union {
		int32 asInteger;
		float asFloat;
	} value;
	value.asFloat = fValue;
//	int32 value = *reinterpret_cast<int32*>(&fValue);
	sprintf(buffer, "0x%08lx", value.asInteger);
	predicate.SetTo(buffer);
	return B_OK;
}


template<>
status_t
ValueNode<double>::GetString(BString &predicate)
{
	char buffer[32];
	union {
		int64 asInteger;
		double asFloat;
	} value;
//	int64 value = *reinterpret_cast<int64*>(&fValue);
	value.asFloat = fValue;
	sprintf(buffer, "0x%016Lx", value.asInteger);
	predicate.SetTo(buffer);
	return B_OK;
}


// #pragma mark - SpecialOpNode


SpecialOpNode::SpecialOpNode(query_op op)
	:
	fOp(op)
{
}


status_t
SpecialOpNode::GetString(BString &predicate)
{
	return B_BAD_VALUE;
}


// #pragma mark - UnaryOpNode


UnaryOpNode::UnaryOpNode(query_op op)
	:
	fOp(op)
{
}


status_t
UnaryOpNode::GetString(BString &predicate)
{
	status_t error = (fChild ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (fOp == B_NOT) {
			BString childString;
			error = fChild->GetString(childString);
			predicate.SetTo("(!");
			predicate << childString << ")";
		} else
			error = B_BAD_VALUE;
	}
	return error;
}


// #pragma mark - BinaryOpNode


BinaryOpNode::BinaryOpNode(query_op op)
	:
	fOp(op)
{
}


status_t
BinaryOpNode::GetString(BString &predicate)
{
	status_t error = (fChild1 && fChild2 ? B_OK : B_BAD_VALUE);
	BString childString1;
	BString childString2;
	if (error == B_OK)
		error = fChild1->GetString(childString1);
	if (error == B_OK)
		error = fChild2->GetString(childString2);
	predicate.SetTo("");
	if (error == B_OK) {
		switch (fOp) {
			case B_EQ:
				predicate << "(" << childString1 << "=="
					<< childString2 << ")";
				break;
			case B_GT:
				predicate << "(" << childString1 << ">"
					<< childString2 << ")";
				break;
			case B_GE:
				predicate << "(" << childString1 << ">="
					<< childString2 << ")";
				break;
			case B_LT:
				predicate << "(" << childString1 << "<"
					<< childString2 << ")";
				break;
			case B_LE:
				predicate << "(" << childString1 << "<="
					<< childString2 << ")";
				break;
			case B_NE:
				predicate << "(" << childString1 << "!="
					<< childString2 << ")";
				break;
			case B_CONTAINS:
				if (StringNode *strNode = dynamic_cast<StringNode*>(fChild2)) {
					BString value;
					value << "*" << strNode->Value() << "*";
					error = StringNode(value.String()).GetString(childString2);
				}
				if (error == B_OK) {
					predicate << "(" << childString1 << "=="
						<< childString2 << ")";
				}
				break;
			case B_BEGINS_WITH:
				if (StringNode *strNode = dynamic_cast<StringNode*>(fChild2)) {
					BString value;
					value << strNode->Value() << "*";
					error = StringNode(value.String()).GetString(childString2);
				}
				if (error == B_OK) {
					predicate << "(" << childString1 << "=="
						<< childString2 << ")";
				}
				break;
			case B_ENDS_WITH:
				if (StringNode *strNode = dynamic_cast<StringNode*>(fChild2)) {
					BString value;
					value << "*" << strNode->Value();
					error = StringNode(value.String()).GetString(childString2);
				}
				if (error == B_OK) {
					predicate << "(" << childString1 << "=="
						<< childString2 << ")";
				}
				break;
			case B_AND:
				predicate << "(" << childString1 << "&&"
					<< childString2 << ")";
				break;
			case B_OR:
				predicate << "(" << childString1 << "||"
					<< childString2 << ")";
				break;
			default:
				error = B_BAD_VALUE;
				break;
		}
	}
	return error;
}


// #pragma mark - QueryStack


QueryStack::QueryStack()
{
}


QueryStack::~QueryStack()
{
	for (int32 i = 0; QueryNode *node = (QueryNode*)fNodes.ItemAt(i); i++)
		delete node;
}


status_t
QueryStack::PushNode(QueryNode *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!fNodes.AddItem(node))
			error = B_NO_MEMORY;
	}
	return error;
}


QueryNode *
QueryStack::PopNode()
{
	return (QueryNode*)fNodes.RemoveItem(fNodes.CountItems() - 1);
}


status_t
QueryStack::ConvertToTree(QueryNode *&rootNode)
{
	status_t error = _GetSubTree(rootNode);
	if (error == B_OK && !fNodes.IsEmpty()) {
		error = B_BAD_VALUE;
		delete rootNode;
		rootNode = NULL;
	}
	return error;
}


status_t
QueryStack::_GetSubTree(QueryNode *&rootNode)
{
	QueryNode *node = PopNode();
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		uint32 arity = node->Arity();
		for (int32 i = (int32)arity - 1; error == B_OK && i >= 0; i--) {
			QueryNode *child = NULL;
			error = _GetSubTree(child);
			if (error == B_OK) {
				error = node->SetChildAt(child, i);
				if (error != B_OK)
					delete child;
			}
		}
	}
	// clean up, if something went wrong
	if (error != B_OK && node) {
		delete node;
		node = NULL;
	}
	rootNode = node;
	return error;
}


}	// namespace Storage
}	// namespace BPrivate
