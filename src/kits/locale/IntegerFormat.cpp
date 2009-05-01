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
	if (!fImpl)
		return B_NO_INIT;
	return IntegerFormatImpl()->Format(this, number, buffer);
}

// Format
status_t
BIntegerFormat::Format(int64 number, BString *buffer,
					   format_field_position *positions, int32 positionCount,
					   int32 *fieldCount, bool allFieldPositions) const
{
	if (!fImpl)
		return B_NO_INIT;
	return IntegerFormatImpl()->Format(this,number, buffer, positions,
		positionCount, fieldCount, allFieldPositions);
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
	: BNumberFormat(impl),
	  BIntegerFormatParameters(impl ? impl->DefaultIntegerFormatParameters()
	  							    : NULL)
{
}

// IntegerFormatImpl
inline
BIntegerFormatImpl *
BIntegerFormat::IntegerFormatImpl() const
{
	return static_cast<BIntegerFormatImpl*>(fImpl);
}

