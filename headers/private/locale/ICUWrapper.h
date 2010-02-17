/* 
** Copyright 2009, Adrien Destugues, pulkomandy@gmail.com.
** Distributed under the terms of the MIT License.
*/
/* This file holds various wrapper functions to interface easily between ICU
 * and the Be API.
 */


#ifndef __ICU_WRAPPER_H__
#define __ICU_WRAPPER_H__

#include <String.h>

#include <unicode/bytestream.h>


/* Convert UnicodeString to BString needs an ICU ByteSink to do the work */
class BStringByteSink : public ByteSink {
	public:
		BStringByteSink(BString* dest) : dest_(dest) {}
		virtual void Append(const char* data, int32_t n)
			{ dest_->Append(data, n); }
	private:
		BString* dest_;
		BStringByteSink(); 
		BStringByteSink(const BStringByteSink &); 
		BStringByteSink &operator=(const BStringByteSink &); 
};


#endif
