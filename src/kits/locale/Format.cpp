#include <Format.h>
#include <FormatImpl.h>

// copy constructor
BFormat::BFormat(const BFormat &other)
	: fImpl(other.fImpl)
{
}

// destructor
BFormat::~BFormat()
{
}

// =
BFormat &
BFormat::operator=(const BFormat &other)
{
	fImpl = other.fImpl;
	return *this;
}

// constructor
BFormat::BFormat(BFormatImpl *impl)
	: fImpl(impl)
{
}

