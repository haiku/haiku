/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
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

#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H
#include <InterfaceKit.h>
#include <SupportKit.h>

//class PrinterDriver;
#include "PrinterDriver.h"
#include "BlockingWindow.h"
#include "PrintUtils.h"

class StatusWindow : public HWindow 
{
private:
	int32 fPass;
	int32 fPages;
	
	BString fPage;
	int32 fPageCount;
	BStringView *fPageLabel;
	BStatusBar *fPageStatus;
	BButton *fCancel;
	PrinterDriver *fPrinterDriver;
	int32      fReportIndex;
	BTextView* fReport;
	BScrollView* fReportScrollView;
	sem_id     fCloseSem;
	CloseOption fCloseOption;

	void UpdateReport();

public:
	typedef HWindow inherited;
	
	StatusWindow(int32 passes, int32 pages, PrinterDriver *pd);
	
	void MessageReceived(BMessage *msg);
	void NextPage();
	void WaitForClose();
};
#endif
