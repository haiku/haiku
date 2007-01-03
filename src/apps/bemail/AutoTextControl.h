/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef AUTOTEXT_H
#define AUTOTEXT_H

#include <TextControl.h>
#include <MessageFilter.h>
#include <Messenger.h>

class AutoTextControlFilter;

enum
{
	M_PREVIOUS_FIELD='mprf',
	M_NEXT_FIELD='mnxf',
	M_ENTER_NAVIGATION='ennv'
};

class AutoTextControl : public BTextControl
{
public:
	AutoTextControl(const BRect &frame, const char *name, const char *label,
			const char *text, BMessage *msg,
			uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual ~AutoTextControl(void);
	virtual void AttachedToWindow(void);
	virtual void DetachedFromWindow(void);
	void SetFilter(AutoTextControlFilter *filter);
	AutoTextControlFilter *GetFilter(void) { return fFilter; }
	
	void SetCharacterLimit(const uint32 &limit);
	uint32 GetCharacterLimit(const uint32 &limit);
	
	void SetEscapeCancel(const bool &value) { fEscapeCancel = value; }
	bool IsEscapeCancel(void) const { return fEscapeCancel; }
private:
	friend AutoTextControlFilter;
	AutoTextControlFilter *fFilter;
	bool fEscapeCancel;
	uint32 fCharLimit;
};

class AutoTextControlFilter : public BMessageFilter
{
public:
	AutoTextControlFilter(AutoTextControl *checkview);
	~AutoTextControlFilter(void);
	virtual filter_result Filter(BMessage *msg, BHandler **target);
	virtual filter_result KeyFilter(const int32 &key, const int32 &mod);
	
	AutoTextControl *TextControl(void) const { return fBox; }
	
	void SetMessenger(BMessenger *msgr);
	BMessenger *GetMessenger(void) const { return fMessenger; }
	void SendMessage(BMessage *msg);
	BMessage *GetCurrentMessage(void) { return fCurrentMessage; }

protected:
	bool IsEscapeCancel(void) const { return fBox->IsEscapeCancel(); }
	
private:
	void HandleNoNumLock(const int32 &code, int32 &rawchar, BMessage *msg);
	
	AutoTextControl *fBox;
	BMessenger *fMessenger;
	BMessage *fCurrentMessage;
};

#endif
