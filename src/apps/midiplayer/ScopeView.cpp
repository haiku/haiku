/*
 * Copyright (c) 2004 Matthijs Hollemans
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

#include "ScopeView.h"

//------------------------------------------------------------------------------

ScopeView::ScopeView()
	: BView(BRect(0, 0, 127, 47), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
	        B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(0, 0, 0);
	enabled = true;
	haveFile = false;
}

//------------------------------------------------------------------------------

ScopeView::~ScopeView()
{
}

//------------------------------------------------------------------------------

void ScopeView::SetEnabled(bool flag)
{
	enabled = flag;
}

//------------------------------------------------------------------------------

void ScopeView::SetHaveFile(bool flag)
{
	haveFile = flag;
}

//------------------------------------------------------------------------------
