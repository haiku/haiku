/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

/****************************************************************************
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
**                                                                         **
**                          DANGER, WILL ROBINSON!                         **
**                                                                         **
** The interfaces contained here are part of BeOS's                        **
**                                                                         **
**                     >> PRIVATE NOT FOR PUBLIC USE <<                    **
**                                                                         **
**                                                       implementation.   **
**                                                                         **
** These interfaces              WILL CHANGE        in future releases.    **
** If you use them, your app     WILL BREAK         at some future time.   **
**                                                                         **
** (And yes, this does mean that binaries built from OpenTracker will not  **
** be compatible with some future releases of the OS.  When that happens,  **
** we will provide an updated version of this file to keep compatibility.) **
**                                                                         **
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
****************************************************************************/

#ifndef _TEXTVIEWSUPPORT_H
#define _TEXTVIEWSUPPORT_H

// This is a stub of text view width buffer
// It relies on the implementation of _BWidthBuffer_ in libbe and should
// not be modified


struct _width_table_ {
#if B_BEOS_VERSION_DANO
	BFont font;				// corresponding font
#else
	int32 fontCode;			// font code
	float fontSize;			// font size
#endif
	int32 hashCount;		// number of hashed items
	int32 tableCount;		// size of table
	void *widths;			// width table	
};
	
	
template<class T> class _BTextViewSupportBuffer_ {
public:
	_BTextViewSupportBuffer_(int32, int32);
	virtual ~_BTextViewSupportBuffer_();
					
protected:
	int32 mExtraCount;	
	int32 mItemCount;	
	int32 mBufferCount;	
	T *mBuffer;		
};	

class _BWidthBuffer_ : public _BTextViewSupportBuffer_<_width_table_> {
public:
	_BWidthBuffer_();
	virtual ~_BWidthBuffer_();

	float StringWidth(const char *inText, int32 fromOffset, int32 length,
		const BFont *inStyle);
};



#endif /* _TEXTVIEWSUPPORT_H */
