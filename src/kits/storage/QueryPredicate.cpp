//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file QueryPredicate.cpp
	BQuery predicate helper classes implementation.
*/
#include "QueryPredicate.h"

#include <ctype.h>

namespace StorageKit {

// QueryNode

// constructor
QueryNode::QueryNode()
{
}

// destructor
QueryNode::~QueryNode()
{
}


// LeafNode

// constructor
LeafNode::LeafNode()
		: QueryNode()
{
}

// destructor
LeafNode::~LeafNode()
{
}

// Arity
uint32
LeafNode::Arity() const
{
	return 0;
}

// SetChildAt
status_t
LeafNode::SetChildAt(QueryNode *child, int32 index)
{
	return B_BAD_VALUE;
}

// ChildAt
QueryNode *
LeafNode::ChildAt(int32 index)
{
	return NULL;
}


// UnaryNode

// constructor
UnaryNode::UnaryNode()
		 : QueryNode(),
		   fChild(NULL)
{
}

// destructor
UnaryNode::~UnaryNode()
{
	delete fChild;
}

// Arity
uint32
UnaryNode::Arity() const
{
	return 1;
}

// SetChildAt
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

// ChildAt
QueryNode *
UnaryNode::ChildAt(int32 index)
{
	QueryNode *result = NULL;
	if (index == 0)
		result = fChild;
	return result;
}


// BinaryNode

// constructor
BinaryNode::BinaryNode()
		  : QueryNode(),
			fChild1(NULL),
			fChild2(NULL)
{
}

// destructor
BinaryNode::~BinaryNode()
{
	delete fChild1;
	delete fChild2;
}

// Arity
uint32
BinaryNode::Arity() const
{
	return 2;
}

// SetChildAt
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

// ChildAt
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


// AttributeNode

// constructor
AttributeNode::AttributeNode(const char *attribute)
			 : LeafNode(),
			   fAttribute(attribute)
{
}

// GetString
status_t
AttributeNode::GetString(BString &predicate)
{
	predicate.SetTo(fAttribute);
	return B_OK;
}


// StringNode

// constructor
StringNode::StringNode(const char *value, bool caseInsensitive)
		  : LeafNode(),
			fValue()
{
	if (value) {
		if (caseInsensitive) {
			int32 len = strlen(value);
			for (int32 i = 0; i < len; i++) {
				char c = value[i];
				if (isalpha(c)) {
					int lower = tolower(c);
					int upper = toupper(c);
					if (lower < 0 || upper < 0)
						fValue << c;
					else
						fValue << "[" << (char)lower << (char)upper << "]";
				} else
					fValue << c;
			}
		} else
			fValue = value;
	}
}

// GetString
status_t
StringNode::GetString(BString &predicate)
{
	BString escaped(fValue);
	escaped.CharacterEscape("\"\\'", '\\');
	predicate.SetTo("");
	predicate << "\"" << escaped << "\"";
	return B_OK;
}


// DateNode

// constructor
DateNode::DateNode(const char *value)
		: LeafNode(),
		  fValue(value)
{
}

// GetString
status_t
DateNode::GetString(BString &predicate)
{
	BString escaped(fValue);
	escaped.CharacterEscape("%\"\\'", '\\');
	predicate.SetTo("");
	predicate << "%" << escaped << "%";
	return B_OK;
}


// ValueNode

// GetString
//
// float specialization
template<>
status_t
ValueNode<float>::GetString(BString &predicate)
{
	char buffer[32];
	sprintf(buffer, "0x%08lx", *(int32*)&fValue);
	predicate.SetTo(buffer);
	return B_OK;
}

// GetString
//
// double specialization
template<>
status_t
ValueNode<double>::GetString(BString &predicate)
{
	char buffer[32];
	sprintf(buffer, "0x%016Lx", *(int64*)&fValue);
	predicate.SetTo(buffer);
	return B_OK;
}


// SpecialOpNode

// constructor
SpecialOpNode::SpecialOpNode(query_op op)
			 : LeafNode(),
			   fOp(op)
{
}

// GetString
status_t
SpecialOpNode::GetString(BString &predicate)
{
	return B_BAD_VALUE;
}


// UnaryOpNode

// constructor
UnaryOpNode::UnaryOpNode(query_op op)
		   : UnaryNode(),
			 fOp(op)
{
}

// GetString
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


// BinaryOpNode

// constructor
BinaryOpNode::BinaryOpNode(query_op op)
			: BinaryNode(),
			  fOp(op)
{
}

// GetString
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


// QueryStack

// constructor
QueryStack::QueryStack()
		  : fNodes()
{
}

// destructor
QueryStack::~QueryStack()
{
	for (int32 i = 0; QueryNode *node = (QueryNode*)fNodes.ItemAt(i); i++)
		delete node;
}

// PushNode
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

// PopNode
QueryNode *
QueryStack::PopNode()
{
	return (QueryNode*)fNodes.RemoveItem(fNodes.CountItems() - 1);
}

// ConvertToTree
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

// _GetSubTree
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


};	// namespace StorageKit
