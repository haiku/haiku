#include "vm.h"
#include "pageManager.h"
#include "swapFileManager.h"
class vmInterface // This is the class that "owns" all of the managers.
{
	private:
		swapFileManager swapMan;
		pageManager pageMan;
		int nextAreaID;
	public:
		vmInterface(int pages) : pageMan(pages) {nextAreaID=0;};
		int createArea(char *AreaName,int pageCount,void **address,
							addressSpec addType=ANY,	
							pageState state=NO_LOCK,protectType protect=writable);
		void freeArea(int Area);
		status_t getAreaInfo(int Area,area_info *dest);
		status_t getNextAreaInfo(int  process,int32 *cookie,area_info *dest);
		int getAreaByName(char *name);
		int getAreaByAddress(void *address);
		int cloneArea(int area,char *AreaName,void **address,
						addressSpec addType=ANY,
						pageState state=NO_LOCK,
						protectType prot=writable);
		status_t resizeArea(int area,size_t size);
		status_t setAreaProtection(int area,protectType prot);
		void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
		void pager(void);
		void saver(void);
		void cleaner(void);
};
