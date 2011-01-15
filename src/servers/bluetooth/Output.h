/*
 * Copyright 2011 Hamish Morrison, hamish@lavabit.com
 * Copyright 2010 Oliver Ruiz Dorantes
 * Copyright 2010 Dan-Matei Epure, mateiepure@gmail.com
 * Copyright BeNet Team (Original Project)
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _Output_h
#define _Output_h

#include <Button.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TabView.h>
#include <TextView.h>
#include <Window.h>

const uint32 kMsgOutputReset	= 'outr';
const uint32 kMsgOutputResetAll	= 'opra';


class OutputView : public BGroupView
{
public:
	OutputView(const char* name);

	void			Add(const char* text)	{fTextView->Insert(text);}
	void			Clear()					{fTextView->Delete(0, fTextView->TextLength());}

private:
	BTextView*		fTextView;
};


class Output : public BWindow
{
public:
	static Output*	Instance();
	
	~Output();
	
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage* msg);
	virtual void	FrameMoved(BPoint point);

	void			AddTab(const char* text, uint32 index);

	void            Post(const char* text, uint32 index);
	int				Postf(uint32 index, const char* format, ...);

private: 
	// functions
	Output();
	void			_Add(const char* text, OutputView* view);
	OutputView*		_OutputViewForTab(int32 index);

private: 
	// data
	static Output*	sInstance;
	BTab*			fAllTab;

	OutputView*		fAll;

	BButton*		fReset;
	BButton*		fResetAll;
	
	BTabView* 		fTabView;
	
	BList*			fOutputViewsList;
};


#endif // _Output_h
