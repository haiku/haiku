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
		char bits; // 0/1 are protection, 2 == dirty, 3 == swappable, 4 == locked
		unsigned long start_address; //bye
	public:
		// Constructors and Destructors and related
		vpage(void);
		vpage(unsigned long address) {start_address=address-address%PAGE_SIZE;} // Only for lookups
		// Setup should now only be called by the vpage manager...
		void setup(unsigned long  start,vnode *backing, page *physMem,protectType prot,pageState state, mmapSharing share=CLONEAREA); // backing and/or physMem can be NULL/0.
		void cleanup(void);

		// Mutators
		void setProtection(protectType prot);
		void flush(void); // write page to vnode, if necessary
		void refresh(void); // Read page back in from vnode
		bool lock(long flags); // lock this page into memory
		void unlock(long flags); // unlock this page from memory
		void dirty(bool yesOrNo) {if (yesOrNo) bits|=4; else bits &= ~4;}
		void swappable(bool yesOrNo) {if (yesOrNo) bits|=8; else bits &= ~8;}
		void locked(bool yesOrNo) {if (yesOrNo) bits|=16; else bits &= ~16;}
		void protection(protectType prot) {bits|= (prot & 3);}

		// Accessors
		protectType getProtection(void) {return (protectType)(bits & 3);}
		bool isDirty(void) {return bits & 4;}
		bool isSwappable(void) {return bits & 8;}
		bool isLocked(void) {return bits & 16;}
		void *getStartAddress(void) {return (void *)start_address;}
		page *getPhysPage(void) {return physPage;}
		vnode *getBacking(void) {return backingNode;}
		bool isMapped(void) {return (physPage);}
		unsigned long end_address(void) {return start_address+PAGE_SIZE;}
		
		// Comparisson with others
		ulong hash(void) {return start_address >> BITS_IN_PAGE_SIZE;}
		bool operator==(vpage &rhs) {return rhs.start_address==start_address; }
		bool contains(uint32 address) { return ((start_address<=address) && (end_address()>=address)); }

		// External methods for "server" type calls
		bool fault(void *fault_address, bool writeError, int &in_count); // true = OK, false = panic.
		bool pager(int desperation);
		void saver(void);
		
		// Debugging
		void dump(void) {
			error ("Dumping vpage %p, address = %lx, vnode-fd=%d, vnode-offset = %d, dirty = %d, swappable = %d, locked = %d\n",
							this,start_address, ((backingNode)?(backingNode->fd):99999), ((backingNode)?(backingNode->offset):999999999),
							isDirty(),isSwappable(),isLocked());
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
