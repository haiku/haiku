#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <BeBuild.h>
#include <GLView.h>

class BGLDispatcher;

class BGLRenderer
{

	// Private unimplemented copy constructors
						BGLRenderer(const BGLRenderer &);
						BGLRenderer & operator=(const BGLRenderer &);
	
public:
						BGLRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher);
	virtual				~BGLRenderer();

	void 				Acquire();
	void	 			Release();

	void 				LockGL();
	void 				UnlockGL();
	
	virtual	void 		SwapBuffers(bool VSync = false) const;
	virtual	void		Draw(BRect updateRect);
	virtual status_t    CopyPixelsOut(BPoint source, BBitmap *dest);
	virtual status_t    CopyPixelsIn(BBitmap *source, BPoint dest);


	virtual void		AttachedToWindow();
	virtual void		AllAttached();
	virtual void		DetachedFromWindow();
	virtual void		AllDetached();

 	virtual void    	FrameResized(float width, float height);
	
	void				DirectConnected( direct_buffer_info *info );
	void				EnableDirectMode( bool enabled );

	inline	int32			ReferenceCount()	{ return fRefCount; };			
	inline	ulong			Options() 			{ return fOptions; };
	inline	BGLView *		GLView()			{ return fView; };
	inline	BGLDispatcher *	GLDispatcher()		{ return fDispatcher; };

private:
	volatile int32		fRefCount;		// How much we're still usefull?
	BGLView *			fView;			// Never forget who is the boss!
	ulong				fOptions;		// Keep that tune in memory
	BGLDispatcher *		fDispatcher;	// Our personal OpenGL API call dispatcher

};

extern "C" _EXPORT BGLRenderer * instanciate_gl_renderer(BGLView *view, BGLDispatcher *dispatcher);


#endif	// GLRENDERER_H





