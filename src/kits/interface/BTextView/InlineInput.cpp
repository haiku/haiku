#include "InlineInput.h"

#include <cstdlib>

_BInlineInput_::_BInlineInput_(BMessenger messenger)
	:
	fMessenger(messenger),
	fActive(false),
	fOffset(0),
	fLength(0)
{
}


_BInlineInput_::~_BInlineInput_()
{
}


/*! \brief Returns a pointer to the Input Server Method BMessenger
	which requested the transaction.
*/
const BMessenger *
_BInlineInput_::Method() const
{
	return &fMessenger;
}


bool
_BInlineInput_::IsActive() const
{
	return fActive;
}


void
_BInlineInput_::SetActive(bool active)
{
	fActive = active;
}


int32
_BInlineInput_::Length() const
{
	return fLength;
}


void
_BInlineInput_::SetLength(int32 len)
{
	fLength = len;
}


int32
_BInlineInput_::Offset() const
{
	return fOffset;
}


void
_BInlineInput_::SetOffset(int32 offset)
{
	fOffset = offset;
}
