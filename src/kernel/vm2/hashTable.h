#ifndef _HASH_H
#define _HASH_H
#include "list.h"
#include "vm.h"
#include "page.h"
#include "pageManager.h"
#include "vmHeaderBlock.h"
#include <new.h>

extern vmHeaderBlock *vmBlock;

class hashTable : public list
{
	public:
	hashTable(int size) {
		nodeCount=0;
		numRocks=size;
		
		if (size*sizeof (list *)>PAGE_SIZE)
			throw ("Hash table too big!"); 
		page *newPage=vmBlock->pageMan->getPage();
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		rocks=(list **)(newPage->getAddress());

		int listsPerPage=PAGE_SIZE/sizeof(list);
		int pages=(size+(listsPerPage-1))/listsPerPage;
		for (int pageCount=0;pageCount<pages;pageCount++)
			{
			page *newPage=vmBlock->pageMan->getPage();
			if (!newPage)
				throw ("Out of pages to allocate a pool!");
			for (int i=0;i<listsPerPage;i++)
				rocks[i]=new ((list *)(newPage->getAddress()+(i*sizeof(list)))) list;	
			}
		} 

	void setHash (ulong (*hash_in)(node &)) { hash=hash_in; }
	void setIsEqual (bool (*isEqual_in)(node &,node &)) { isEqual=isEqual_in; }

	int count(void) {return nodeCount;}
	void add (node *newNode) { 
		if (!hash)
			throw ("Attempting to use a hash table without setting up a 'hash' function");
		unsigned long hashValue=hash(*newNode)%numRocks;
		// Note - no looking for duplicates; no ordering.
		rocks[hashValue]->add(newNode);	
		}

	node *next(void) {throw ("Next is invalid in a hash table!");} // This operation doesn't make sense for this class

	void remove(node *toNuke) { 
		if (!hash)
			throw ("Attempting to use a hash table without setting up a 'hash' function");
		unsigned long hashValue=hash(*toNuke)%numRocks;
		rocks[hashValue]->remove(toNuke);	
		}

	void dump(void) {
		for (int i=0;i<numRocks;i++)
			for (struct node *cur=rocks[i]->rock;cur;cur=cur->next)
				error ("hashTable::dump: On bucket %d of %d, At %p, next = %p\n",i,numRocks,cur,cur->next);
		}

	bool ensureSane (void) {
		bool ok=true;
		for (int i=0;i<numRocks;i++)
			ok|=rocks[i]->ensureSane();
		return ok;
		}
	node *find(node *findNode) {
		if (!hash)
			throw ("Attempting to use a hash table without setting up a 'hash' function");
		if (!isEqual)
			throw ("Attempting to use a hash table without setting up an 'isEqual' function");
		unsigned long hashValue=hash(*findNode)%numRocks;
		for (struct node *cur=rocks[hashValue]->rock;cur ;cur=cur->next)
			if (isEqual(*findNode,*cur))
				return cur;
		return NULL;
		}

	private:
	ulong (*hash)(node &a);
	bool (*isEqual)(node &a,node &b);
	list **rocks;
	int numRocks;
};
#endif
