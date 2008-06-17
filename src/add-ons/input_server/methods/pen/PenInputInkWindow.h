/*
	Copyright 2005, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>
#include <Shape.h>

class PenInputInkView;

class PenInputInkWindow : public BWindow
{
public:
	PenInputInkWindow(BRect frame);
	virtual ~PenInputInkWindow();
	void			MessageReceived(BMessage *message);
	
private:
	friend class PenInputInkView;
	friend class PenInputLooper;//DEBUG
	void ClearStrokes();
	PenInputInkView *fInkView;
	BShape fStrokes;
	int32 fStrokeCount;
};

