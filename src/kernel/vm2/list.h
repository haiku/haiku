#ifndef _LIST_H
#define _LIST_H
// Simple linked list
#include <stdlib.h>
#include <stdio.h>
#include "error.h"

struct node 
{
	node *next;
};

class list {
	public:
		// Constructors and Destructors and related
		list(void){nodeCount=0;rock=NULL;} 

		// Mutators
		void add (node *newNode) {
			newNode->next=rock;
			rock=newNode;
			nodeCount++;
			}
		node *next(void) {
			//dump();
			node *n=rock;
			if (rock) {
				rock=rock->next;
				nodeCount--;
				}
			//dump();
			return n;
			} 
		void remove(node *toNuke) {
			//error ("list::remove starting: nuking %x \n",toNuke);
			//dump();	
			if (rock==toNuke) {
				rock=rock->next;
				nodeCount--;
				}
			else {
				bool done=false;
				for (struct node *cur=rock;!done && cur->next;cur=cur->next)
					if (cur->next==toNuke) {
						cur->next=toNuke->next;
						nodeCount--;
						done=true;
						}
				if (!done)
					throw ("list::remove failed to find node %x\n",toNuke);
				}
			//error ("list::remove ending: \n");
			//dump();	
			}

		// Accessors
		int count(void) {return nodeCount;}
		node *top(void) {return rock;} // Intentionally non-destructive ; works like peek() on a queue

		// Debugging
		void dump(void) {
			for (struct node *cur=rock;cur;cur=cur->next)
				{ error ("list::dump: At %p, next = %p\n",cur,cur->next); }
			}
		bool ensureSane (void) {
			int temp=nodeCount;
			for (struct node *cur=rock;cur && --temp;cur=cur->next) ; // Intentional to have no body
			if (temp<0) {
				error ("list::ensureSane: found too many records!\n");
				return false;
				}
			if (temp>0) {
				error ("list::ensureSane: found too few records!\n");
				return false;
				}
			return true;
			}

	protected:
		struct node *rock;
		int nodeCount;
};
#endif
