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
		page(void) {cpuSpecific=NULL;physicalAddress=NULL;};
		void setup (void *address) {count=0;physicalAddress=address;};
		void zero(void);
		unsigned long getAddress(void) {return (unsigned long)physicalAddress;}
		void dump(void) { printf ("Page %x, physicalAddress = %x\n",this,getAddress()); }
};
#endif
