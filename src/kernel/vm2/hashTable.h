#ifndef _HASH_H
#define _HASH_H
#include "list.h"
#include "vm.h"
#include "page.h"
#include "pageManager.h"
#include "vmHeaderBlock.h"
#include <new.h>

extern vmHeaderBlock *vmBlock;

class hashIterate;

class hashTable : public list
{
	friend hashIterate;
	public:
		// Constructors and Destructors and related
	hashTable(int size) {
		nodeCount=0;
		numRocks=size;
		
		//error ("Starting to initalize hash table\n");
		if (size*sizeof (list *)>PAGE_SIZE)
			throw ("Hash table too big!"); 
		//error ("Getting Page\n");
		
		// Get the block for the page of pointers
		page *newPage=vmBlock->pageMan->getPage();
		//error ("hashTable::hashTable - Got Page %x\n",newPage);
		pageList.add(newPage);

		if (!newPage) {
			error ("Out of pages to allocate a pool! newPage = %x\n",newPage);
			throw ("Out of pages to allocate a pool!");
			}
		rocks=(list **)(newPage->getAddress());
		//error ("Got rocks\n");

		int listsPerPage=PAGE_SIZE/sizeof(list);
		int pages=(size+(listsPerPage-1))/listsPerPage;
		for (int pageCount=0;pageCount<pages;pageCount++) {
			// Allocate a page of lists
			page *newPage=vmBlock->pageMan->getPage();
			//error ("hashTable::hashTable - Got Page %x\n",newPage);
			if (!newPage)
				throw ("Out of pages to allocate a pool!");
			for (int i=0;i<listsPerPage;i++)
				rocks[i]=new ((list *)(newPage->getAddress()+(i*sizeof(list)))) list;	
			pageList.add(newPage);
			}
		} 
	~hashTable() {
		while (struct page *cur=reinterpret_cast<page *>(pageList.next())) {
			//error ("hashTable::~hashTable; freeing page %x\n",cur);
			vmBlock->pageMan->freePage(cur);
		}
	}
		// Mutators
	void setHash (ulong (*hash_in)(node &)) { hash=hash_in; }
	void setIsEqual (bool (*isEqual_in)(node &,node &)) { isEqual=isEqual_in; }
	void add (node *newNode) { 
		if (!hash)
			throw ("Attempting to use a hash table without setting up a 'hash' function");
		unsigned long hashValue=hash(*newNode)%numRocks;
		// Note - no looking for duplicates; no ordering.
		rocks[hashValue]->add(newNode);	
		}

		// Accessors
	int count(void) {return nodeCount;}
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
	node *next(void) {throw ("Next is invalid in a hash table!");} // This operation doesn't make sense for this class
	void remove(node *toNuke) { 
		if (!hash)
			throw ("Attempting to use a hash table without setting up a 'hash' function");
		unsigned long hashValue=hash(*toNuke)%numRocks;
		rocks[hashValue]->remove(toNuke);	
		}

		// Debugging
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

	private:
	ulong (*hash)(node &a);
	bool (*isEqual)(node &a,node &b);
	list **rocks;
	list pageList;
	int numRocks;
};

class hashIterate {
	private:
	int bucket;
	node *current;
	hashTable &table;

	public:
	hashIterate(hashTable *in) : bucket(0),current(NULL),table(*in) {}
	hashIterate(hashTable &in) : bucket(0),current(NULL),table(in) {}

	node *get(void)
	{
		while (!current && bucket<table.numRocks) 
			current=table.rocks[bucket++]->top();
		if (!current)
			return NULL; // No more in the hash table
		node *retVal=current;	 // Store the current and get then next in preperation
		current=current->next;
		return retVal;
	}
};

#endif
