#include <new.h>
#include "area.h"
#include "areaManager.h"
#include "vpage.h"
#include "vnodePool.h"
#include "vpagePool.h"

#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

ulong vpageHash (node &vp) {return reinterpret_cast <vpage &>(vp).hash();}
bool vpageisEqual (node &vp,node &vp2) {return reinterpret_cast <vpage &>(vp)==reinterpret_cast <vpage &>(vp2);}

// Simple constructor; real work is later
area::area(void) : vpages(AREA_HASH_TABLE_SIZE) {
	vpages.setHash(vpageHash);
	vpages.setIsEqual(vpageisEqual);
	}

// Not much here, either
void area::setup (areaManager *myManager) {
	//error ("area::setup setting up new area\n");
	manager=myManager;
	//error ("area::setup done setting up new area\n");
	}

// Decide which algorithm to use for finding the next virtual address and try to find one.
unsigned long area::mapAddressSpecToAddress(addressSpec type,void * req,int pageCount) {
	// We will lock in the callers
	unsigned long base,requested=(unsigned long)req;
	switch (type) {
		case EXACT: 
			base=manager->getNextAddress(pageCount,requested); 
			if (base!=requested)
				return 0; 
			break;
		case BASE: 
			base=manager->getNextAddress(pageCount,requested); 
			break;
		case ANY: 
			base=manager->getNextAddress(pageCount,USER_BASE); 
			break;
		case ANY_KERNEL: 
			base=manager->getNextAddress(pageCount,KERNEL_BASE); 
			break;
		case CLONE: base=0;break; // Not sure what to do...
		default: // should never happen
			throw ("Unknown type passed to mapAddressSpecToAddress");
		}
//	error ("area::mapAddressSpecToAddress, in type: %s, address = %x, size = %d\n", ((type==EXACT)?"Exact":(type==BASE)?"BASE":(type==ANY)?"ANY":(type==CLONE)?"CLONE":"ANY_KERNEL"), requested,pageCount);
	return base;
}

// This is the really interesting part of creating an area
status_t area::createAreaGuts( char *inName, int pageCount, void **address, addressSpec type, pageState inState, protectType protect, bool inFinalWrite, int fd, size_t offset, area *originalArea=NULL /* For clone only*/) {
	error ("area::createAreaGuts : name = %s, pageCount = %d, address = %lx, addressSpec = %d, pageState = %d, protection = %d, inFinalWrite = %d, fd = %d, offset = %d,originalArea=%ld\n",
					inName,pageCount,address,type,inState,protect,inFinalWrite,fd,offset,originalArea);
	vpage *newPage;

	// Get an address to start this area at
	unsigned long base=mapAddressSpecToAddress(type,*address,pageCount);
	if (base==0)
		return B_ERROR;
	// Set up some basic info
	strcpy(name,inName);
	state=inState;
	start_address=base;
	end_address=base+(pageCount*PAGE_SIZE)-1;
	*address=(void *)base;
	finalWrite=inFinalWrite;
	// For non-cloned areas, make a new vpage for every page necesssary. 
	if (originalArea==NULL) // Not for cloning
		for (int i=0;i<pageCount;i++) {
			newPage=new (vmBlock->vpagePool->get()) vpage;
			if (fd) {
				vnode newVnode;
				newVnode.fd=fd;
				newVnode.offset=offset;
//				vmBlock->vnodeManager->addVNode(newVnode,newPage);
				newPage->setup(base+PAGE_SIZE*i,&newVnode,NULL,protect,inState);
				}
			else
				newPage->setup(base+PAGE_SIZE*i,NULL,NULL,protect,inState);
			vpages.add(newPage);
			}
	else // cloned
		// Need to lock other area, here, just in case...
		// Make a copy of each page in the other area...	
		for (hashIterate hi(vpages);node *cur=hi.get();) {
			vpage *page=(vpage *)cur;
			newPage=new (vmBlock->vpagePool->get()) vpage;	
			newPage->setup(base,page->getBacking(),page->getPhysPage(),protect,inState);// Cloned area has the same physical page and backing store...
			vpages.add(newPage);
			base+=PAGE_SIZE;
			}
	dump();
	vmBlock->areas.add(this);
	return B_OK;
}

status_t area::createAreaMappingFile(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect,int fd,size_t offset) {
	return createAreaGuts(inName,pageCount,address,type,inState,protect,true,fd,offset);
	}

status_t area::createArea(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect) {
	return createAreaGuts(inName,pageCount,address,type,inState,protect,false,0,0);
	}

// Clone another area.
status_t area::cloneArea(area *origArea, char *inName, void **address, addressSpec type,pageState inState,protectType protect) {
	if (type==CLONE) {
		*address=(void *)(origArea->getStartAddress());
		type=EXACT;
		}
	if (origArea->getAreaManager()!=manager) { // If they are in different areas...
		origArea->getAreaManager()->lock(); // This is just begging for a deadlock...
		status_t retVal = createAreaGuts(inName,origArea->getPageCount(),address,type,inState,protect,false,0,0,origArea);
		origArea->getAreaManager()->unlock(); 
		return retVal;
		}
	else
		return createAreaGuts(inName,origArea->getPageCount(),address,type,inState,protect,false,0,0,origArea);
	}

