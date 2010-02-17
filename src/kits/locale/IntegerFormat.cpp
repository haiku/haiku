#include <IntegerFormat.h>
#include <IntegerFormatImpl.h>

// copy constructor
BIntegerFormat::BIntegerFormat(const BIntegerFormat &other)
	: BNumberFormat(other),
	  BIntegerFormatParameters(other)
{
}

// destructor
BIntegerFormat::~BIntegerFormat()
{
}

// Format
status_t
BIntegerFormat::Format(int64 number, BString *buffer) const
{
	return B_ERROR;
}

// Format
status_t
BIntegerFormat::Format(int64 number, BString *buffer,
					   format_field_position *positions, int32 positionCount,
					   int32 *fieldCount, bool allFieldPositions) const
{
	return B_ERROR;
}

// =
BIntegerFormat &
BIntegerFormat::operator=(const BIntegerFormat &other)
{
	BNumberFormat::operator=(other);
	BIntegerFormatParameters::operator=(other);
	return *this;
}

// constructor
BIntegerFormat::BIntegerFormat(BIntegerFormatImpl *impl)
	: BNumberFormat(),
	  BIntegerFormatParameters(impl ? impl->DefaultIntegerFormatParameters()
	  							    : NULL)
{
}

