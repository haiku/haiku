#include "vm.h"
#include "pageManager.h"
#include "areaManager.h"
#include "swapFileManager.h"
class vmInterface // This is the class that "owns" all of the managers.
{
	private:
		areaManager *getAM(void); // This is for testing only...
	public:
		vmInterface(int pages);
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
		status_t munmap(void *addr, size_t len);
		void pager(void);
		void saver(void);
		void cleaner(void);
		status_t writeCachedBlock(int fd, size_t offset, void *data);
		status_t readCachedBlock(int fd, size_t offset, void *data);

		// Driver Interface
		long get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries);
		long lock_memory(void *address, ulong numBytes, ulong flags);
		long unlock_memory(void *address, ulong numBytes, ulong flags);
		area_id map_physical_memory(const char *areaName, void *physAddress, size_t bytes, uint32 spec, uint32 protection, void **vaddress);


		char getByte(unsigned long offset) {return getAM()->getByte(offset);} // This is for testing only
		void setByte(unsigned long offset,char value) {getAM()->setByte(offset,value);} // This is for testing only
		int getInt(unsigned long offset) {return getAM()->getInt(offset);} // This is for testing only
		void setInt(unsigned long offset,int value) {getAM()->setByte(offset,value);} // This is for testing only
		thread_id tid_cleaner,tid_saver,tid_pager;
}; 
