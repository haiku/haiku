/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//--------------------------------------------------------------------
//	
//	Signature.h
//
//--------------------------------------------------------------------

#ifndef _SIGNATURE_H
#define _SIGNATURE_H

#include <Alert.h>
#include <Beep.h>
#include <Box.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_index.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TextControl.h>
#include <Window.h>

const float kSigHeight = 200;
const float kSigWidth = 457;


#define INDEX_SIGNATURE		"_signature"


class TMenu;
class TNameControl;
class TScrollView;
class TSignatureView;
class TSigTextView;

//====================================================================

class TSignatureWindow : public BWindow {
public:
	TSignatureWindow(BRect);
	~TSignatureWindow();
	virtual void MenusBeginning();
	virtual void MessageReceived(BMessage*);
	virtual bool QuitRequested();
	virtual void Show();
	void FrameResized(float width, float height);
	bool Clear();
	bool IsDirty();
	void Save();

private:
	BMenuItem *fCut;
	BMenuItem *fCopy;
	BMenuItem *fDelete;
	BMenuItem *fNew;
	BMenuItem *fPaste;
	BMenuItem *fSave;
	BMenuItem *fUndo;
	BEntry fEntry;
	BFile *fFile;
	TMenu *fSignature;
	TSignatureView *fSigView;
};

//--------------------------------------------------------------------

class TSignatureView : public BBox {
public:
	TSignatureView(BRect); 
	virtual	void AttachedToWindow();

	TNameControl *fName;
	TSigTextView *fTextView;

private:
	float fOffset;
};

//====================================================================

class TNameControl : public BTextControl {
public:
	TNameControl(BRect, const char*, BMessage*);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage*);

private:
	char fLabel[100];
};

//====================================================================

class TSigTextView : public BTextView {
public:
	TSigTextView(BRect, BRect); 
	void FrameResized(float width, float height);

	virtual void DeleteText(int32, int32);
	virtual void KeyDown(const char*, int32);
	virtual void InsertText(const char*, int32, int32, const text_run_array*);
	virtual void MessageReceived(BMessage*);

	bool fDirty;

private:
	TSignatureView *fParent;
};

#endif // #ifndef _SIGNATURE_H

