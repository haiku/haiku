#ifndef _AREA_H
#define _AREA_H
#include "OS.h"
#include "vm.h"
#include "page.h"
#include "lockedList.h"
//#include "hashTable.h"

class areaManager;
class vpage;

class area : public node
{
	protected:
		int fullPages; // full pages of vpage pointers
		int vpagesOnIndexPage; // number of vpages stored on the index page
		int vpagesOnNextPage; // number of vpages on each "fullPage"
		page *indexPage; // physical page of the index page

		int pageCount; // count of the number of pages in this area

		char name[B_OS_NAME_LENGTH]; // Our name
		pageState state; // Allocation policy.
		protectType protection; // read, r/w, copy on write
		bool finalWrite; // Write blocks in this area out on freeing area?
		area_id areaID; // Our numeric ID.
		int in_count; // Number of pages read in
		int out_count; // Number of pages written out
		int copy_count; // Number of block copies that have been made 
		areaManager *manager; // Our manager/process
		unsigned long start_address; // Where we start

		vpage *findVPage(unsigned long address)  { // Returns the page for this address.
			return getNthVpage((address-start_address)/PAGE_SIZE);
			//	error ("area::findVPage: finding %ld\n",address);
			}
		void allocateVPages(int pageCount); // Allocates blocks for the vpages
		page *getNthPage(int pageNum) { return 	&(((page *)(indexPage->getAddress()))[pageNum]); }
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
		unsigned long getPageCount(void) {return (pageCount);}
		areaManager *getAreaManager(void) {return manager;}
		unsigned long getEndAddress(void) {return getStartAddress()+(PAGE_SIZE*pageCount)-1;}
		unsigned long getStartAddress(void) {return start_address;}
		const char *getName(void) {return name;}
		vpage *getNthVpage(int pageNum);

		// Debugging
		void dump(void);
		char getByte(unsigned long ); // This is for testing only
		void setByte(unsigned long ,char value); // This is for testing only
		int getInt(unsigned long ); // This is for testing only
		void setInt(unsigned long ,int value); // This is for testing only

		// Comparisson with others
		bool nameMatch(char *matchName) {return (strcmp(matchName,name)==0);}
		bool couldAdd(unsigned long start,unsigned long end) { return ((end<start_address) || (start>getEndAddress()));}
		bool contains(const void *address);
		
		// External methods for "server" type calls
		void pager(int desperation);
		void saver(void);
		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.


};
#endif
