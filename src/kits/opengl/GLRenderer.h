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
						BGLRenderer(BGLView *view, BGLDispatcher *dispatcher);
	virtual				~BGLRenderer();

	status_t 			Acquire();
	status_t 			Release();

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

private:
	uint32	fRefCount;
};

extern "C" _EXPORT BGLRenderer * instanciate_gl_renderer(BGLView *view, BGLDispatcher *dispatcher);


#endif	// GLRENDERER_H





