#ifndef _AREA_H
#define _AREA_H
#include "OS.h"
#include "vm.h"
#include "list.h"

class areaManager;
class vpage;

class area : public node
{
	protected:
		list vpages;
		char name[B_OS_NAME_LENGTH];
		pageState state;
		protectType protection;
		bool finalWrite;
		int areaID;
		int in_count;
		int out_count;
		int copy_count;
		areaManager *manager;
		unsigned long start_address;
		unsigned long end_address;
		vpage *findVPage(unsigned long);
	public:
		area(areaManager *myManager);
		bool nameMatch(char *matchName) {return (strcmp(matchName,name)==0);}
		unsigned long mapAddressSpecToAddress(addressSpec type,unsigned long requested,int pageCount);
		status_t createAreaMappingFile(char *name, int pageCount,void **address, addressSpec type,pageState state,protectType protect,int fd,size_t offset);
		status_t createArea (char *name, int pageCount,void **address, addressSpec type,pageState state,protectType protect);
		status_t cloneArea(area *area, char *inName, void **address, addressSpec type,pageState inState,protectType protect);
		int getAreaID(void) {return areaID;}
		void setAreaID(int id) {areaID=id;}
		void freeArea(void);
		status_t getInfo(area_info *dest);
		bool contains(void *address);
		status_t resize(size_t newSize);
		status_t setProtection(protectType prot);
		bool couldAdd(unsigned long start,unsigned long end) { return ((end<start_address) || (start>end_address));}
		unsigned long getEndAddress(void) {return end_address;}
		unsigned long getStartAddress(void) {return start_address;}
		void pager(int desperation);
		void saver(void);
		unsigned long getSize(void) {return getEndAddress()-getStartAddress();}
		unsigned long getPageCount(void) {return (getEndAddress()-getStartAddress())/PAGE_SIZE;}
		areaManager *getAreaManager(void) {return manager;}

		void dump(void);
		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.

		char getByte(unsigned long ); // This is for testing only
		void setByte(unsigned long ,char value); // This is for testing only
		int getInt(unsigned long ); // This is for testing only
		void setInt(unsigned long ,int value); // This is for testing only
};
#endif
