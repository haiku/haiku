#include <new.h>
#include "area.h"
#include "areaManager.h"
#include "vpage.h"
#include "vnodePool.h"
#include "vnodeManager.h"

#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

// Simple constructor; real work is later
area::area(void) {
	}

// Not much here, either
void area::setup (areaManager *myManager) {
	//error ("area::setup setting up new area\n");
	manager=myManager;
	//error ("area::setup done setting up new area\n");
	}

// Decide which algorithm to use for finding the next virtual address and try to find one.
unsigned long area::mapAddressSpecToAddress(addressSpec type,void * req,int inPageCount) {
	// We will lock in the callers
	unsigned long base,requested=(unsigned long)req;
	switch (type) {
		case EXACT: 
			base=manager->getNextAddress(inPageCount,requested); 
			if (base!=requested)
				return 0; 
			break;
		case BASE: 
			base=manager->getNextAddress(inPageCount,requested); 
			break;
		case ANY: 
			base=manager->getNextAddress(inPageCount,USER_BASE); 
			break;
		case ANY_KERNEL: 
			base=manager->getNextAddress(inPageCount,KERNEL_BASE); 
			break;
		case CLONE: base=0;break; // Not sure what to do...
		default: // should never happen
			throw ("Unknown type passed to mapAddressSpecToAddress");
		}
	error ("area::mapAddressSpecToAddress, in type: %s, address = %x, size = %d\n", ((type==EXACT)?"Exact":(type==BASE)?"BASE":(type==ANY)?"ANY":(type==CLONE)?"CLONE":"ANY_KERNEL"), requested,inPageCount);
	return base;
}

vpage *area::getNthVpage(int pageNum) {
		/*
	error ("Inside getNthVPage; pageNum=%d, fullPages = %d, vpagesOnIndexPage=%d, vpagesOnNextPage=%d\n",pageNum,fullPages,vpagesOnIndexPage,vpagesOnNextPage);
	error ("Inside getNthVPage; indexPage = %x, address = %x\n",indexPage,indexPage->getAddress());
	for (int i=0;i<PAGE_SIZE/64;fprintf (stderr,"\n"),i++)
		for (int j=0;j<32;j++)
			fprintf (stderr,"%04.4x ",indexPage->getAddress()+i*64+j);
			*/
				
	if (pageNum<=vpagesOnIndexPage) { // Skip the page pointers, then skip to the right vpage number;
		return &(((vpage *)(getNthPage(fullPages+1)))[pageNum]);
	}
	else {
		page *myPage=getNthPage(((pageNum-vpagesOnIndexPage-1)/vpagesOnNextPage));
		return (vpage *)(myPage->getAddress()+(((pageNum-vpagesOnIndexPage-1)%vpagesOnNextPage)*sizeof(vpage)));
	}
}

void area::allocateVPages(int pageCountIn) {

	// Allocate all of the physical page space that we will need here and now for vpages
	// Allocate number of pages necessary to hold the vpages, plus the index page...
	pageCount=pageCountIn;	
	indexPage=vmBlock->pageMan->getPage();
	error ("area::allocateVPages : index page = %x, (physical address = %x\n",indexPage,indexPage->getAddress());
	vpagesOnNextPage=PAGE_SIZE/sizeof(vpage); // Number of vpages per full physical page.
	fullPages = pageCount / vpagesOnNextPage; // Number of full pages that we need.
	int bytesLeftOnIndexPage = PAGE_SIZE-(fullPages*sizeof(vpage *)); // Room left on index page
	vpagesOnIndexPage=bytesLeftOnIndexPage/sizeof(vpage); 
	if ((fullPages*vpagesOnNextPage + vpagesOnIndexPage)< pageCount)  { // not enough room...
		fullPages++;
		bytesLeftOnIndexPage = PAGE_SIZE-(fullPages*sizeof(vpage *)); // Recalculate these, since they have changed.
		vpagesOnIndexPage=bytesLeftOnIndexPage/sizeof(vpage); 
		}

	// Allocate the physical page space. 
	for (int count=0;count<fullPages;count++) {
		page *curPage=vmBlock->pageMan->getPage();
		((page **)(indexPage->getAddress()))[count]=curPage;
		}
	error ("area::allocateVPages : index page = %x, (physical address = %x\n",indexPage,indexPage->getAddress());
	}

