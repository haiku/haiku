#include <OS.h>
#include "lockedList.h"
class area;
class vnode;
class poolvnode
{
	private: 
		lockedList unused;
	public:
		poolvnode(void) {;}
		vnode *get(void);
		void put(vnode *in)
			{ unused.add((node *)in); }

};
