#include <vm.h>
#include <pageManager.h>
#include <swapFileManager.h>

class vpage : public node
{
	private:
		page *physPage;
		vnode *backingNode;
		protectType protection;
		bool dirty;
		bool swappable;
		unsigned long start_address;
		unsigned long end_address;
	public:
		bool isMapped(void) {return (physPage);}
		bool contains(uint32 address) { return ((start_address<=address) && (end_address>=address)); }
		void flush(void); // write page to vnode, if necessary
		void refresh(void); // Read page back in from vnode
		vpage *clone(unsigned long); // Make a new vpage that is exactly the same as this one. 
									// If we are read only, it is read only. 
									// If we are read/write, both pages are copy on write
		vpage(unsigned long  start,vnode *backing, page *physMem,protectType prot,pageState state); // backing and/or physMem can be NULL/0.
		~vpage(void);
		void setProtection(protectType prot);
		protectType getProtection(void) {return protection;}
		void *getStartAddress(void) {return (void *)start_address;}

		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.

		void pager(int desperation);
		void saver(void);
		
		char getByte(unsigned long offset); // This is for testing only
		void setByte(unsigned long offset,char value); // This is for testing only
		int getInt(unsigned long offset); // This is for testing only
		void setInt(unsigned long offset,int value); // This is for testing only
};
