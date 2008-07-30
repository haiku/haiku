/*

PDF Writer printer driver.

Copyright (c) 2002 OpenBeOS. 

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
#ifndef DOCINFOWINDOW_H
#define DOCINFOWINDOW_H


#include "InterfaceUtils.h"


#include <String.h>


class BBox;
class BMenu;
class BScrollView;
class BTabView;
class BTextControl;
class BView;


#define HAVE_FULLVERSION_PDF_LIB 0


#if HAVE_FULLVERSION_PDF_LIB

class PermissionLabels {
public:
					PermissionLabels(const char* name, const char* pdfName)
						: fName(name), fPDFName(pdfName) { }

	const char*		GetName() const		{ return fName; }
	const char*		GetPDFName() const	{ return fName; }

private:
	const char*		fName;
	const char*		fPDFName;
};


class Permission {
public:
					Permission()
						: fLabels(NULL), fAllowed(true) { }

	const char*		GetName() const		{ return fLabels->GetName(); }
	const char*		GetPDFName() const	{ return fLabels->GetPDFName(); }
	bool			IsAllowed() const	{ return fAllowed; }

	void			SetLabels(const PermissionLabels* labels) { fLabels = labels; }
	void			SetAllowed(bool allowed) { fAllowed = allowed; }

private:
	const PermissionLabels*		fLabels;
	bool						fAllowed;
};

class Permissions {
public:
						Permissions();

	// accessors
	Permission*			At(int32 i)		{ return &fPermissions[i]; }
	int32				Length() const	{ return fNofPermissions; }

	// decode/encode pdflib permission string
	void				Decode(const char* s);
	void				Encode(BString& permissions);

private:
	Permission*			fPermissions;
	int32				fNofPermissions;
};

#endif	// HAVE_FULLVERSION_PDF_LIB


class DocInfoWindow : public HWindow 
{
	typedef HWindow				inherited;
public:
								DocInfoWindow(BMessage *doc_info);

	virtual void				Quit();
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage *message);
	virtual void				FrameResized(float newWidth, float newHeight);

	enum {
								OK_MSG			= 'ok__',
								CANCEL_MSG		= 'cncl',
								ADD_KEY_MSG		= 'add_',
								REMOVE_KEY_MSG	= 'rmov',
								DEFAULT_KEY_MSG	= 'dflt',
	};

private:
			BMessage*			_DocInfo() { return fDocInfo; }
			void				_SetupDocInfoView(BBox* panel);
			void				_BuildTable(const BMessage& fromDocInfo);
			void				_AdjustScrollBar(float controlHeight,
									float fieldsHeight);
			BTextControl*		_AddFieldToTable(BRect rect, const char* name,
									const char* value);
			void				_ReadFieldsFromTable(BMessage& docInfo);

			void				_RemoveKey(BMessage* msg);
			bool				_IsKeyValid(const char *key) const;
			void				_AddKey(BMessage* msg, bool textControl);

			void				_EmptyKeyList();

#if HAVE_FULLVERSION_PDF_LIB
			void				_SetupPasswordView(BBox* panel);
			void				_SetupPermissionsView(BBox* panel);
			BBox*				_CreateTabPanel(BTabView* tabView, const char* label);
			BTextControl*		_AddPasswordControl(BRect r, BView* panel,
									const char* name, const char* label);

			void				_ReadPasswords();
			void				_ReadPermissions();
#endif	// HAVE_FULLVERSION_PDF_LIB

private:
			BMessage*			fDocInfo; // owned by parent window
			BView*				fTable;
			BScrollView*		fTableScrollView;
			BMenu*				fKeyList;

#if HAVE_FULLVERSION_PDF_LIB
			BTextControl*		fMasterPassword;
			BTextControl*		fUserPassword;
			Permissions			fPermissions;
#endif	// HAVE_FULLVERSION_PDF_LIB
};

#endif
