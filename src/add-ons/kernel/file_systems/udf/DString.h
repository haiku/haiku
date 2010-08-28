/*
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _D_STRING_H
#define _D_STRING_H


#include "UdfDebug.h"

#include "UdfString.h"

#include <util/kernel_cpp.h>


/*! \brief Fixed-length d-string class that takes a UdfString as input
	and provides a properly formatted ECMA-167 d-string of the given
	field length as ouput.

	For d-string info, see: ECMA-167 1/7.2.12, UDF-2.50 2.1.3
*/
class DString {
public:
								DString();
								DString(const DString &ref);
								DString(const UdfString &string,
									uint8 fieldLength);
								DString(const char *utf8, uint8 fieldLength);
								~DString();

			uint8				Length() const { return fLength; }

			void				SetTo(const DString &ref);
			void				SetTo(const UdfString &string, uint8 fieldLength);
			void				SetTo(const char *utf8, uint8 fieldLength);

			const uint8* 		String() const { return fString; }

private:
			void 				_Clear();

private:
			uint8				fLength;
			uint8				*fString;
};


#endif	// _D_STRING_H
