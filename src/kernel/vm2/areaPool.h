#include <OS.h>
#include "lockedList.h"
class area;
class poolarea
{
	private: 
		lockedList unused;
		sem_id inUse;
	public:
		// Constructors and Destructors and related
		poolarea(void) { }

		// Mutators
		area *get(void);
		void put(area *in) {
			unused.add((node *)in);
			}

};