// This is the really interesting part of creating an area
status_t area::createAreaGuts( char *inName, int inPageCount, void **address, addressSpec type, pageState inState, protectType protect, bool inFinalWrite, int fd, size_t offset, area *originalArea /* For clone only*/, mmapSharing share) {
	error ("area::createAreaGuts : name = %s, pageCount = %d, address = %lx, addressSpec = %d, pageState = %d, protection = %d, inFinalWrite = %d, fd = %d, offset = %d,originalArea=%ld\n",
					inName,inPageCount,address,type,inState,protect,inFinalWrite,fd,offset,originalArea);
	vpage *newPage;

	// We need RAM - let's fail if we don't have enough... This is Be's way. I probably would do this differently...
	if (!originalArea && (inState!=LAZY) && (inState!=NO_LOCK) && (inPageCount>(vmBlock->pageMan->freePageCount())))
			return B_NO_MEMORY;
	else
		error ("origArea = %d, instate = %d, LAZY = %d, NO_LOCK = %d, pageCountIn = %d, free pages = %d\n",
			originalArea, inState,LAZY ,NO_LOCK,inPageCount,(vmBlock->pageMan->freePageCount()));

	// Get an address to start this area at
	unsigned long base=mapAddressSpecToAddress(type,*address,inPageCount);
	if (base==0)
		return B_ERROR;
	// Set up some basic info
	strcpy(name,inName);
	state=inState;
	start_address=base;
	*address=(void *)base;
	finalWrite=inFinalWrite;

	error ("area::createAreaGuts:About to allocate vpages\n");

	allocateVPages(inPageCount);
	error ("area::createAreaGuts:done allocating vpages\n");
	
	// For non-cloned areas, make a new vpage for every page necesssary. 
	if (originalArea==NULL) // Not for cloning
		for (int i=0;i<pageCount;i++) {
			newPage=new (getNthVpage(i)) vpage;
			error ("got a vpage at %x\n",newPage);
			if (fd) {
				error ("area::createAreaGuts:populating vnode\n");
				vnode newVnode;
				newVnode.fd=fd;
				newVnode.offset=offset+i*PAGE_SIZE;
				newVnode.valid=true;
//				vmBlock->vnodeManager->addVNode(newVnode,newPage);
				error ("area::createAreaGuts:calling setup on %x\n",newPage);
				newPage->setup(base+PAGE_SIZE*i,&newVnode,NULL,protect,inState,share);
				error ("area::createAreaGuts:done with setup on %x\n",newPage);
				}
			else {
				error ("area::createAreaGuts:calling setup on %x\n",newPage);
				newPage->setup(base+PAGE_SIZE*i,NULL,NULL,protect,inState);
				error ("area::createAreaGuts:done with setup on %x\n",newPage);
				}
			}
	else // cloned
		// Need to lock other area, here, just in case...
		// Make a copy of each page in the other area...	
		for (int i=0;i<pageCount;i++) {
			vpage *page=originalArea->getNthVpage(i);
			newPage=new (getNthVpage(i)) vpage;
			newPage->setup(base,page->getBacking(),page->getPhysPage(),protect,inState);// Cloned area has the same physical page and backing store...
			base+=PAGE_SIZE;
			}
	error ("Dumping the area's hashtable\n");
	dump();
	vmBlock->areas.add(this);
	return B_OK;
}

status_t area::createAreaMappingFile(char *inName, int inPageCount,void **address, addressSpec type,pageState inState,protectType protect,int fd,size_t offset, mmapSharing share) {
	return createAreaGuts(inName,inPageCount,address,type,inState,protect,true,fd,offset,NULL,share);
	}

