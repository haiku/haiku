// NodeTable.cpp

#include "Debug.h"
#include "NodeTable.h"

// constructor
NodeTable::NodeTable()
	: fElementArray(1000),
	  fNodes(1000, &fElementArray)
{
}

// destructor
NodeTable::~NodeTable()
{
}

// InitCheck
status_t
NodeTable::InitCheck() const
{
	RETURN_ERROR(fNodes.InitCheck() && fElementArray.InitCheck()
				 ? B_OK : B_NO_MEMORY);
}

// AddNode
status_t
NodeTable::AddNode(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		NodeHashElement *element
			= fNodes.Add(NodeHashElement::HashForID(node));
		if (element)
			element->fNode = node;
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// RemoveNode
status_t
NodeTable::RemoveNode(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveNode(node->GetID());
	return error;
}

// RemoveNode
status_t
NodeTable::RemoveNode(vnode_id id)
{
	status_t error = B_OK;
	if (NodeHashElement *element = _FindElement(id))
		fNodes.Remove(element);
	else
		error = B_ERROR;
	return error;
}

// GetNode
Node *
NodeTable::GetNode(vnode_id id)
{
	Node *node = NULL;
	if (NodeHashElement *element = _FindElement(id))
		node = element->fNode;
	return node;
}

// GetAllocationInfo
void
NodeTable::GetAllocationInfo(AllocationInfo &info)
{
	info.AddNodeTableAllocation(fNodes.ArraySize(), fNodes.VectorSize(),
								sizeof(NodeHashElement),
								fNodes.CountElements());
}

// _FindElement
NodeHashElement *
NodeTable::_FindElement(vnode_id id) const
{
	NodeHashElement *element
		= fNodes.FindFirst(NodeHashElement::HashForID(id));
	while (element && element->fNode->GetID() != id) {
		if (element->fNext >= 0)
			element = fNodes.ElementAt(element->fNext);
		else
			element = NULL;
	}
	return element;
}

