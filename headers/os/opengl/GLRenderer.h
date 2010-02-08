/*
 * Copyright 2006, Philippe Houdoin. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <BeBuild.h>
#include <GLView.h>

class BGLDispatcher;
class GLRendererRoster;

typedef unsigned long renderer_id;

class BGLRenderer
{

	// Private unimplemented copy constructors
				BGLRenderer(const BGLRenderer &);
				BGLRenderer & operator=(const BGLRenderer &);
	
public:
				BGLRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher);
	virtual			~BGLRenderer();

	void 			Acquire();
	void			Release();

	virtual void		LockGL();
	virtual void 		UnlockGL();
	
	virtual	void 		SwapBuffers(bool VSync = false);
	virtual	void		Draw(BRect updateRect);
	virtual status_t    	CopyPixelsOut(BPoint source, BBitmap *dest);
	virtual status_t    	CopyPixelsIn(BBitmap *source, BPoint dest);

 	virtual void    	FrameResized(float width, float height);
	
	virtual void		DirectConnected(direct_buffer_info *info);
	virtual void		EnableDirectMode(bool enabled);

	inline	int32		ReferenceCount() const	{ return fRefCount; };
	inline	ulong		Options() const		{ return fOptions; };
	inline	BGLView *	GLView()		{ return fView; };
	inline	BGLDispatcher *	GLDispatcher()		{ return fDispatcher; };

private:
	friend class GLRendererRoster;

	virtual status_t	_Reserved_Renderer_0(int32, void *);
	virtual status_t	_Reserved_Renderer_1(int32, void *);
	virtual status_t	_Reserved_Renderer_2(int32, void *);
	virtual status_t	_Reserved_Renderer_3(int32, void *);
	virtual status_t	_Reserved_Renderer_4(int32, void *);

	volatile int32		fRefCount;	// How much we're still usefull?
	BGLView*		fView;		// Never forget who is the boss!
	ulong			fOptions;	// Keep that tune in memory
	BGLDispatcher*		fDispatcher;	// Our personal OpenGL API call dispatcher

	GLRendererRoster*	fOwningRoster;
	renderer_id		fID;
};

extern "C" _EXPORT BGLRenderer * instantiate_gl_renderer(BGLView *view, ulong options, BGLDispatcher *dispatcher);


#endif	// GLRENDERER_H

