/*

Mask Cache Item.

Copyright (c) 2003 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef _MASK_CACHE_ITEM_H
#define _MASK_CACHE_ITEM_H

#include <String.h>

#include "pdflib.h"
#include "Cache.h"

class MaskDescription : public CIDescription {
public:
	MaskDescription(PDF* pdf, const char* mask, int length, int width, int height, int bpc);
	
	CacheItem* NewItem(int id);

	const char* Mask() const { return fMask; }
	int Length() const { return fLength; }
	int Width() const { return fWidth; }
	int Height() const { return fHeight; }
	int BPC() const { return fBPC; }

private:
	bool StoreMask(const char* name);
	int MakePDFMask();

	PDF* fPDF;
	const char* fMask;
	int fLength;
	int fWidth;
	int fHeight;
	int fBPC;
};

class Mask : public CacheItem {
public:
	Mask(PDF* pdf, int imageID, const char* fileName, int length, int width, int height, int bpc);
	~Mask();
	
	int ImageID() const { return fImageID; };
	const char* FileName() const  { return fFileName.String(); };
	int Length() const { return fLength; };
	int Width() const { return fWidth; }
	int Height() const { return fHeight; }
	int BPC() const { return fBPC; }
	bool Equals(CIDescription* desc) const;

private:
	PDF* fPDF;
	int fImageID;
	BString fFileName;
	int fLength;
	int fWidth;
	int fHeight;
	int fBPC;
};

#endif
