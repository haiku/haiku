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

#include <unistd.h>
#include <sys/stat.h>
#include <File.h>

#include "Report.h"
#include "Mask.h"
#include "ImageCache.h"

// Implementation of MaskDescription

MaskDescription::MaskDescription(PDF* pdf, const char* mask, int length, int width, int height, int bpc)
	: fPDF(pdf)
	, fMask(mask)
	, fLength(length)
	, fWidth(width)
	, fHeight(height)
	, fBPC(bpc)
{
}

CacheItem* MaskDescription::NewItem(int id) {
	REPORT(kDebug, -1, "MaskDescription::NewItem called");
	int imageID;
	BString fileName(kMaskPathPrefix);
	const char* name;
	::Mask* mask = NULL;
	
	fileName << id;
	name = fileName.String();

	if (!StoreMask(name)) {
		REPORT(kError, -1, "Could not store mask in cache.");
		return NULL;
	}
	
	imageID = MakePDFMask();
	
	if (imageID == -1) {
		REPORT(kError, -1, "Could not embed mask in PDF file.");
		unlink(name);
	} else {
		mask = new ::Mask(fPDF, imageID, name, Length(), Width(), Height(), BPC());
	}
	return mask;
}

bool MaskDescription::StoreMask(const char* name) {
	bool ok;
	BFile file(name, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK) return false;
	ok = file.Write(Mask(), Length()) == Length();
	if (!ok) {
		unlink(name);
	} else {
	}
	return ok;
}

int MaskDescription::MakePDFMask() {
//	*maskId = PDF_open_image(fPdf, "raw", "memory", (const char *) mask, length, width, height, 1, bpc, "mask");
	BString options;
	int maskID;
	PDF_create_pvf(fPDF, "mask", 0, Mask(), Length(), NULL);
	options << "width " << fWidth << " height " << fHeight << " components 1 bpc " << fBPC;  
	maskID = PDF_load_image(fPDF, "raw", "mask", 0, options.String());
	PDF_delete_pvf(fPDF, "mask", 0);
	return maskID;
}

// Implementation of Mask

Mask::Mask(PDF* pdf, int imageID, const char* fileName, int length, int width, int height, int bpc)
	: fPDF(pdf)
	, fImageID(imageID)
	, fFileName(fileName)
	, fLength(length)
	, fWidth(width)
	, fHeight(height)
	, fBPC(bpc)
{
}

Mask::~Mask() {
	PDF_close_image(fPDF, ImageID());
	unlink(FileName());
}

bool Mask::Equals(CIDescription* description) const {
	REPORT(kDebug, -1, "Mask::Equals called");
	MaskDescription* desc = dynamic_cast<MaskDescription*>(description);
	if (desc && Length() == desc->Length() && Width() == desc->Width() && Height() == desc->Height() && BPC() == desc->BPC()) {
		off_t size;
		BFile file(FileName(), B_READ_ONLY);
		if (file.InitCheck() == B_OK && file.GetSize(&size) == B_OK && size == Length()) {
			char* buffer = new char[Length()];
			if (buffer == NULL) return false;
			bool ok = file.Read(buffer, Length()) == Length() && 
				memcmp(desc->Mask(), buffer, Length()) == 0;
			delete buffer;
			return ok;
		}
	}
	return false;
}
