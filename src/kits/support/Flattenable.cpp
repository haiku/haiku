// Quick and dirty implementation of the BFlattenable class.
// Just here to be able to compile and test BPath.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#include <stdio.h>

#include <Flattenable.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// AllowsTypeCode
bool
BFlattenable::AllowsTypeCode(type_code code) const
{
	return (TypeCode() == code);
}

// destructor
BFlattenable::~BFlattenable()
{
}


void BFlattenable::_ReservedFlattenable1() {}
void BFlattenable::_ReservedFlattenable2() {}
void BFlattenable::_ReservedFlattenable3() {}


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif
