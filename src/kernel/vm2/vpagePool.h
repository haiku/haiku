#include <OS.h>
#include "lockedList.h"
class vpage;
class poolvpage
{
	private: 
		list unused;
		sem_id inUse;
	public:
		poolvpage(void) {
			inUse = create_sem(1,"vpagepool");
			}
		vpage *get(void);
		void put(vpage *in) {
			acquire_sem(inUse);
			unused.add((node *)in);
			release_sem(inUse);
			}

};
