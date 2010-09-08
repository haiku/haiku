/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BGLVIEW_H
#define BGLVIEW_H

#include <GL/gl.h>

#define BGL_RGB			0
#define BGL_INDEX		1
#define BGL_SINGLE		0
#define BGL_DOUBLE		2
#define BGL_DIRECT		0
#define BGL_INDIRECT	4
#define BGL_ACCUM		8
#define BGL_ALPHA		16
#define BGL_DEPTH		32
#define BGL_OVERLAY		64
#define BGL_UNDERLAY	128
#define BGL_STENCIL		512

#ifdef __cplusplus

#include <AppKit.h>
#include <Bitmap.h>
#include <DirectWindow.h>
#include <View.h>
#include <Window.h>
#include <WindowScreen.h>

struct glview_direct_info;
class BGLRenderer;
class GLRendererRoster;

class BGLView : public BView {
public:
					BGLView(BRect rect, const char* name,
						ulong resizingMode, ulong mode,
						ulong options);
	virtual			~BGLView();

			void	LockGL();
			void	UnlockGL();
			void	SwapBuffers();
			void	SwapBuffers(bool vSync);

			BView*	EmbeddedView(); // deprecated, returns NULL
			void* 	GetGLProcAddress(const char* procName);

		status_t	CopyPixelsOut(BPoint source, BBitmap *dest);
		status_t	CopyPixelsIn(BBitmap *source, BPoint dest);

	// Mesa's GLenum is uint where Be's ones was ulong!
	virtual	void	ErrorCallback(unsigned long errorCode);

	virtual void	Draw(BRect updateRect);
	virtual void	AttachedToWindow();
	virtual void	AllAttached();
	virtual void	DetachedFromWindow();
	virtual void	AllDetached();

	virtual void	FrameResized(float newWidth, float newHeight);
virtual status_t	Perform(perform_code d, void *arg);

virtual status_t	Archive(BMessage *data, bool deep = true) const;

	virtual void	MessageReceived(BMessage *message);
	virtual void	SetResizingMode(uint32 mode);

	virtual void	Show();
	virtual void	Hide();

virtual BHandler*	ResolveSpecifier(BMessage *msg, int32 index,
								BMessage *specifier, int32 form,
								const char *property);
virtual status_t    GetSupportedSuites(BMessage *data);

			void	DirectConnected(direct_buffer_info *info);
			void	EnableDirectMode(bool enabled);

			void*	getGC()	{ return fGc; } // ???

	virtual void	GetPreferredSize(float* width, float* height);


private:

	virtual void	_ReservedGLView1();
	virtual void	_ReservedGLView2();
	virtual void	_ReservedGLView3();
	virtual void	_ReservedGLView4();
	virtual void	_ReservedGLView5();
	virtual void	_ReservedGLView6();
	virtual void	_ReservedGLView7();
	virtual void	_ReservedGLView8();

					BGLView(const BGLView &);
					BGLView &operator=(const BGLView &);

		void		_DitherFront();
		bool		_ConfirmDither();
		void		_Draw(BRect rect);
		void		_CallDirectConnected();

		void *		fGc;
		uint32		fOptions;
		uint32      fDitherCount;
		BLocker		fDrawLock;
		BLocker     fDisplayLock;
		glview_direct_info *		fClipInfo;

		BGLRenderer *fRenderer;
		GLRendererRoster *fRoster;

		BBitmap *   fDitherMap;
		BRect       fBounds;
		int16 *     fErrorBuffer[2];
		uint64      _reserved[8];

		void	_LockDraw();
		void	_UnlockDraw();

// BeOS compatibility
private:
					BGLView(BRect rect, char* name,
						ulong resizingMode, ulong mode,
						ulong options);
};


class BGLScreen : public BWindowScreen {
public:
				BGLScreen(char* name,
					ulong screenMode, ulong options,
					status_t *error, bool debug=false);
				~BGLScreen();

			void	LockGL();
			void	UnlockGL();
			void	SwapBuffers();
	// Mesa's GLenum is uint where Be's ones was ulong!
	virtual	void	ErrorCallback(unsigned long errorCode);

	virtual void	ScreenConnected(bool connected);
	virtual void	FrameResized(float width, float height);
virtual status_t	Perform(perform_code code, void *arg);

virtual status_t	Archive(BMessage *data, bool deep = true) const;
	virtual void	MessageReceived(BMessage *message);

	virtual void	Show();
	virtual void	Hide();

virtual BHandler*	ResolveSpecifier(BMessage *message,
								int32 index,
								BMessage *specifier,
								int32 form,
								const char *property);
virtual status_t	GetSupportedSuites(BMessage *data);

private:

	virtual void	_ReservedGLScreen1();
	virtual void	_ReservedGLScreen2();
	virtual void	_ReservedGLScreen3();
	virtual void	_ReservedGLScreen4();
	virtual void	_ReservedGLScreen5();
	virtual void	_ReservedGLScreen6();
	virtual void	_ReservedGLScreen7();
	virtual void	_ReservedGLScreen8();

					BGLScreen(const BGLScreen &);
					BGLScreen	&operator=(const BGLScreen &);

		void *		fGc;
		long		fOptions;
		BLocker		fDrawLock;

		int32		fColorSpace;
		uint32		fScreenMode;

		uint64      _reserved[7];
};

#endif // __cplusplus

#endif // BGLVIEW_H
