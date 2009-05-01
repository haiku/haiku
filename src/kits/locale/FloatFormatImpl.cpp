#include <FloatFormatImpl.h>
#include <FloatFormatParameters.h>

// constructor
BFloatFormatImpl::BFloatFormatImpl()
	: BNumberFormatImpl()
{
}

// destructor
BFloatFormatImpl::~BFloatFormatImpl()
{
}

// DefaultNumberFormatParameters
BNumberFormatParameters *
BFloatFormatImpl::DefaultNumberFormatParameters()
{
	return DefaultFloatFormatParameters();
}

// DefaultNumberFormatParameters
const BNumberFormatParameters *
BFloatFormatImpl::DefaultNumberFormatParameters() const
{
	return DefaultFloatFormatParameters();
}

