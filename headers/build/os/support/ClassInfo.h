/******************************************************************************
/
/	File:			ClassInfo.h
/
/	Description:	C++ class identification and casting macros
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _CLASS_INFO_H
#define _CLASS_INFO_H

#include <typeinfo>

// deprecated, use standard RTTI calls instead 

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

/*-------------------------------------------------------------*/
/*--------- Class Info Macros ---------------------------------*/

#define class_name(ptr)				((typeid(*(ptr))).name())
#define cast_as(ptr, class)			(dynamic_cast<class*>(ptr))
#define is_kind_of(ptr, class)		(cast_as(ptr, class) != 0)
#define is_instance_of(ptr, class)	(typeid(*(ptr)) == typeid(class))
	
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#ifdef USE_OPENBEOS_NAMESPACE
}	// namespace OpenBeOS
using namespace OpenBeOS;
#endif

#endif /* _CLASS_INFO_H */
