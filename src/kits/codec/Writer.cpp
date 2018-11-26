/* 
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Writer.h>

#include <stdio.h>


namespace BCodecKit {


BWriter::BWriter()
	:
	fTarget(NULL),
	fMediaPlugin(NULL)
{
}


BWriter::~BWriter()
{
}


BDataIO*
BWriter::Target() const
{
	return fTarget;
}


void
BWriter::_Setup(BDataIO* target)
{
	fTarget = target;
}


status_t
BWriter::Perform(perform_code code, void* data)
{
	return B_OK;
}


void BWriter::_ReservedWriter1() {}
void BWriter::_ReservedWriter2() {}
void BWriter::_ReservedWriter3() {}
void BWriter::_ReservedWriter4() {}
void BWriter::_ReservedWriter5() {}


}
