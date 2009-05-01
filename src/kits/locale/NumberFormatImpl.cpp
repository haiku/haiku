#include <NumberFormatImpl.h>
#include <NumberFormatParameters.h>

// constructor
BNumberFormatImpl::BNumberFormatImpl()
	: BFormatImpl()
{
}

// destructor
BNumberFormatImpl::~BNumberFormatImpl()
{
}

// DefaultFormatParameters
BFormatParameters *
BNumberFormatImpl::DefaultFormatParameters()
{
	return DefaultNumberFormatParameters();
}

// DefaultFormatParameters
const BFormatParameters *
BNumberFormatImpl::DefaultFormatParameters() const
{
	return DefaultNumberFormatParameters();
}

