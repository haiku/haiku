#ifndef _PAGE_H
#define _PAGE_H
#include "vm.h"
#include "list.h"
class page : public node {
		private:
		void *cpuSpecific;
		void *physicalAddress;
		public:
		// Constructors and Destructors and related
		page(void) {cpuSpecific=NULL;physicalAddress=NULL;};
		void setup (void *address) {count=0;physicalAddress=address;};

		// Accessors
		unsigned long getAddress(void) {return (unsigned long)physicalAddress;}

		// Debugging
		void dump(void) { error ("page::dump: Page %p, physicalAddress = %lx\n",this,getAddress()); }

		// External methods for "server" type calls
		void zero(void);

		long count; // Yes, this is large. However, the only atomic add that I have in userland works on int32's. In kernel land, we could shrink this
};
#endif
