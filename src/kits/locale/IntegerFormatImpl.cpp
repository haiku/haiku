#include <IntegerFormatImpl.h>
#include <IntegerFormatParameters.h>

// constructor
BIntegerFormatImpl::BIntegerFormatImpl()
	: BNumberFormatImpl()
{
}

// destructor
BIntegerFormatImpl::~BIntegerFormatImpl()
{
}

// DefaultNumberFormatParameters
BNumberFormatParameters *
BIntegerFormatImpl::DefaultNumberFormatParameters()
{
	return DefaultIntegerFormatParameters();
}

// DefaultNumberFormatParameters
const BNumberFormatParameters *
BIntegerFormatImpl::DefaultNumberFormatParameters() const
{
	return DefaultIntegerFormatParameters();
}

