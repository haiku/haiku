#ifndef _PAGE_H
#define _PAGE_H
#include "vm.h"
#include "list.h"
class page : public node {
		private:
		void *cpuSpecific;
		void *physicalAddress;
		public:
		int count;
		page(void *address) : physicalAddress(address) {count=0;} ;
		void zero(void);
		unsigned long getAddress(void) {return (unsigned long)physicalAddress;}
		void dump(void) { printf ("Page %x, physicalAddress = %x\n",this,getAddress()); }
};
#endif
