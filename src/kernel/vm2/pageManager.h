#ifndef _PAGE_MANAGER_H
#define _PAGE_MANAGER_H
#include "/boot/develop/headers/be/kernel/OS.h"
#include "list.h"
#include "page.h"

class pageManager {
	public:
		pageManager(void);
		void setup(void *memory,int pages);
		page *getPage(void);
		bool getContiguousPages(int pages,page **location);
		void freePage(page *);
		void cleaner(void);
		int desperation(void);
		void dump(void);
	private:
		list clean,unused,inUse;
		sem_id cleanLock,unusedLock,inUseLock;
		int totalPages;
};
#endif
