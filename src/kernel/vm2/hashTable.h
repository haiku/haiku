#ifndef _HASH_H
#define _HASH_H
#include "list.h"

static bool throwException (node &foo)
{ throw ("Attempting to use an hash table without setting up a 'hash' function"); }

class hashTable : public list
{
	public:
	hashTable(int size)
		{
		nodeCount=0;
		numRocks=size;
		int pageCount=PAGE_SIZE*size/sizeof(list);	
		page *newPage=vmBlock->pageMan->getPage();
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		int newCount=PAGE_SIZE/sizeof(area);
		acquire_sem(inUse);
		//error ("poolarea::get: Adding %d new elements to the pool!\n",newCount);
		for (int i=0;i<newCount;i++)
			unused.add(((node *)(newPage->getAddress()+(i*sizeof(area)))));	
		release_sem(inUse);
		} 

	void setHash (ulong (*hash_in)(node &)) { hash=hash_in; }
	void setIsEqual (ulong (*isEqual_in)(node &,node &)) { isEqual=isEqual_in; }

	int count(void) {return nodeCount;}
	void add (node *newNode) { 
			unsigned long hashValue=hash(*newNode)%numRocks;
			// Note - no looking for duplicates; no ordering.
			rocks[hashValue].add(newNode);	
			}
	node *next(void) { return NULL; // This operation doesn't make sense for this class}
	void remove(node *toNuke) 
		{ 
		unsigned long hashValue=hash(*findNode)%numRocks;
		rocks[hashValue].remove(newNode);	
		}
	void dump(void) 
		{
		for (int i=0;i<numRocks;i++)
			for (struct node *cur=rocks[hashValue].rock;cur && !done;cur=cur->next)
				error ("hashTable::dump: At %p, next = %p\n",cur,cur->next);
		}
	node *find(node *findNode)
		{
		unsigned long hashValue=hash(*findNode)%numRocks;
		for (struct node *cur=rocks[hashValue].rock;cur && !done;cur=cur->next)
			if (isEqual(*findNode,*cur))
				return cur;
		return NULL;
		}
	private:
	ulong (*hash)(node &a);
	bool (*isEqual)(node &a,node &b);
	list *rocks;
	int numRocks;

	
};
#endif
