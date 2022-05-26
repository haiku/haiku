/*****************************************************************************/
// Print to file transport add-on.
//
// Author
//   Philippe Houdoin
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001,2002 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <InterfaceKit.h>
#include <StorageKit.h>

class FileSelector : public BWindow {
	public:
		// Constructors, destructors, operators...

								FileSelector(void);
								~FileSelector(void);

		typedef BWindow 		inherited;

		// public constantes
		enum	{
			START_MSG			= 'strt',
			SAVE_INTO_MSG		= 'save'
		};
				
		// Virtual function overrides
	public:	
		virtual void 			MessageReceived(BMessage * msg);
		virtual bool 			QuitRequested();
		status_t 				Go(entry_ref * ref);

		// From here, it's none of your business! ;-)
	private:
		BEntry					fEntry;
		volatile status_t 		fResult;
		sem_id 					fExitSem;
		BFilePanel *			fSavePanel;
};

#endif // FILESELECTOR_H

