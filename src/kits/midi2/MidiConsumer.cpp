/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "protocol.h"

//------------------------------------------------------------------------------

bigtime_t BMidiConsumer::Latency() const
{
	bigtime_t res = 0LL;

	if (LockLooper()) 
	{ 
		res = latency; 
		UnlockLooper(); 
	}

	return res;
}

//------------------------------------------------------------------------------

BMidiConsumer::BMidiConsumer(const char* name)
	: BMidiEndpoint(name)
{
	isConsumer = true;
	latency = 0LL;
	port = 0;
}

//------------------------------------------------------------------------------

BMidiConsumer::~BMidiConsumer()
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiConsumer::_Reserved1() { }
void BMidiConsumer::_Reserved2() { }
void BMidiConsumer::_Reserved3() { }
void BMidiConsumer::_Reserved4() { }
void BMidiConsumer::_Reserved5() { }
void BMidiConsumer::_Reserved6() { }
void BMidiConsumer::_Reserved7() { }
void BMidiConsumer::_Reserved8() { }

//------------------------------------------------------------------------------
