#ifndef _VM_HEADERBLOCK
#define _VM_HEADERBLOCK


#ifndef I_AM_VM_INTERFACE
class poolarea;
class poolvpage;
class poolvnode;
class pageManager;
class swapFileManager;
class cacheManager;
#endif

struct vmHeaderBlock
{
	poolarea *areaPool;
	poolvpage *vpagePool;
	poolvnode *vnodePool;
	pageManager *pageMan; 
	swapFileManager *swapMan;
	cacheManager *cacheMan;
};

#endif
