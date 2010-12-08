/*
 * Copyright 2010 Oliver Ruiz Dorantes
 * Copyright 2010 Dan-Matei Epure, mateiepure@gmail.com
 * Copyright BeNet Team (Original Project)
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _Output_h
#define _Output_h

#include <Window.h>
#include <TextView.h>
#include <ScrollBar.h>
#include <Button.h>
#include <TabView.h>

const uint32 kMsgOutputReset	= 'outr';
const uint32 kMsgOutputResetAll	= 'opra';


class OutputView : public BView
{
public:
	OutputView(BRect frame);
	virtual void	FrameResized(float width, float height);

	void			Add(const char* text)	{fTextView->Insert(text);}
	void			Clear()					{fTextView->Delete(0, fTextView->TextLength());}

private:
	BTextView*		fTextView;
	BScrollBar*		fScrollBar;
};


class Output : public BWindow
{
public:
	static Output*	Instance();
	
	~Output();
	
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage* msg);
	virtual void	FrameMoved(BPoint point);

	void			AddTab(const char* text, int32 index);

	void            Post(const char* text, uint32 index);
	int				Postf(uint32 index, const char* format, ...);

private: 
	// functions
	Output();
	void			Add(const char* text, OutputView* view);

private: 
	// data
	static Output*	sInstance;
	BTab*			fAllTab;

	OutputView*		fAll;

	BButton*		fReset;
	BButton*		fResetAll;
	
	BTabView* 		fTabView;
	
	BList*			fTabsList;
	BList*			fOutputViewsList;
	BRect			fBounds; 
						// Bounds for tabs
};


#endif // _Output_h
