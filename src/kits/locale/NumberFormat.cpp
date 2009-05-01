#include <NumberFormat.h>
#include <NumberFormatImpl.h>

// copy constructor
BNumberFormat::BNumberFormat(const BNumberFormat &other)
	: BFormat(other)
{
}

// destructor
BNumberFormat::~BNumberFormat()
{
}

// =
BNumberFormat &
BNumberFormat::operator=(const BNumberFormat &other)
{
	BFormat::operator=(other);
	return *this;
}

// constructor
BNumberFormat::BNumberFormat(BNumberFormatImpl *impl)
	: BFormat(impl)
{
}

// NumberFormatImpl
inline
BNumberFormatImpl *
BNumberFormat::NumberFormatImpl() const
{
	return static_cast<BNumberFormatImpl*>(fImpl);
}

