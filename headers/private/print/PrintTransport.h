/*

PrintTransport

Copyright (c) 2004 Haiku.

Authors:
	Michael Pfeiffer
	Philippe Houdoin
	
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

#ifndef _PRINT_TRANSPORT_H
#define _PRINT_TRANSPORT_H

#include <image.h>
#include <DataIO.h>
#include <Node.h>
#include <Message.h>

// The class PrintTransport is intended to be used by
// a printer driver.
class PrintTransport
{
public:
	PrintTransport();
	~PrintTransport();

	// opens the transport add-on associated with the printerFolder
	status_t Open(BNode* printerFolder);
	// returns the output stream created by the transport add-on
	BDataIO* GetDataIO();
	// returns false if the user has canceled the save to file dialog
	// of the "Print To File" transport add-on.
	bool IsPrintToFileCanceled() const;
	
private:	
	BDataIO*   fDataIO;
	image_id   fAddOnID;
	void       (*fExitProc)(void);		
};

#endif // PRINT_TRANSPORT_H
