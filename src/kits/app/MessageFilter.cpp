//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MessageFilter.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BMessageFilter class creates objects that filter
//					in-coming BMessages.  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BMessageFilter::BMessageFilter(uint32 inWhat, filter_hook func)
	:	fFiltersAny(false),
		what(inWhat),
		fDelivery(B_ANY_DELIVERY),
		fSource(B_ANY_SOURCE),
		fLooper(NULL),
		fFilterFunction(func)
{
}
//------------------------------------------------------------------------------
BMessageFilter::BMessageFilter(message_delivery delivery, message_source source,
							   filter_hook func)
	:	fFiltersAny(true),
		what(0),
		fDelivery(delivery),
		fSource(source),
		fLooper(NULL),
		fFilterFunction(func)
{
}
//------------------------------------------------------------------------------
BMessageFilter::BMessageFilter(message_delivery delivery, message_source source,
							   uint32 inWhat, filter_hook func)
	:	fFiltersAny(false),
		what(inWhat),
		fDelivery(delivery),
		fSource(source),
		fLooper(NULL),
		fFilterFunction(func)
{
}
//------------------------------------------------------------------------------
BMessageFilter::BMessageFilter(const BMessageFilter& filter)
{
	*this = filter;
}
//------------------------------------------------------------------------------
BMessageFilter::BMessageFilter(const BMessageFilter* filter)
{
	*this = *filter;
}
//------------------------------------------------------------------------------
BMessageFilter::~BMessageFilter()
{
	;
}
//------------------------------------------------------------------------------
BMessageFilter& BMessageFilter::operator=(const BMessageFilter& from)
{
	fFiltersAny			= from.FiltersAnyCommand();
	what				= from.Command();
	fDelivery			= from.MessageDelivery();
	fSource				= from.MessageSource();
	fFilterFunction		= from.FilterFunction();

	SetLooper(from.Looper());

	return *this;
}
//------------------------------------------------------------------------------
filter_result BMessageFilter::Filter(BMessage* message, BHandler** target)
{
	if (fFilterFunction)
	{
		return fFilterFunction(message, target, this);
	}

	return B_DISPATCH_MESSAGE;
}
//------------------------------------------------------------------------------
message_delivery BMessageFilter::MessageDelivery() const
{
	return fDelivery;
}
//------------------------------------------------------------------------------
message_source BMessageFilter::MessageSource() const
{
	return fSource;
}
//------------------------------------------------------------------------------
uint32 BMessageFilter::Command() const
{
	return what;
}
//------------------------------------------------------------------------------
bool BMessageFilter::FiltersAnyCommand() const
{
	return fFiltersAny;
}
//------------------------------------------------------------------------------
BLooper* BMessageFilter::Looper() const
{
	return fLooper;
}
//------------------------------------------------------------------------------
void BMessageFilter::_ReservedMessageFilter1()
{
	;
}
//------------------------------------------------------------------------------
void BMessageFilter::_ReservedMessageFilter2()
{
	;
}
//------------------------------------------------------------------------------
void BMessageFilter::SetLooper(BLooper* owner)
{
	// TODO: implement locking?
	fLooper = owner;
}
//------------------------------------------------------------------------------
filter_hook BMessageFilter::FilterFunction() const
{
	return fFilterFunction;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

