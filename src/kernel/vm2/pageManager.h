#ifndef _PAGE_MANAGER_H
#define _PAGE_MANAGER_H
#include "/boot/develop/headers/be/kernel/OS.h"
#include "list.h"
#include "page.h"

class pageManager {
	public:
		// Constructors and Destructors and related
		pageManager(void);
		void setup(void *memory,int pages);
		void freePage(page *);

		// Mutators
		page *getPage(void);

		// Accessors
		int desperation(void);
		int freePageCount(void) {return clean.count()+unused.count();}

		// External methods for "server" type calls
		void cleaner(void);
		void cleanOnePage(void);

		// Debugging
		void dump(void);
		int getCleanCount(void) {return clean.count();}
		int getUnusedCount(void) {return unused.count();}
		int getInUseCount(void) {return inUse.count();}
	private:
		list clean,unused,inUse;
		sem_id cleanLock,unusedLock,inUseLock;
		int totalPages;
};
#endif
