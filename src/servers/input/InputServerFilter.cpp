/*****************************************************************************/
// Haiku InputServer
//
// This is the InputServerFilter implementation
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002-2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include <Region.h>
#include <InputServerFilter.h>
#include "InputServer.h"

/**
 *  Method: BInputServerFilter::BInputServerFilter()
 *   Descr: 
 */
BInputServerFilter::BInputServerFilter()
{
	CALLED();
}


/**
 *  Method: BInputServerFilter::~BInputServerFilter()
 *   Descr: 
 */
BInputServerFilter::~BInputServerFilter()
{
	CALLED();
}


/**
 *  Method: BInputServerFilter::InitCheck()
 *   Descr: 
 */
status_t
BInputServerFilter::InitCheck()
{
	CALLED();
	return B_OK;
}


/**
 *  Method: BInputServerFilter::Filter()
 *   Descr: 
 */
filter_result
BInputServerFilter::Filter(BMessage *message,
                           BList *outList)
{	
	CALLED();
	return B_DISPATCH_MESSAGE;
}


/**
 *  Method: BInputServerFilter::GetScreenRegion()
 *   Descr: 
 */
status_t
BInputServerFilter::GetScreenRegion(BRegion *region) const
{
	if (!region)
		return B_BAD_VALUE;

	*region = BRegion(((InputServer*)be_app)->ScreenFrame());
	return B_OK;
}


/**
 *  Method: BInputServerFilter::_ReservedInputServerFilter1()
 *   Descr: 
 */
void
BInputServerFilter::_ReservedInputServerFilter1()
{
}


/**
 *  Method: BInputServerFilter::_ReservedInputServerFilter2()
 *   Descr: 
 */
void
BInputServerFilter::_ReservedInputServerFilter2()
{
}


/**
 *  Method: BInputServerFilter::_ReservedInputServerFilter3()
 *   Descr: 
 */
void
BInputServerFilter::_ReservedInputServerFilter3()
{
}


/**
 *  Method: BInputServerFilter::_ReservedInputServerFilter4()
 *   Descr: 
 */
void
BInputServerFilter::_ReservedInputServerFilter4()
{
}


