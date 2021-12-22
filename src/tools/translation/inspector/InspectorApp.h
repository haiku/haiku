/*****************************************************************************/
// InspectorApp
// Written by Michael Wilber, Haiku Translation Kit Team
//
// InspectorApp.h
//
// BApplication object for the Inspector application.  The purpose of
// Inspector is to provide the user with as much relevant information as
// possible about the currently open document.
//
//
// Copyright (c) 2003 Haiku Project
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

#ifndef INSPECTORAPP_H
#define INSPECTORAPP_H

#include "ActiveTranslatorsWindow.h"
#include "InfoWindow.h"
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <List.h>
#include <String.h>


class InspectorApp : public BApplication {
public:
	InspectorApp();
	void MessageReceived(BMessage *pmsg);
	void RefsReceived(BMessage *pmsg);

	BList *GetTranslatorsList();

private:
	void AddToTranslatorsList(const char *folder, int32 group);

	BString fbstrInfo;
	BList flstTranslators;
	ActiveTranslatorsWindow *fpActivesWin;
	InfoWindow *fpInfoWin;
};

#endif // #ifndef INSPECTORAPP_H
