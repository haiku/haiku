#include "area.h"
#include "olist.h"

class areaManager // One of these per process
{
	private:
		orderedList areas; // A list, ordered by address, of our areas.
		team_id team; // The team that we belong to
		sem_id myLock; // a lock for adding/searching/removing teams
		static long nextAreaID; // The next area id to be used. Should be in vmInterface or vmBlock
	public:
		// Constructors and Destructors and related
		areaManager ();
		void addArea(area *newArea) {areas.add(newArea);}
		void removeArea(area *oldArea) {areas.remove(oldArea); }
		status_t freeArea(area_id area);
		int createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect) ;
		int cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType=ANY, pageState state=NO_LOCK, protectType prot=writable);

		// Accessors
		team_id getTeam(void) {return team;}
		unsigned long getNextAddress(int pages,unsigned long minimum=USER_BASE);
		status_t getAreaInfo(int areaID,area_info *dest); 
		int getAreaByName(char *name);
		void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
		status_t munmap(void *addr,size_t len);

		// Mutators
		area *findArea(const void *address);
		area *findAreaLock(void *address);
		area *findArea(char *address);
		area *findArea(area_id id);
		area *findAreaLock(area_id id);
		status_t setProtection(int areaID,protectType prot);
		status_t resizeArea(int Area,size_t size);
		status_t getInfoAfter(int32 & areaID,area_info *dest);
		void lock() { acquire_sem(myLock); }
		void unlock() {release_sem(myLock);}
		long get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries);
		long lock_memory(void *address, ulong numBytes, ulong flags);
		long unlock_memory(void *address, ulong numBytes, ulong flags);
		
		// External methods for "server" type calls
		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.
		void pager(int desperation);
		void saver(void);

		// Debugging
		char getByte(unsigned long offset); // This is for testing only
		void setByte(unsigned long offset,char value); // This is for testing only
		int getInt(unsigned long offset); // This is for testing only
		void setInt(unsigned long offset,int value); // This is for testing only
};
