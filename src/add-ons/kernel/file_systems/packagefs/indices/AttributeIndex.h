/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_INDEX_H
#define ATTRIBUTE_INDEX_H


#include "Index.h"
#include "NodeListener.h"


class AttributeIndexer;
struct AttributeIndexTreeKey;
struct AttributeIndexTreeValue;


class AttributeIndex : public Index, private NodeListener {
public:
								AttributeIndex();
	virtual						~AttributeIndex();

			status_t			Init(Volume* volume, const char* name,
									uint32 type, size_t keyLength);

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
			typedef AttributeIndexTreeKey TreeKey;
			typedef AttributeIndexTreeValue TreeValue;
			struct TreeDefinition;
			struct NodeTree;

			struct IteratorPolicy;
			class Iterator;
			class IteratorList;
			friend class Iterator;
			friend struct IteratorPolicy;

			friend class AttributeIndexer;

private:
			void				_AddIteratorToUpdate(Iterator* iterator);

private:
			NodeTree*			fNodes;
			IteratorList*		fIteratorsToUpdate;
			AttributeIndexer*	fIndexer;
};


#endif	// ATTRIBUTE_INDEX_H