// To free an area, interate over its poges, final writing them if necessary, then call cleanup and put the vpage back in the pool
void area::freeArea(void) {
//error ("area::freeArea: starting \n");

//	vpages.dump();
	node *cur;
	for (hashIterate hi(vpages);node *cur=hi.get();) {
//error ("area::freeArea: wasting a page: %x\n",cur);
		vpage *page=reinterpret_cast<vpage *>(cur);
		if (finalWrite) 
			page->flush(); 
//error ("area::freeArea: flushed a page \n");
		page->cleanup();
		//page->next=NULL;
		vmBlock->vpagePool->put(page);
		}
	vpages.~hashTable();
//error ("area::freeArea: unlocking \n");
//error ("area::freeArea: ending \n");
	}

// Get area info
status_t area::getInfo(area_info *dest) {
	dest->area=areaID;
	strcpy(dest->name,name);
	dest->size=end_address-start_address;
	dest->lock=state;
	dest->protection=protection; 
	dest->team=manager->getTeam();
	dest->ram_size=0;
	dest->in_count=in_count;
	dest->out_count=out_count;
	dest->copy_count=0;
	for (hashIterate hi(vpages);node *cur=hi.get();) {
		vpage *page=(vpage *)cur;
		if (page->isMapped())
			dest->ram_size+=PAGE_SIZE;
		}
	dest->address=(void *)start_address;
	return B_OK;
	}

bool area::contains(void *address) {
	unsigned long base=(unsigned long)(address); 
//	error ("area::contains: looking for %d in %d -- %d, value = %d\n",base,start_address,end_address, ((start_address<=base) && (end_address>=base)));
					
	return ((start_address<=base) && (base<=end_address));
	}

// Resize an area. 
status_t area::resize(size_t newSize) {
	size_t oldSize =end_address-start_address;
	// Duh. Nothing to do.
	if (newSize==oldSize)
		return B_OK;
	// Grow the area. Figure out how many pages, allocate them and set them up
	if (newSize>oldSize) {
		int pageCount = (newSize - oldSize + PAGE_SIZE - 1) / PAGE_SIZE;
		vpage *newPage;
		for (int i=0;i<pageCount;i++) {
			newPage=new (vmBlock->vpagePool->get()) vpage;
			newPage->setup(end_address+PAGE_SIZE*i-1,NULL,NULL,protection,state);
			vpages.add(newPage);
			}
		end_address+=start_address+newSize;
		}
	else { // Ewww. Shrinking. This is ugly right now. 
		int pageCount = (oldSize - newSize + PAGE_SIZE - 1) / PAGE_SIZE;
		vpage *oldPage;
		struct node *cur;
		for (int i=0;i<pageCount;i++) { // This is probably really slow. OTOH, how often do people shrink their allocations?
			node *max;
			void *maxAddress=NULL,*curAddress;
			for (hashIterate hi(vpages);node *cur=hi.get();) 
				if ((curAddress=(reinterpret_cast<vpage *>(cur))->getStartAddress()) > maxAddress) {
					maxAddress=curAddress;
					max=cur;
					}
			// Found the right one to removei; waste it, pool it, and move on
			oldPage=reinterpret_cast<vpage *>(max);
			vpages.remove(cur);
			if (finalWrite) 
				oldPage->flush(); 
			oldPage->cleanup();
			vmBlock->vpagePool->put(oldPage);
			}
		}
	return B_OK;
	}

// When the protection for the area changes, the protection for every one of the pages must change
status_t area::setProtection(protectType prot) {
	for (hashIterate hi(vpages);node *cur=hi.get();) {
		vpage *page=(vpage *)cur;
		page->setProtection(prot);
		}
	protection=prot;
	return B_OK;
	}

vpage *area::findVPage(unsigned long address) {
	vpage findMe(address);
	return reinterpret_cast <vpage *>(vpages.find(&findMe));
	}

// To fault, find the vpage associated with the fault and call it's fault function
bool area::fault(void *fault_address, bool writeError) { // true = OK, false = panic.  
	vpage *page=findVPage((unsigned long)fault_address);
	if (page)
		return page->fault(fault_address,writeError,in_count);
	else
		return false;
	}

char area::getByte(unsigned long address) { // This is for testing only 
	vpage *page=findVPage(address);
	if (page)
		return page->getByte(address,manager);
	else
		return 0;
	}

void area::setByte(unsigned long address,char value) { // This is for testing only
	vpage *page=findVPage(address);
	if (page)
		page->setByte(address,value,manager);
	}

int area::getInt(unsigned long address) { // This is for testing only
	vpage *page=findVPage(address);
	if (page)
		return page->getInt(address,manager);
	else
		return 0;
	}

void area::setInt(unsigned long address,int value) { // This is for testing only
	vpage *page=findVPage(address);
	if (page)
		page->setInt(address,value,manager);
	}

// For every one of our vpages, call the vpage's pager
void area::pager(int desperation) {
	for (hashIterate hi(vpages);node *cur=hi.get();) {
		vpage *page=(vpage *)cur;
		if (page->pager(desperation))
			out_count++;
		}
	}

// For every one of our vpages, call the vpage's saver
void area::saver(void) {
	for (hashIterate hi(vpages);node *cur=hi.get();) {
		vpage *page=(vpage *)cur;
		page->saver();
		}
	}

void area::dump(void) { 
	error ("area::dump: size = %ld, lock = %d, address = %lx\n",end_address-start_address,state,start_address); 
	vpages.dump();
	for (hashIterate hi(vpages);node *cur=hi.get();) {
		vpage *page=(vpage *)cur;
		page->dump();
		cur=cur->next;
		}
	}

