/*****************************************************************************/
// Haiku InputServer
//
// This is the InputServerMethod implementation
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

#include <InputServerMethod.h>
#include <Messenger.h>
#include "InputServer.h"

/**
 *  Method: BInputServerMethod::BInputServerMethod()
 *   Descr: 
 */
BInputServerMethod::BInputServerMethod(const char *name,
                                       const uchar *icon)
{
	fOwner = new _BMethodAddOn_(this);
}


/**
 *  Method: BInputServerMethod::~BInputServerMethod()
 *   Descr: 
 */
BInputServerMethod::~BInputServerMethod()
{
}


/**
 *  Method: BInputServerMethod::MethodActivated()
 *   Descr: 
 */
status_t
BInputServerMethod::MethodActivated(bool active)
{
    return B_OK;
}


/**
 *  Method: BInputServerMethod::EnqueueMessage()
 *   Descr: 
 */
status_t
BInputServerMethod::EnqueueMessage(BMessage *message)
{
	return ((InputServer*)be_app)->EnqueueMethodMessage(message);
}


/**
 *  Method: BInputServerMethod::SetName()
 *   Descr: 
 */
status_t
BInputServerMethod::SetName(const char *name)
{
    status_t dummy;

    return dummy;
}


/**
 *  Method: BInputServerMethod::SetIcon()
 *   Descr: 
 */
status_t
BInputServerMethod::SetIcon(const uchar *icon)
{
    status_t dummy;

    return dummy;
}


/**
 *  Method: BInputServerMethod::SetMenu()
 *   Descr: 
 */
status_t
BInputServerMethod::SetMenu(const BMenu *menu,
                            const BMessenger target)
{
    status_t dummy;

    return dummy;
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod1()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod1()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod2()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod2()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod3()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod3()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod4()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod4()
{
}


