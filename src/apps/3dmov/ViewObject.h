/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
					
					This is the base class for all 3Dmov objects.
					As a bare minimum, the derived classes need to implement the Render() method.
*/

#ifndef _VIEW_OBJECT_H_
#define _VIEW_OBJECT_H_

#include <GLView.h>

class BBitmap;
class Video;
class ViewObject;

/**********************************
	MediaSource can be an image or video.
	A ViewObject can contain multiple media sources (eg. a cube has 6)
***********************************/
class MediaSource
{
public:
				MediaSource(ViewObject *owner);
				~MediaSource();
	void		SetVideo(Video *video);
				
	GLuint			mTextureID;			//	OpenGL handle to texture
	GLenum			mTextureFormat;		//	OpenGL texture format	
	int				mTextureWidth;		
	int				mTextureHeight;
	ViewObject		*mOwner;
	Video			*mVideo;			//	video decoder
};
	

/**********************************
	A ViewObject is the base class for various 3Dmov shapes.
	The shapes will typically override the Render method.
***********************************/
class ViewObject : public BGLView
{
public:
					ViewObject(BRect frame);
	virtual			~ViewObject();
	
	virtual void	AttachedToWindow(void);
	virtual void	FrameResized(float width, float height);
	virtual void	MouseDown(BPoint) {}
	virtual void	MouseMoved(BPoint p, uint32 transit, const BMessage *message = NULL) {}
	virtual void	MouseUp(BPoint) {}
	virtual void	Render(void);
	
	void			DragDrop(entry_ref *ref, float mouse_x, float mouse_y);
	void			UpdateFrame(MediaSource *source);	// called by video thread
	void			ToggleWireframe(bool enable);
	void			GLCheckError();
	
protected:
	virtual bool	SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y) {return false;}
	MediaSource		*GetDefaultMediaSource() {return sDefaultMediaSource;}
	
private:
	void			GLDetermineFormat(color_space cs, GLenum *format, int *num_bytes);
	void			GLCreateTexture(MediaSource *source, BBitmap *bitmap);
	
	static MediaSource	*sDefaultMediaSource;	// instructions image, shared to conserve resources
	static BBitmap*		sDefaultImage;

};

#endif	//#ifndef _VIEW_OBJECT_H_


