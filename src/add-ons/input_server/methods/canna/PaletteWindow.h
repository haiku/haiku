//
//	PaletteWindow.h
//
//	This is a part of...
//	CannaIM
//	version 1.0
//	(c) 1999 M.Kawamura
//

#ifndef PALETTEWINDOW_H
#define PALETTEWINDOW_H

#include <Looper.h>
#include <Window.h>
#include <PictureButton.h>
#include <Box.h>

extern Preferences gSettings;

class PaletteWindow : public BWindow
{
private:
	BLooper*			cannaLooper;
	BBox*				back;
	BPictureButton*		HiraButton;
	BPictureButton*		KataButton;
	BPictureButton*		ZenAlphaButton;
	BPictureButton*		HanAlphaButton;
	BPictureButton*		ExtendButton;
	BPictureButton*		KigoButton;
	BPictureButton*		HexButton;
	BPictureButton*		BushuButton;
	BPictureButton*		TorokuButton;
	void				AllButtonOff();
public:
	PaletteWindow( BRect rect, BLooper* looper );
	virtual void	MessageReceived( BMessage *msg );
	virtual void	FrameMoved( BPoint screenPoint );
};

#endif
