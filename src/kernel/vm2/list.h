#ifndef _LIST_H
#define _LIST_H
// Simple linked list
#include <stdlib.h>
#include <stdio.h>

struct node 
{
	node *next;
};

class list {
	public:
		list(void) {nodeCount=0;rock=NULL;}
		void add (void *in)
			{
			struct node *newNode=(node *)in;
			newNode->next=rock;
			rock=newNode;
			nodeCount++;
			}
		//int count(void) {printf ("list::count: About to return %d\n",nodeCount);return nodeCount;}
		int count(void) {return nodeCount;}
		void *next(void) {nodeCount--;node *n=rock;if (rock) rock=rock->next;return n;} 
		void remove(void *in)
			{
			struct node *toNuke=(node *)in;
			for (struct node *cur=rock;cur;cur=cur->next)
				if (cur->next==toNuke)
					{
					cur->next=toNuke->next;
					cur=NULL; // To bust out of the loop...
					}
			}
		void dump(void)
			{
			for (struct node *cur=rock;cur;cur=cur->next)
				{
				printf ("list::dump: At %x, next = %x\n",cur,cur->next);
				}
			}
		struct node *rock;
		int nodeCount;
	private:
};
#endif
