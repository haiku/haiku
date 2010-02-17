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
	return B_ERROR;
}

// Format
status_t
BFloatFormat::Format(double number, BString *buffer,
					 format_field_position *positions, int32 positionCount,
					 int32 *fieldCount, bool allFieldPositions) const
{
	return B_ERROR;
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
	: BNumberFormat(),
	  BFloatFormatParameters(impl ? impl->DefaultFloatFormatParameters()
	  							    : NULL)
{
}

