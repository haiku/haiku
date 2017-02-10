/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "cpp.h"


//const struct nothrow_t nothrow = {};

//extern "C" void __pure_virtual()
//{
	//printf("pure virtual function call");
//}

int stderr;

extern "C" int fprintf() { return 0; }
extern "C" void abort() {}
