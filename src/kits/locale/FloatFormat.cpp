#include <FloatFormat.h>
#include <FloatFormatImpl.h>

// copy constructor
BFloatFormat::BFloatFormat(const BFloatFormat &other)
	: BNumberFormat(other),
	  BFloatFormatParameters(other)
{
}

// destructor
BFloatFormat::~BFloatFormat()
{
}

// Format
status_t
BFloatFormat::Format(double number, BString *buffer) const
{
	if (!fImpl)
		return B_NO_INIT;
	return FloatFormatImpl()->Format(this, number, buffer);
}

// Format
status_t
BFloatFormat::Format(double number, BString *buffer,
					 format_field_position *positions, int32 positionCount,
					 int32 *fieldCount, bool allFieldPositions) const
{
	if (!fImpl)
		return B_NO_INIT;
	return FloatFormatImpl()->Format(this,number, buffer, positions,
		positionCount, fieldCount, allFieldPositions);
}

// =
BFloatFormat &
BFloatFormat::operator=(const BFloatFormat &other)
{
	BNumberFormat::operator=(other);
	BFloatFormatParameters::operator=(other);
	return *this;
}

// constructor
BFloatFormat::BFloatFormat(BFloatFormatImpl *impl)
	: BNumberFormat(impl),
	  BFloatFormatParameters(impl ? impl->DefaultFloatFormatParameters()
	  							    : NULL)
{
}

// FloatFormatImpl
inline
BFloatFormatImpl *
BFloatFormat::FloatFormatImpl() const
{
	return static_cast<BFloatFormatImpl*>(fImpl);
}

