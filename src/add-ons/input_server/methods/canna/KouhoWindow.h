//
//	KouhoWindow.h
//
//	This is a part of...
//	CannaIM
//	version 1.0
//	(c) 1999 M.Kawamura
//

#ifndef KOUHOWINDOW_H
#define KOUHOWINDOW_H

#include <Window.h>
#include <TextView.h>
#include <Region.h>
#include <Box.h>
#include <View.h>
#include <StringView.h>

#include "CannaCommon.h"
#include "CannaLooper.h"

#define DUMMY_RECT	BRect(0,0,150,30)
#define INDEXVIEW_SIDE_MARGIN	2
#define KOUHOVIEW_SIDE_MARGIN	2
#define WINDOW_BORDER_WIDTH_H	22
#define WINDOW_BORDER_WIDTH_V	5
#define INFOVIEW_HEIGHT			14

extern Preferences gSettings;

class KouhoView : public BTextView
{
private:
	BRect			highlightRect;
//	BView*			theView;
	rgb_color		selection_color;
public:
					KouhoView( BRect rect );
	void			HighlightLine( int32 line );
	void			HighlightPartial( int32 begin, int32 end );
	virtual void	Draw( BRect rect );
//	void			SetTargetITView( BView *itView )
//					{ theView = itView; };
	virtual void	MouseDown( BPoint point );
};

class KouhoIndexView : public BBox
{
private:
	float				lineHeight;
	int32				fontOffset; //for vertical centering
	bool				hideNumber;
public:
						KouhoIndexView( BRect rect, float height );
	virtual void		Draw( BRect rect );
	void				HideNumberDisplay( bool hide )
						{ hideNumber = hide; Invalidate(); };
	bool				IsNumberDisplayHidden()
						{ return hideNumber; };
};

class KouhoInfoView : public BStringView
{
public:
						KouhoInfoView( BRect rect );
	virtual void		Draw( BRect rect );
};

class KouhoWindow : public BWindow
{
private:
	BLooper*			cannaLooper;
	KouhoView*			kouhoView;
	KouhoIndexView*		indexView;
	KouhoInfoView*		infoView;
	BFont*				kouhoFont;
	float				indexWidth; //width of index pane
	float				minimumWidth;
	void				ShowAt( BPoint revpoint, float height );
	bool				standalone_mode;
	void				ShowWindow(); //beta 2 fix
public:
						KouhoWindow( BFont *font, BLooper *looper );
	virtual void		MessageReceived( BMessage *msg );
	virtual void		FrameMoved( BPoint screenPoint );
};

#endif
