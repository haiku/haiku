#ifndef VPAGE_H
#define VPAGE_H
#include <vm.h>
#include <pageManager.h>
#include <swapFileManager.h>

class areaManager;
class vpage : public node
{
	private:
		page *physPage;
		vnode *backingNode;
		protectType protection;
		bool dirty;
		bool swappable;
		bool locked;
		unsigned long start_address;
		unsigned long end_address;
	public:
		// Constructors and Destructors and related
		vpage(void);
		vpage(unsigned long address) {start_address=address-address%PAGE_SIZE;end_address=start_address+PAGE_SIZE-1;} // Only for lookups
		// Setup should now only be called by the vpage manager...
		void setup(unsigned long  start,vnode *backing, page *physMem,protectType prot,pageState state); // backing and/or physMem can be NULL/0.
		void cleanup(void);

		// Mutators
		void setProtection(protectType prot);
		void flush(void); // write page to vnode, if necessary
		void refresh(void); // Read page back in from vnode
		bool lock(long flags); // lock this page into memory
		void unlock(long flags); // unlock this page from memory


		// Accessors
		protectType getProtection(void) {return protection;}
		void *getStartAddress(void) {return (void *)start_address;}
		page *getPhysPage(void) {return physPage;}
		vnode *getBacking(void) {return backingNode;}
		bool isMapped(void) {return (physPage);}
		
		// Comparisson with others
		ulong hash(void) {return start_address >> BITS_IN_PAGE_SIZE;}
		bool operator==(vpage &rhs) {return rhs.start_address==start_address && rhs.end_address==end_address;}
		bool contains(uint32 address) { return ((start_address<=address) && (end_address>=address)); }

		// External methods for "server" type calls
		bool fault(void *fault_address, bool writeError, int &in_count); // true = OK, false = panic.
		bool pager(int desperation);
		void saver(void);
		
		// Debugging
		void dump(void) {
			error ("Dumping vpage %p, address = %lx, physPage: \n",this,start_address);
			if (physPage)
				physPage->dump();
			else
				error ("NULL\n");
		}
		char getByte(unsigned long offset,areaManager *manager); // This is for testing only
		void setByte(unsigned long offset,char value,areaManager *manager); // This is for testing only
		int getInt(unsigned long offset,areaManager *manager); // This is for testing only
		void setInt(unsigned long offset,int value,areaManager *manager); // This is for testing only
};
#endif
