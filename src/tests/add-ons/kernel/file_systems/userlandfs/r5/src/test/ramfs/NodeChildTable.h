// NodeChildTable.h

#ifndef NODE_CHILD_TABLE_H
#define NODE_CHILD_TABLE_H

#include "AllocationInfo.h"
#include "Debug.h"
#include "Misc.h"
#include "Node.h"
#include "OpenHashTable.h"

// NodeChildHashElement
template<typename ParentNode, typename NodeChild>
class NodeChildHashElement : public OpenHashElement {
private:
	typedef NodeChildHashElement<ParentNode, NodeChild> Element;
public:

	NodeChildHashElement() : OpenHashElement(), fID(-1), fChild(NULL)
	{
		fNext = -1;
	}

	static inline uint32 HashFor(vnode_id id, const char *name)
	{
		return node_child_hash(id, name);
	}

	static inline uint32 HashFor(ParentNode *parent, NodeChild *child)
	{
		return node_child_hash(parent->GetID(), child->GetName());
	}

	inline uint32 Hash() const
	{
		return HashFor(fID, fChild->GetName());
	}

	inline bool Equals(vnode_id id, const char *name)
	{
		return (fID == id && !strcmp(fChild->GetName(), name));
	}

	inline bool operator==(const OpenHashElement &_element) const
	{
		const Element &element = static_cast<const Element&>(_element);
		return Equals(element.fID, element.fChild->GetName());
	}

	inline void Adopt(Element &element)
	{
		fID = element.fID;
		fChild = element.fChild;
	}

	vnode_id	fID;
	NodeChild	*fChild;
};

// NodeChildTable
template<typename ParentNode, typename NodeChild>
class NodeChildTable {
public:
	NodeChildTable();
	~NodeChildTable();

	status_t InitCheck() const;

	status_t AddNodeChild(ParentNode *node, NodeChild *child);
	status_t AddNodeChild(vnode_id, NodeChild *child);
	status_t RemoveNodeChild(ParentNode *node, NodeChild *child);
	status_t RemoveNodeChild(vnode_id id, NodeChild *child);
	status_t RemoveNodeChild(vnode_id id, const char *name);
	NodeChild *GetNodeChild(vnode_id id, const char *name);

protected:
	typedef NodeChildHashElement<ParentNode, NodeChild>	Element;

private:
	Element *_FindElement(vnode_id id, const char *name) const;

protected:
	OpenHashElementArray<Element>							fElementArray;
	OpenHashTable<Element, OpenHashElementArray<Element> >	fTable;
};

// define convenient instantiation types

// DirectoryEntryTable
class DirectoryEntryTable : public NodeChildTable<Directory, Entry> {
public:
	DirectoryEntryTable()	{}
	~DirectoryEntryTable()	{}

	void GetAllocationInfo(AllocationInfo &info)
	{
		info.AddDirectoryEntryTableAllocation(fTable.ArraySize(),
											  fTable.VectorSize(),
											  sizeof(Element),
											  fTable.CountElements());
	}
};

// NodeAttributeTable
class NodeAttributeTable : public NodeChildTable<Node, Attribute> {
public:
	NodeAttributeTable()	{}
	~NodeAttributeTable()	{}

	void GetAllocationInfo(AllocationInfo &info)
	{
		info.AddNodeAttributeTableAllocation(fTable.ArraySize(),
											 fTable.VectorSize(),
											 sizeof(Element),
											 fTable.CountElements());
	}
};


// NodeChildTable implementation

// constructor
template<typename ParentNode, typename NodeChild>
NodeChildTable<ParentNode, NodeChild>::NodeChildTable()
	: fElementArray(1000),
	  fTable(1000, &fElementArray)
{
}

// destructor
template<typename ParentNode, typename NodeChild>
NodeChildTable<ParentNode, NodeChild>::~NodeChildTable()
{
}

// InitCheck
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::InitCheck() const
{
	RETURN_ERROR(fTable.InitCheck() && fElementArray.InitCheck()
				 ? B_OK : B_NO_MEMORY);
}

// AddNodeChild
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::AddNodeChild(ParentNode *node,
													NodeChild *child)
{
	status_t error = (node && child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = AddNodeChild(node->GetID(), child);
	return error;
}

// AddNodeChild
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::AddNodeChild(vnode_id id,
													NodeChild *child)
{
	status_t error = (child ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		Element *element = fTable.Add(Element::HashFor(id, child->GetName()));
		if (element) {
			element->fID = id;
			element->fChild = child;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// RemoveNodeChild
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::RemoveNodeChild(ParentNode *node,
													   NodeChild *child)
{
	status_t error = (node && child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveNodeChild(node->GetID(), child->GetName());
	return error;
}

// RemoveNodeChild
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::RemoveNodeChild(vnode_id id,
													   NodeChild *child)
{
	status_t error = (child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveNodeChild(id, child->GetName());
	return error;
}

// RemoveNodeChild
template<typename ParentNode, typename NodeChild>
status_t
NodeChildTable<ParentNode, NodeChild>::RemoveNodeChild(vnode_id id,
													   const char *name)
{
	status_t error = B_OK;
	if (Element *element = _FindElement(id, name))
		fTable.Remove(element);
	else
		error = B_ERROR;
	return error;
}

// GetNodeChild
template<typename ParentNode, typename NodeChild>
NodeChild *
NodeChildTable<ParentNode, NodeChild>::GetNodeChild(vnode_id id,
													const char *name)
{
	NodeChild *child = NULL;
	if (Element *element = _FindElement(id, name))
		child = element->fChild;
	return child;
}

// _FindElement
template<typename ParentNode, typename NodeChild>
typename NodeChildTable<ParentNode, NodeChild>::Element *
NodeChildTable<ParentNode, NodeChild>::_FindElement(vnode_id id,
													const char *name) const
{
	Element *element = fTable.FindFirst(Element::HashFor(id, name));
	while (element && !element->Equals(id, name)) {
		if (element->fNext >= 0)
			element = fTable.ElementAt(element->fNext);
		else
			element = NULL;
	}
	return element;
}


// undefine the PRINT from <Debug.h>
//#undef PRINT

#endif	// NODE_CHILD_TABLE_H
