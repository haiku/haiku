//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		ServerPicture.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Server-side counterpart to BPicture
//  
//------------------------------------------------------------------------------
#include "TokenHandler.h"
#include "ServerPicture.h"

TokenHandler picture_token_handler;

ServerPicture::ServerPicture(void)
{
	_token=picture_token_handler.GetToken();
	
	_initialized=false;
	
	int8 *ptr;
	_area=create_area("ServerPicture",(void**)&ptr,B_ANY_ADDRESS,B_PAGE_SIZE,
		B_NO_LOCK,B_READ_AREA | B_WRITE_AREA);
	
	if(_area!=B_BAD_VALUE && _area!=B_NO_MEMORY && _area!=B_ERROR)
		_initialized=true;
}

ServerPicture::~ServerPicture(void)
{
}