status_t area::createArea(char *inName, int inPageCount,void **address, addressSpec type,pageState inState,protectType protect) {
	return createAreaGuts(inName,inPageCount,address,type,inState,protect,false,0,0);
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
	for (int vp=0;vp<pageCount;vp++) {
		vpage *page=getNthVpage(vp);
		if (finalWrite)  {
			page->flush(); 
//			error ("area::freeArea: flushed page %x\n",page);
		}
		page->cleanup();
		}

	for (int i=0;i<fullPages;i++)
		vmBlock->pageMan->freePage(getNthPage(i));
	vmBlock->pageMan->freePage(indexPage);

//	error ("area::freeArea ----------------------------------------------------------------\n");
//	vmBlock->vnodeMan->dump();
//error ("area::freeArea: unlocking \n");
//error ("area::freeArea: ending \n");
	}

// Get area info
status_t area::getInfo(area_info *dest) {
	dest->area=areaID;
	strcpy(dest->name,name);
	dest->size=pageCount*PAGE_SIZE;
	dest->lock=state;
	dest->protection=protection; 
	dest->team=manager->getTeam();
	dest->ram_size=0;
	dest->in_count=in_count;
	dest->out_count=out_count;
	dest->copy_count=0;
	for (int vp=0;vp<pageCount;vp++) {
		vpage *page=getNthVpage(vp);
		if (page->isMapped())
			dest->ram_size+=PAGE_SIZE;
		}
	dest->address=(void *)start_address;
	return B_OK;
	}

bool area::contains(const void *address) {
	unsigned long base=(unsigned long)(address); 
//	error ("area::contains: looking for %d in %d -- %d, value = %d\n",base,getStartAddress(),getEndAddress(), ((getStartAddress()<=base) && (getEndAddress()>=base)));
					
	return ((getStartAddress()<=base) && (base<=getEndAddress()));
	}

// Resize an area. 
status_t area::resize(size_t newSize) {
	size_t oldSize =pageCount*PAGE_SIZE+1;
	// Duh. Nothing to do.
	if (newSize==oldSize)
		return B_OK;
	
	if (newSize>oldSize) {  // Grow the area. Figure out how many pages, allocate them and set them up
		int newPageCount = (newSize - oldSize + PAGE_SIZE - 1) / PAGE_SIZE;
		if (!mapAddressSpecToAddress(EXACT,(void *)(getEndAddress()+1),newPageCount)) // Ensure that the address space is available...
			return B_ERROR;
		int oldPageMax=vpagesOnIndexPage+(fullPages*vpagesOnNextPage); // Figure out what remaining, empty slots we have...
		if (oldPageMax<newSize) {  // Do we have enough room in the existing area?
			page *oldIndexPage=indexPage; // Guess not. Remember where we were...
			int oldVpagesOnIndexPage=vpagesOnIndexPage;
			int oldVpagesOnNextPage=vpagesOnNextPage;
			int oldFullPages=fullPages;
			allocateVPages(newSize/PAGE_SIZE); // Get room
			for (int i=0;i<oldSize/PAGE_SIZE;i++)//Shift over existing space...
				if (i<=vpagesOnIndexPage)  
					memcpy(getNthVpage(i),(void *)((indexPage->getAddress()+sizeof(page *)*fullPages)+(sizeof(vpage)*i)),sizeof(vpage));
				else {
					page *myPage=(page *)(indexPage->getAddress()+((i-vpagesOnIndexPage-1)/vpagesOnNextPage)*sizeof(page *));
					memcpy(getNthVpage(i),(void *)(myPage->getAddress()+(((i-vpagesOnIndexPage-1)%vpagesOnNextPage)*sizeof(vpage))),sizeof(vpage));
					}
			}
		for (int i=oldSize;i<newSize;i+=PAGE_SIZE) { // Fill in the new space
			vpage *newPage=new (getNthVpage(i/PAGE_SIZE)) vpage; // build a new vpage
			newPage->setup(getEndAddress()+PAGE_SIZE*i-1,NULL,NULL,protection,state); // and set it up
			}

		error ("Old size = %d, new size = %d, pageCount = %d\n",oldSize,newSize,pageCount);
		dump();
		}
	else { // Shrinking - not even going to free up the vpages - that could be a bad decision
		size_t newFinalBlock=newSize/PAGE_SIZE;
		for (int i=newFinalBlock;i<pageCount;i++) {
			vpage *oldPage=getNthVpage(i);
			if (finalWrite) 
				oldPage->flush(); 
			oldPage->cleanup();
			}
		}
	return B_OK;
	}

