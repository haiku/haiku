/*

MessagePrinter.cpp

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

#include <SupportKit.h>
#include <Alert.h> 

#include "MessagePrinter.h"

/**
 * Print() 
 *
 * @param BMessage*
 * @return status_t, any result from the operation
 */
status_t MessagePrinter::Print(BMessage* msg)
{
	status_t status;
	BFile file;
	BPath settingsPath;
	// char settingsPath[B_PATH_NAME_LENGTH];
	char  *name; 
	uint32  type; 
	int32   count; 

	// open a file to print message on the desktop
	status = find_directory(B_DESKTOP_DIRECTORY, &settingsPath);
	if (status != B_OK) {
		BAlert* alert = new BAlert("","Find directory error", "Doh!");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return status;
	}

	settingsPath.Append(msgFileName);
	status = file.SetTo(settingsPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (status != B_OK) {
		BAlert* alert = new BAlert("","File write error", "Doh!");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return status;
	}	
	
	// print the contents of the message to file
	for ( int32 i = 0; 
    	msg->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; 
     	i++ ) 
	{ 
		BString out;
		// count
		out << i;
		if (file.Write(out.String(), out.Length()) < 0) {
			BAlert* alert = new BAlert("","Count write error", "Doh!");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return B_ERROR;
		}	
		// name
		out = " ";
		out << name;
		if (file.Write(out.String(), out.Length()) < 0) {
			BAlert* alert = new BAlert("","Name write error", "Doh!");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return B_ERROR;
		}	
		
		// type
		switch (type) {
			case B_INT32_TYPE:
				{
				int32 v;
				out = " B_INT32_TYPE ";
				msg->FindInt32(name, 0, &v);
				out << v << '\n';
				}
				break;
			case B_UINT32_TYPE:
				{
				int32 v;
				out = " B_UINT32_TYPE ";
				msg->FindInt32(name, 0, &v);
				out << v << '\n';
				}
				break;
			case B_INT64_TYPE:
				{
				int64 v;
				out = " B_INT64_TYPE ";
				msg->FindInt64(name, 0, &v);
				out << v << '\n';
				}
				break;
			case B_FLOAT_TYPE:
				{
				float f;
				out = " B_FLOAT_TYPE ";
				msg->FindFloat(name, 0, &f);
				out << f << '\n';
				}
				break;
			case B_BOOL_TYPE:
				{
				bool b;
				out = " B_BOOL_TYPE ";
				msg->FindBool(name, 0, &b);
				out << (int)b << '\n';
				}
				break;
			case B_MESSAGE_TYPE:
				{
				BMessage *m = new BMessage(); 
				MessagePrinter *mp = new MessagePrinter(name);
				msg->FindMessage(name, m);
				mp->Print(m);
				out = " B_MESSAGE_TYPE \n";
				delete m;
				delete mp;
				}
				break;
			case B_RECT_TYPE:
				{
				BRect r;
				out = " B_RECT_TYPE ";
				msg->FindRect(name, 0, &r);
				out << "{ " 
					<< r.left <<" " 
					<< r.top <<" " 
					<< r.right <<" " 
					<< r.bottom <<" " 
					<< " }"
					<< '\n';
				}
				break;
			case B_STRING_TYPE:
				{
				BString s;
				out = " B_STRING_TYPE ";
				msg->FindString(name, 0, &s);
				out << s.String() << '\n';
				}
				break;
			default:
				out = " UNKNOWN TYPE \n";
				break;
		}
	    if (file.Write(out.String(), out.Length()) < 0) {
			BAlert* alert = new BAlert("","Value write error", "Doh!");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return B_ERROR;
		}	
	}
	return status;
}

