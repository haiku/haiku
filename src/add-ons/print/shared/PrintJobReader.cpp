/*

PrintJobReader

Copyright (c) 2002 OpenBeOS. 

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

#include <stdio.h>
#include <PrintJob.h>
#include "PrintJobReader.h"

// Implementation of PrintJobPage

PrintJobPage::PrintJobPage()
	: fNextPicture(-1)
	, fNumberOfPictures(0)
	, fPicture(0)
	, fStatus(B_ERROR)
{	
}

PrintJobPage::PrintJobPage(const PrintJobPage& copy)
	: fJobFile(copy.fJobFile)
	, fNextPicture(copy.fNextPicture)
	, fNumberOfPictures(copy.fNumberOfPictures)
	, fPicture(copy.fPicture)
	, fStatus(copy.fStatus)
{
}

PrintJobPage& PrintJobPage::operator=(const PrintJobPage& copy) {
	if (this != &copy) {
		fJobFile = copy.fJobFile;
		fNextPicture = copy.fNextPicture;
		fNumberOfPictures = copy.fNumberOfPictures;
		fPicture = copy.fPicture;
		fStatus = copy.fStatus;
	}
	return *this;
}

PrintJobPage::PrintJobPage(BFile* jobFile, off_t start) 
	: fJobFile(*jobFile)
	, fPicture(0)
	, fStatus(B_ERROR)
{
	off_t size;
	if (fJobFile.GetSize(&size) != B_OK || start > size) return;
	if (fJobFile.Seek(start, SEEK_SET) != start) return;
	if (fJobFile.Read(&fNumberOfPictures, sizeof(fNumberOfPictures)) == sizeof(fNumberOfPictures)) {
		fJobFile.Seek(40 + sizeof(off_t), SEEK_CUR);
		fNextPicture = fJobFile.Position();
		fStatus = B_OK;
	}
}

status_t PrintJobPage::InitCheck() const {
	return fStatus;
}

status_t PrintJobPage::NextPicture(BPicture& picture, BPoint& point, BRect& rect) {
	if (fPicture >= fNumberOfPictures) return B_ERROR;
	fPicture ++;
	
	fJobFile.Seek(fNextPicture, SEEK_SET);
	fJobFile.Read(&point, sizeof(point));
	fJobFile.Read(&rect, sizeof(rect));
	status_t rc = picture.Unflatten(&fJobFile);
	fNextPicture = fJobFile.Position();
	if (rc != B_OK) {
		fPicture = fNumberOfPictures;
	}
	return rc;
}


// Implementation of PrintJobReader

PrintJobReader::PrintJobReader(BFile* jobFile) 
	: fJobFile(*jobFile)
	, fNumberOfPages(-1)
	, fPageIndex(NULL)
{
	print_file_header header;
	fJobFile.Seek(0, SEEK_SET);
	if (fJobFile.Read(&header, sizeof(header)) == sizeof(header) &&
		fJobSettings.Unflatten(&fJobFile) == B_OK) {
		fNumberOfPages = header.page_count;
		fFirstPage = header.first_page;
		BuildPageIndex();
	}
}

PrintJobReader::~PrintJobReader() {
	delete[] fPageIndex;
}

status_t PrintJobReader::InitCheck() const {
	return fNumberOfPages > 0 ? B_OK : B_ERROR;
}

void PrintJobReader::BuildPageIndex() {
	fPageIndex = new off_t[fNumberOfPages];
	
	for (int page = 0; page < fNumberOfPages; page ++) {
		int32 pictures;
		off_t next_page;
			// add position to page index
		fPageIndex[page] = fJobFile.Position();
			
			// determine start position of next page
		if (fJobFile.Read(&pictures, sizeof(pictures)) == sizeof(pictures) &&
			fJobFile.Read(&next_page, sizeof(next_page)) == sizeof(next_page) &&
			fPageIndex[page] < next_page) {
			fJobFile.Seek(next_page, SEEK_SET);
		} else {
			fNumberOfPages = 0; delete fPageIndex; fPageIndex = NULL;
			return;
		}
	}
}

status_t PrintJobReader::GetPage(int page, PrintJobPage& pjp) {
	if (0 <= page && page < fNumberOfPages) {
		PrintJobPage p(&fJobFile, fPageIndex[page]);
		if (p.InitCheck() == B_OK) {
			pjp = p; return B_OK;
		}
	}
	return B_ERROR;
}

BRect PrintJobReader::PaperRect() const {
	BRect r;
	fJobSettings.FindRect("paper_rect", &r);
	return r;
}

BRect PrintJobReader::PrintableRect() const {
	BRect r;
	fJobSettings.FindRect("printable_rect", &r);
	return r;
}

void PrintJobReader::GetResolution(int32 *xdpi, int32 *ydpi) const {
	fJobSettings.FindInt32("xres", xdpi);
	fJobSettings.FindInt32("yres", ydpi);
}

