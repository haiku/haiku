#ifndef _AREA_H
#define _AREA_H
#include "OS.h"
#include "vm.h"
#include "list.h"
#include "hashTable.h"

class areaManager;
class vpage;

class area : public node
{
	protected:
		hashTable vpages;
		char name[B_OS_NAME_LENGTH];
		pageState state;
		protectType protection;
		bool finalWrite;
		area_id areaID;
		int in_count;
		int out_count;
		int copy_count;
		areaManager *manager;
		unsigned long start_address;
		unsigned long end_address;
		vpage *findVPage(unsigned long);
	public:
		// Constructors and Destructors and related
		area(void);
		void setup(areaManager *myManager);
		void freeArea(void);
		status_t createAreaGuts( char *inName, int pageCount, void **address, addressSpec type, pageState inState, protectType protect, bool inFinalWrite, int fd, size_t offset, area *originalArea=NULL,mmapSharing share=CLONEAREA /* For clone only*/);
		status_t createAreaMappingFile(char *name, int pageCount,void **address, addressSpec type,pageState state,protectType protect,int fd,size_t offset, mmapSharing share=CLONEAREA);
		status_t createArea (char *name, int pageCount,void **address, addressSpec type,pageState state,protectType protect);
		status_t cloneArea(area *area, char *inName, void **address, addressSpec type,pageState inState,protectType protect);
		unsigned long mapAddressSpecToAddress(addressSpec type,void *requested,int pageCount);

		// Mutators
		void setAreaID(int id) {areaID=id;}
		status_t setProtection(protectType prot);
		status_t resize(size_t newSize);
		long get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries);
		long lock_memory(void *address, ulong numBytes, ulong flags);
		long unlock_memory(void *address, ulong numBytes, ulong flags);
		
		// Accessors
		status_t getInfo(area_info *dest);
		int getAreaID(void) {return areaID;}
		unsigned long getSize(void) {return getEndAddress()-getStartAddress();}
		unsigned long getPageCount(void) {return (getEndAddress()-getStartAddress())/PAGE_SIZE;}
		areaManager *getAreaManager(void) {return manager;}
		unsigned long getEndAddress(void) {return end_address;}
		unsigned long getStartAddress(void) {return start_address;}

		// Debugging
		void dump(void);
		char getByte(unsigned long ); // This is for testing only
		void setByte(unsigned long ,char value); // This is for testing only
		int getInt(unsigned long ); // This is for testing only
		void setInt(unsigned long ,int value); // This is for testing only

		// Comparisson with others
		bool nameMatch(char *matchName) {return (strcmp(matchName,name)==0);}
		bool couldAdd(unsigned long start,unsigned long end) { return ((end<start_address) || (start>end_address));}
		bool contains(const void *address);
		
		// External methods for "server" type calls
		void pager(int desperation);
		void saver(void);
		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.


};
#endif
