/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef MAGNIFY_H
#define MAGNIFY_H


#include <Application.h>
#include <Box.h>
#include <FilePanel.h>
#include <MenuBar.h>
#include <View.h>
#include <Window.h>


class TMagnify;
class TWindow;

class TOSMagnify : public BView {
	public:
						TOSMagnify(BRect, TMagnify* parent, color_space space);
		virtual			~TOSMagnify();

		void			InitObject();

		virtual void	FrameResized(float width, float height);
		void			SetSpace(color_space space);

		void			Resize(int32 width, int32 height);

		bool			CreateImage(BPoint, bool force=false);
		bool			CopyScreenRect(BRect);

		void			DrawGrid(int32 width, int32 height,
							BRect dest, int32 pixelSize);
		void			DrawSelection();

		rgb_color		ColorAtSelection();

		BBitmap*		Bitmap() { return fBitmap; }

	private:
		color_space		fColorSpace;
		char*			fOldBits;
		long			fBytesPerPixel;

		TMagnify*		fParent;
		BBitmap*		fBitmap;
		BBitmap*		fPixel;
		BView*			fPixelView;
};

class TMagnify : public BView {
	public:
						TMagnify(BRect, TWindow*);
		virtual			~TMagnify();

		void			InitBuffers(int32 hPixelCount, int32 vPixelCount,
							int32 pixelSize, bool showGrid);

		virtual void	AttachedToWindow();
		virtual void	Draw(BRect);

		virtual void	KeyDown(const char *bytes, int32 numBytes);
		virtual void	FrameResized(float, float);
		virtual void	MouseDown(BPoint where);
		virtual void	ScreenChanged(BRect bounds, color_space cs);
		virtual void	WindowActivated(bool);

		void			SetSelection(bool state);
		void			MoveSelection(int32 x, int32 y);
		void			MoveSelectionTo(int32 x, int32 y);
		void			ShowSelection();

		short 			Selection();
		bool			SelectionIsShowing();
		void			SelectionLoc(float* x, float* y);
		void			SetSelectionLoc(float, float);
		rgb_color		SelectionColor();

		void			CrossHair1Loc(float* x, float* y);
		void			CrossHair2Loc(float* x, float* y);
		BPoint			CrossHair1Loc();
		BPoint			CrossHair2Loc();

		void			NudgeMouse(float x, float y);

		void			Update(bool force);
		bool			NeedToUpdate();
		void			SetUpdate(bool);

		void			CopyImage();

		long			ThreadID() { return fThread; }

		void			MakeActive(bool);
		bool			Active() { return fActive; }

		void			MakeSticked(bool);
		bool			Sticked() const { return fStickCoordinates; }

		void			AddCrossHair();
		void			RemoveCrossHair();
		void			SetCrossHairsShowing(bool ch1=false, bool ch2=false);
		void			CrossHairsShowing(bool*, bool*);

		void			PixelCount(int32* width, int32* height);
		int32 			PixelSize();
		bool			ShowGrid();

		void			StartSave();
		void			SaveImage(entry_ref* ref, char* name);
		void			EndSave();

	private:
		static status_t	MagnifyTask(void *);

		bool			fNeedToUpdate;
		long			fThread;				//	magnify thread id
		bool			fActive;				//	magnifying toggle

		BBitmap*		fImageBuf;				// os buffer
		TOSMagnify*		fImageView;				// os view
		BPoint			fLastLoc;

		short			fSelection;

		bool			fShowSelection;
		BPoint			fSelectionLoc;

		bool			fShowCrossHair1;
		BPoint			fCrossHair1;
		bool			fShowCrossHair2;
		BPoint			fCrossHair2;

		TWindow*		fParent;

		bool			fImageFrozenOnSave;
		bool			fStickCoordinates;
};

class TMenu : public BMenu {
	public:
						TMenu(TWindow* mainWindow, const char *title = NULL,
							menu_layout layout = B_ITEMS_IN_COLUMN);
		virtual			~TMenu();

		virtual void	AttachedToWindow();

	private:
		TWindow*		fMainWindow;
};

class TInfoView : public BBox {
	public:
						TInfoView(BRect frame);
		virtual			~TInfoView();

		virtual void	AttachedToWindow();
		virtual void	Draw(BRect updateRect);
		virtual void	FrameResized(float width, float height);
		virtual void	GetPreferredSize(float* _width, float* _height);

		void			AddMenu();
		void			SetMagView(TMagnify* magView);
		void			SetInfoTextVisible(bool visible);
		bool			IsInfoTextVisible();

	private:
		float	 		fFontHeight;
		TMagnify*		fMagView;
		BMenuField*	 	fPopUp;
		TMenu*			fMenu;

		int32 			fHPixelCount;
		int32 			fVPixelCount;
		int32			fPixelSize;

		rgb_color		fSelectionColor;

		BPoint			fCH1Loc;
		BPoint			fCH2Loc;

		char			fInfoStr[64];
		char			fRGBStr[64];
		char			fCH1Str[64];
		char			fCH2Str[64];

		bool			fInfoTextVisible;
};

class TWindow : public BWindow {
	public:
						TWindow(int32 pixelCount = -1);
		virtual			~TWindow();

		virtual void	MessageReceived(BMessage* message);
		virtual bool	QuitRequested();

		void			GetPrefs(int32 pixelCount = -1);
		void			SetPrefs();

		virtual void	FrameResized(float width, float height);
		virtual void	ScreenChanged(BRect screenSize, color_space depth);

		virtual void	Minimize(bool);
		virtual void	Zoom(BPoint position, float width, float height);

		void			CalcViewablePixels();
		void			GetPreferredSize(float* width, float* height);

		void			ResizeWindow(int32 rowCount, int32 columnCount);
		void			ResizeWindow(bool direction);

		void			SetGrid(bool);
		bool			ShowGrid();

		void			ShowInfo(bool);
		bool			InfoIsShowing();
		bool			InfoBarIsVisible();
		void			UpdateInfo();
		void			UpdateInfoBarOnResize();

		void			AddCrossHair();
		void			RemoveCrossHair();
		void			CrossHairsShowing(bool* ch1, bool* ch2);

		void			PixelCount(int32* h, int32 *v);

		void			SetPixelSize(int32);
		void			SetPixelSize(bool);
		int32			PixelSize();

		bool			IsActive();
		bool			IsSticked();

	private:
		float			fInfoHeight;
		bool			fShowInfo;
		float 			fFontHeight;

		bool			fShowGrid;
		bool			fInfoBarState;

		int32			fHPixelCount;
		int32			fVPixelCount;
		int32	 		fPixelSize;

		TMagnify*		fFatBits;
		TInfoView*		fInfo;

		BFilePanel*		fSavePanel;
};

class TApp : public BApplication {
	public:
						TApp(int32 pixelCount = -1);
};

#endif	// MAGNIFY_H
