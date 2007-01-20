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

#ifndef _PRINT_JOB_READER_H
#define _PRINT_JOB_READER_H

#include <File.h>
#include <Message.h>

class PrintJobPage {
	BFile fJobFile; // the job file
	off_t fNextPicture; // offset to first picture
	int32 fNumberOfPictures; // of this page
	int32 fPicture; // the picture returned by NextPicture()
	status_t fStatus;
		
public:
	PrintJobPage();
	PrintJobPage(const PrintJobPage& copy);
	PrintJobPage(BFile* jobFile, off_t start);
	status_t InitCheck() const;
	
	int32 NumberOfPictures() const { return fNumberOfPictures; }
	
	status_t NextPicture(BPicture& picture, BPoint& p, BRect& r);
	
	PrintJobPage& operator=(const PrintJobPage& copy);
};

class PrintJobReader {
	BFile fJobFile;  // the job file
	int32 fNumberOfPages; // the number of pages in the job file
	int32 fFirstPage; // the page number of the first page
	BMessage fJobSettings; // the settings extracted from the job file
	off_t* fPageIndex; // start positions of pages in the job file

	void BuildPageIndex();

public:
	PrintJobReader(BFile* jobFile);
	virtual ~PrintJobReader();
		
		// test after construction if this is a valid job file
	status_t InitCheck() const;
	
		// accessors to informations from job file
	int32 NumberOfPages() const { return fNumberOfPages; }
	int32 FirstPage() const { return fFirstPage; }
	int32 LastPage() const { return fFirstPage + fNumberOfPages - 1; }
	const BMessage* JobSettings() const { return &fJobSettings; }
	BRect PaperRect() const;
	BRect PrintableRect() const;
	void GetResolution(int32 *xdpi, int32 *ydpi) const;
	float GetScale() const;
		
		// retrieve page
	status_t GetPage(int no, PrintJobPage& pjp);
};

#endif

