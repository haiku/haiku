#include <OS.h>
#include "list.h"
class area;
class poolarea
{
	private: 
		list unused;
		sem_id inUse;
	public:
		// Constructors and Destructors and related
		poolarea(void) {
			inUse = create_sem(1,"areapool");
			}

		// Mutators
		area *get(void);
		void put(area *in) {
			acquire_sem(inUse);
			unused.add((node *)in);
			release_sem(inUse);
			}

};
