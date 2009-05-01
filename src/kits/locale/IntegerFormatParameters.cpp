#include <IntegerFormatParameters.h>

// constructor
BIntegerFormatParameters::BIntegerFormatParameters(
	const BIntegerFormatParameters *parent)
	: BNumberFormatParameters(parent),
	  fParent(parent)
{
}

// copy constructor
BIntegerFormatParameters::BIntegerFormatParameters(
	const BIntegerFormatParameters &other)
	: BNumberFormatParameters(other),
	  fParent(other.fParent)
{
}

// destructor
BIntegerFormatParameters::~BIntegerFormatParameters()
{
}

// SetParentIntegerParameters
void
BIntegerFormatParameters::SetParentIntegerParameters(
	const BIntegerFormatParameters *parent)
{
	fParent = parent;
	SetParentNumberParameters(parent);
}

// ParentIntegerParameters
const BIntegerFormatParameters *
BIntegerFormatParameters::ParentIntegerParameters() const
{
	return fParent;
}

// =
BIntegerFormatParameters &
BIntegerFormatParameters::operator=(const BIntegerFormatParameters &other)
{
	BNumberFormatParameters::operator=(other);
	fParent = other.fParent;
	return *this;
}

