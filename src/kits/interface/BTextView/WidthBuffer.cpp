// A quick and dirty implementation of _BWidthBuffer_, needed to compile
// OpenTracker. We'll want to implement it correctly.

#include <Font.h>

#include "WidthBuffer.h"

_BWidthBuffer_::_BWidthBuffer_()
{
	//TODO: Implement
}


_BWidthBuffer_::~_BWidthBuffer_()
{
	//TODO: Implement
}


float
_BWidthBuffer_::StringWidth(const char *inText, int32 fromOffset, int32 length,
		const BFont *inStyle)
{
	// TODO: Should use local hashed items
	return inStyle->StringWidth(inText + fromOffset, length);
}


float
_BWidthBuffer_::StringWidth(_BTextGapBuffer_ &inBuffer, int32 fromOffset, int32 length,
		const BFont *inStyle)
{
	return inStyle->StringWidth(inBuffer.Text() + fromOffset, length);
}


bool
_BWidthBuffer_::FindTable(const BFont *inStyle, int32 *outIndex)
{
	float fontSize = inStyle->Size();
	int32 fontCode = inStyle->FamilyAndStyle();

	for (int32 i = 0; i < fItemCount; i++) {
#if B_BEOS_VERSION_DANO
		if (*inStyle == fBuffer[i].font) {
#else
		if (fontSize == fBuffer[i].fontSize && fontCode == fBuffer[i].fontCode) {
#endif
			*outIndex = i;
			return true;
		}
	}
	*outIndex = -1;
	
	return false;
}


int32
_BWidthBuffer_::InsertTable(const BFont *font)
{
	//TODO: Implement
	return B_ERROR;
}


bool 
_BWidthBuffer_::GetEscapement(uint32 code, int32 index, float *escapement)
{
	//TODO: Implement
	return false;
}


uint32
_BWidthBuffer_::Hash(uint32 val)
{
	//TODO: Implement
	return B_ERROR;
}


void
_BWidthBuffer_::HashEscapements(const char *inText, int32 numChars, int32,
		int32, const BFont *inStyle)
{
	//TODO: Implement
}
