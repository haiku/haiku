#ifndef _PAGE_H
#define _PAGE_H
#include "vm.h"
#include "list.h"
class page : public node {
		private:
		void *cpuSpecific;
		void *physicalAddress;
		public:
		page(void *address) : physicalAddress(address) {} ;
		void zero(void);
		unsigned long getAddress(void) {return (unsigned long)physicalAddress;}
};
#endif
