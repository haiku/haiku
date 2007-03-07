// NodeTable.h

#ifndef NODE_TABLE_H
#define NODE_TABLE_H

#include "AllocationInfo.h"
#include "Node.h"
#include "OpenHashTable.h"

// NodeHashElement
class NodeHashElement : public OpenHashElement {
public:
	NodeHashElement() : OpenHashElement(), fNode(NULL)
	{
		fNext = -1;
	}

	static inline uint32 HashForID(vnode_id id)
	{
		return uint32(id & 0xffffffff);
	}

	static inline uint32 HashForID(Node *node)
	{
		return HashForID(node->GetID());
	}

	inline uint32 Hash() const
	{
		return HashForID(fNode);
	}

	inline bool operator==(const OpenHashElement &element) const
	{
		return (static_cast<const NodeHashElement&>(element).fNode == fNode);
	}

	inline void Adopt(NodeHashElement &element)
	{
		fNode = element.fNode;
	}

	Node	*fNode;
};

// NodeTable
class NodeTable {
public:
	NodeTable();
	~NodeTable();

	status_t InitCheck() const;

	status_t AddNode(Node *node);
	status_t RemoveNode(Node *node);
	status_t RemoveNode(vnode_id id);
	Node *GetNode(vnode_id id);

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	NodeHashElement *_FindElement(vnode_id id) const;

private:
	OpenHashElementArray<NodeHashElement>	fElementArray;
	OpenHashTable<NodeHashElement, OpenHashElementArray<NodeHashElement> >
		fNodes;
};

// undefine the PRINT from <Debug.h>
//#undef PRINT

#endif	// NODE_TABLE_H
