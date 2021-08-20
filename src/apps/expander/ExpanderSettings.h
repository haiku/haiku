/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderSettings.h
//
//
// Copyright (c) 2004 Haiku Project
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
#ifndef _EXPANDER_SETTINGS_H
#define _EXPANDER_SETTINGS_H


#include <Message.h>
#include <File.h>


static const uint32 kMsgExpanderSettings = 'Exst';


class BPath;

class ExpanderSettings {
public:
							ExpanderSettings();
							~ExpanderSettings();

			const			BMessage& Message() const { return fMessage; }
			void			UpdateFrom(BMessage* message);

	static	status_t		GetSettingsDirectoryPath(BPath& _path);

private:
			status_t		Open(BFile* file, int32 mode);

			BMessage		fMessage;
			bool			fUpdated;
};


#endif // _EXPANDER_SETTINGS_H
