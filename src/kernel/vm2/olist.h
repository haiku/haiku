#ifndef _OLIST_H
#define _OLIST_H
#include "list.h"

static bool throwException (void *foo, void *bar)
{
	throw ("Attempting to use an ordered list without setting up a 'toLessThan' function");
}

class orderedList : public list
{
	public:
		// Constructors and Destructors and related
	orderedList(void) {nodeCount=0;rock=NULL; isLessThan=throwException; }
	
		// Mutators
	void setIsLessThan (bool (*iLT)(void *,void *)) { isLessThan=iLT; }
	void add(node *in) {
		nodeCount++;
		//error ("orderedList::add starting\n");
		if (!rock || isLessThan(in,rock)) { // special case - this will be the first one
			//error ("orderedList::specialCase starting\n");
			in->next=rock;
			rock=in;
			}
		else {
			//error ("orderedList::Normal Case starting\n");
			bool done=false;
			for (struct node *cur=rock;cur && !done;cur=cur->next)
				if (!(cur->next) || isLessThan(in,cur->next)) { // If we have found our niche, *OR* this is the last element, insert here. 
					//error ("orderedList::Normal Case Adding Start\n");
					in->next=cur->next;
					cur->next=in;
					done=true;
					//error ("orderedList::Normal Case Adding END\n");
					}
			//error ("orderedList::Normal Case ending\n");
			}
		}
	void remove(node *toNuke) {
		if (rock==toNuke) {
			rock=rock->next;
			nodeCount--;
			}
		else {
			bool done=false;
			for (struct node *cur=rock;!done && (cur->next);cur=cur->next)
				if (cur->next==toNuke) {
					cur->next=toNuke->next;
					nodeCount--;
					done=true;
					}
				else if (isLessThan(cur->next,toNuke)) // this is backwards intentionally
					done=true;
			}
		}

	private:
	bool (*isLessThan)(void *a,void *b);
};
#endif