// When the protection for the area changes, the protection for every one of the pages must change
status_t area::setProtection(protectType prot) {
	dump();
	for (int vp=0;vp<pageCount;vp++) {
		vpage *page=getNthVpage(vp);
		error ("setting protection on %x\n",page);
		page->setProtection(prot);
		}
	protection=prot;
	return B_OK;
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
		throw ("area::getByte - attempting an address out of range!");
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
//	error ("area::setInt - start\n");
	vpage *page=findVPage(address);
//	error ("area::setInt - page = %x\n",page);
	if (page)
		page->setInt(address,value,manager);
//	error ("area::setInt - done\n");
	}

// For every one of our vpages, call the vpage's pager
void area::pager(int desperation) {
	for (int vp=0;vp<pageCount;vp++) {
		vpage *page=getNthVpage(vp);
		if (page->pager(desperation))
			out_count++;
		}
	}

// For every one of our vpages, call the vpage's saver
void area::saver(void) {
	for (int vp=0;vp<pageCount;vp++) {
		vpage *page=getNthVpage(vp);
		page->saver();
		}
	}

void area::dump(void) { 
	error ("area::dump: size = %ld, lock = %d, address = %lx\n",pageCount*PAGE_SIZE,state,start_address); 
	for (int vp=0;vp<pageCount;vp++)  {
		error ("Dumping vpage %d of %d\n",vp,pageCount);
		getNthVpage(vp)->dump();
		}
	}

long area::get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries) {
	unsigned long vbase=(unsigned long)address;
	long prevMem=0,tableEntry=-1;
	// Cycle over each "page to find"; 
	for (int byteOffset=0;byteOffset<=numBytes;byteOffset+=PAGE_SIZE) {
		vpage *found=findVPage(((unsigned long)(address))+byteOffset);
		if (!found) return B_ERROR;
		unsigned long mem=found->getPhysPage()->getAddress();
		if (mem!=prevMem+PAGE_SIZE) {
			if (++tableEntry==numEntries)
				return B_ERROR; // Ran out of places to fill in
			prevMem=mem;
			table[tableEntry].address=(void *)mem;
			}
		table[tableEntry].size=+PAGE_SIZE;
		}
	if (++tableEntry==numEntries)
		return B_ERROR; // Ran out of places to fill in
	table[tableEntry].size=0;
}

long area::lock_memory(void *address, ulong numBytes, ulong flags) {
	unsigned long vbase=(unsigned long)address;
	for (int byteOffset=0;byteOffset<=numBytes;byteOffset+=PAGE_SIZE) {
		vpage *found=findVPage((unsigned long)address+byteOffset);
		if (!found) return B_ERROR;
		if (!found->lock(flags)) return B_ERROR;
		}
	return B_OK;
	}

long area::unlock_memory(void *address, ulong numBytes, ulong flags) {
	unsigned long vbase=(unsigned long)address;
	for (int byteOffset=0;byteOffset<=numBytes;byteOffset+=PAGE_SIZE) {
		vpage *found=findVPage((unsigned long)address+byteOffset);
		if (!found) return B_ERROR;
		found->unlock(flags);
		}
	return B_OK;
	}
