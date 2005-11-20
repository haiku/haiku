#ifndef USING_MESSAGE4
//------------------------------------------------------------------------------
//	MessageField.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <ByteOrder.h>

// Project Includes ------------------------------------------------------------
#include "MessageField.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

const char* BMessageField::sNullData = "\0\0\0\0\0\0\0\0";

//------------------------------------------------------------------------------
void BMessageField::PrintToStream(const char* name) const
{
	int32 type = B_BENDIAN_TO_HOST_INT32(Type());
	printf("    entry %14s, type='%.4s', c=%2ld, ",
		   name, (char*)&type, CountItems());

	for (int32 i = 0; i < CountItems(); ++i)
	{
		if (i)
		{
			printf("                                            ");
		}
		PrintDataItem(i);
	}
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

#endif	// USING_MESSAGE4
