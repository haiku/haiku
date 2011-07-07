/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LAST_MODIFIED_INDEX_H
#define LAST_MODIFIED_INDEX_H


#include "Index.h"
#include "NodeListener.h"


class LastModifiedIndex : public Index, private NodeListener {
public:
								LastModifiedIndex();
	virtual						~LastModifiedIndex();

	status_t					Init(Volume* volume);

	virtual	int32				CountEntries() const;

private:
	virtual	void				NodeAdded(Node* node);
	virtual	void				NodeRemoved(Node* node);
	virtual	void				NodeChanged(Node* node, uint32 statFields,
									const OldNodeAttributes& oldAttributes);

protected:
	virtual	AbstractIndexIterator* InternalGetIterator();
	virtual	AbstractIndexIterator* InternalFind(const void* key,
									size_t length);

private:
			struct IteratorPolicy;
			class Iterator;
			class IteratorList;
			class NodeTree;
			friend class Iterator;
			friend struct IteratorPolicy;

private:
			void				_AddIteratorToUpdate(Iterator* iterator);

private:
			NodeTree*			fNodes;
			IteratorList*		fIteratorsToUpdate;
};


#endif	// LAST_MODIFIED_INDEX_H
