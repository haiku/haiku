/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "AutoTextControl.h"
#include <Window.h>
#include <String.h>
#include <stdio.h>
#include <ctype.h>

AutoTextControlFilter::AutoTextControlFilter(AutoTextControl *box)
 : BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE,B_KEY_DOWN),
 	fBox(box),
 	fMessenger(NULL),
 	fCurrentMessage(NULL)
{
}

AutoTextControlFilter::~AutoTextControlFilter(void)
{
}

filter_result AutoTextControlFilter::Filter(BMessage *msg, BHandler **target)
{
	// This is really slick -- all that is needed to allow Escape key cancelling is
	// just calling SetEscapeCancel(true) for just *one* AutoTextControl in a window. *heh*
	int32 rawchar,mod;
	msg->FindInt32("raw_char",&rawchar);
	msg->FindInt32("modifiers",&mod);
	
	
	if(rawchar == B_ESCAPE)
	{
		if(mod & B_COMMAND_KEY)
			return B_DISPATCH_MESSAGE;
		
		if(IsEscapeCancel())
		{
			BLooper *loop = (*target)->Looper();
			if(loop)
			{
				BMessenger msgr(loop);
				msgr.SendMessage(B_QUIT_REQUESTED);
				return B_SKIP_MESSAGE;
			}
		}
	}
	
	BView *v=dynamic_cast<BView*>(*target);
	if(!v || strcmp("_input_",v->Name())!=0)
		return B_DISPATCH_MESSAGE;
	
	AutoTextControl *text = dynamic_cast<AutoTextControl*>(v->Parent());
	if(!text || text!=fBox)
		return B_DISPATCH_MESSAGE;
	
	// handle instances where numlock is off and the user tries to punch in numbers.
	// Instead of simply blocking the resulting keypresses, transform them into legit ones
	// and turn NumLock on for the user. This doesn't work on R5, so only do it on other
	// versions of BeOS
	#ifndef HAIKU_TARGET_PLATFORM_R5
	
	int32 scancode;
	if(msg->FindInt32("key",&scancode)!=B_OK)
		scancode = -1;
	
	HandleNoNumLock(scancode,rawchar,msg);
	
	#endif
	
	fCurrentMessage = msg;
	filter_result result = KeyFilter(rawchar,mod);
	fCurrentMessage = NULL;
	
	if(fBox->fCharLimit && result == B_DISPATCH_MESSAGE)
	{
		// See to it that we still allow shortcut keys
		if(mod & B_COMMAND_KEY)
			return B_DISPATCH_MESSAGE;
		
		// We don't use strlen() because it is not UTF-8 aware, which can affect
		// how many characters can be typed.
		if(isprint(rawchar) && (uint32)BString(text->Text()).CountChars()==text->fCharLimit)
			return B_SKIP_MESSAGE;
	}
	
	return result;
}

filter_result AutoTextControlFilter::KeyFilter(const int32 &rawchar, const int32 &mod)
{
	if(fMessenger)
		fMessenger->SendMessage(fBox->ModificationMessage());
	else
	if(fBox)
		fBox->Invoke();
	return B_DISPATCH_MESSAGE;
}

void AutoTextControlFilter::SetMessenger(BMessenger *msgr)
{
	if(fMessenger)
		delete fMessenger;
	fMessenger = msgr;
}

void AutoTextControlFilter::SendMessage(BMessage *msg)
{
	if(!msg)
		return;
	
	if(fMessenger)
		fMessenger->SendMessage(msg);
	else
		delete msg;
}

void AutoTextControlFilter::HandleNoNumLock(const int32 &code, int32 &rawchar,
											BMessage *msg)
{
	switch(code)
	{
		case 101:	// Numlock .
		{
			rawchar = '.';
			break;
		}
		case 100:	// Numlock 0
		{
			rawchar = '0';
			break;
		}
		case 88:
		{
			rawchar = '1';
			break;
		}
		case 89:
		{
			rawchar = '2';
			break;
		}
		case 90:
		{
			rawchar = '3';
			break;
		}
		case 72:
		{
			rawchar = '4';
			break;
		}
		case 74:
		{
			rawchar = '6';
			break;
		}
		case 55:
		{
			rawchar = '7';
			break;
		}
		case 56:
		{
			rawchar = '8';
			break;
		}
		case 57:
		{
			rawchar = '9';
			break;
		}
		default:
			return;
	}
	msg->ReplaceInt32("raw_char",rawchar);
	
	BString string;
	string << (char)rawchar;
	msg->ReplaceString("bytes",string);
	
	msg->ReplaceInt8("byte",(int8)rawchar);
	
	int32 mod;
	if(msg->FindInt32("modifiers",&mod)==B_OK)
	{
		uint32 mask = B_NUM_LOCK;
		if(mod & B_CAPS_LOCK)
			mask |= B_CAPS_LOCK;
		if(mod & B_SCROLL_LOCK)
			mask |= B_SCROLL_LOCK;
		
		set_keyboard_locks(mask);
	}
}


AutoTextControl::AutoTextControl(const BRect &frame, const char *name, const char *label,
		const char *text, BMessage *msg, uint32 resize,	uint32 flags)
 : BTextControl(frame,name,label,text,msg,resize,flags),
 	fFilter(NULL),
 	fEscapeCancel(false),
 	fCharLimit(0)
{
	SetFilter(new AutoTextControlFilter(this));
}

AutoTextControl::~AutoTextControl(void)
{
	if(Window())
		Window()->RemoveCommonFilter(fFilter);

	delete fFilter;
}

void AutoTextControl::AttachedToWindow(void)
{
	BTextControl::AttachedToWindow();
	if(fFilter)
		Window()->AddCommonFilter(fFilter);
}

void AutoTextControl::DetachedFromWindow(void)
{
	if(fFilter)
		Window()->RemoveCommonFilter(fFilter);
}

void AutoTextControl::SetCharacterLimit(const uint32 &limit)
{
	fCharLimit = limit;
}

uint32 AutoTextControl::GetCharacterLimit(const uint32 &limit)
{
	return fCharLimit;
}

void AutoTextControl::SetFilter(AutoTextControlFilter *filter)
{
	if(fFilter)
	{
		if(Window())
			Window()->RemoveCommonFilter(fFilter);
		delete fFilter;
	}
	
	fFilter = filter;
	if(Window())
		Window()->AddCommonFilter(fFilter);
}
