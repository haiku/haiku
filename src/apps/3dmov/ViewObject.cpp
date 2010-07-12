/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					This is the base class for all View shapes. 
*/

#include <stdio.h>
#include <assert.h>

#include <GLView.h>
#include <GL/glext.h>	
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Path.h>
#include "Video.h"

#include "GLUtility.h"
#include "ViewObject.h"

//	Local definitions

// 	Local functions

//	Local variables
MediaSource* 	ViewObject::sDefaultMediaSource = NULL;
BBitmap* 		ViewObject::sDefaultImage = NULL;

static int 		sCountViewObjects = 0;

/***********************************
	MediaSurface
************************************/

/*	FUNCTION:		MediaSource :: MediaSource
	ARGUMENTS:		owner
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MediaSource :: MediaSource(ViewObject *owner)
{
	mOwner = owner;
	mTextureID = 0;
	mVideo = 0;
}

/*	FUNCTION:		MediaSource :: ~MediaSource
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MediaSource :: ~MediaSource()
{
	if (mTextureID > 0)
		glDeleteTextures(1, &mTextureID);
	delete mVideo;
}

/*	FUNCTION:		MediaSurface :: SetVideo
	ARGUMENTS:		video
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void MediaSource :: SetVideo(Video *video)
{
	mVideo = video;
	mVideo->SetMediaSource(this);
}

/***********************************
	ViewObject
************************************/
/*	FUNCTION:		ViewObject :: ViewObject
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ViewObject :: ViewObject(BRect frame)
	: BGLView(frame, "3Dmov_view", B_FOLLOW_ALL_SIDES, 0, BGL_RGB | BGL_DOUBLE | BGL_DEPTH)
{
	sCountViewObjects++;
		
	FrameResized(frame.Width(), frame.Height());
}

/*	FUNCTION:		ViewObject :: ~ViewObject
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ViewObject :: ~ViewObject()
{ 
	if (--sCountViewObjects == 0)
	{
		delete sDefaultMediaSource;
		sDefaultMediaSource = 0;
		delete sDefaultImage;
	}
}

/*	FUNCTION:		ViewObject :: AttachedToWindow
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when view attached to window (looper)
*/
void ViewObject :: AttachedToWindow(void)
{
	LockGL();
	BGLView::AttachedToWindow();
	
	//	init OpenGL state
	glClearColor(0, 0, 0, 1);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LESS);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	//	init default texture (drag and drop media files)
	if (sDefaultMediaSource == 0)
	{		
		sDefaultImage = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "instructions.png");

		sDefaultMediaSource = new MediaSource(this);
		glGenTextures(1, &sDefaultMediaSource->mTextureID);
		glBindTexture(GL_TEXTURE_2D, sDefaultMediaSource->mTextureID);
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		int num_bytes;
		GLDetermineFormat(sDefaultImage->ColorSpace(), &sDefaultMediaSource->mTextureFormat, &num_bytes);
		glTexImage2D(GL_TEXTURE_2D,
				0, 										// mipmap level
				num_bytes,								// internal format
				sDefaultImage->Bounds().IntegerWidth() + 1,					// width
				sDefaultImage->Bounds().IntegerHeight() + 1,				// height
				0,										// border
				sDefaultMediaSource->mTextureFormat,	// format of pixel data
				GL_UNSIGNED_BYTE,						// data type
				(unsigned char *) sDefaultImage->Bits());		// pixels
	}
		
	MakeFocus(true);
	UnlockGL();
}	

/*	FUNCTION:		ViewObject :: FrameResized
	ARGUMENTS:		width
					height
	RETURN:			n/a
	DESCRIPTION:	Hook function called when view resized.
					Since we have a static camera, we configure it here.
					If we had a dynamic camera, then we'd have to reposition
					the camera in the derived view.	
*/
void ViewObject :: FrameResized(float width, float height)
{
	LockGL();
	BGLView::FrameResized(width, height);
	glViewport(0, 0, (int)width, (int)height);
	
	//	Camera projection
	glMatrixMode(GL_PROJECTION);
	float m[16];
	GLCreatePerspectiveMatrix(m, 30, width/height, 0.1f, 10);
	glLoadMatrixf(m);
	
	//	Camera position
	glMatrixMode(GL_MODELVIEW);
	GLCreateModelViewMatrix(m, 0, -0.45f, 0.4f, 0, -30);
	glLoadMatrixf(m);
	
	UnlockGL();
}

/*	FUNCTION:		ViewObject :: GLDetermineFormat
	ARGUMENTS:		cs				Haiku colour space
					format			Equivalent OpenGL format (see glPixelStore)
					num_bytes		number of bytes needed to store pixel at given format
	RETURN:			via argument pointers
	DESCRIPTION:	Haiku colour space is different to OpenGL colour space.
					This function will convert it.
*/
void ViewObject :: GLDetermineFormat(color_space cs, GLenum *format, int *num_bytes)
{
	switch (cs)
	{
		case B_RGBA32:	
			*num_bytes = 4;	
			*format = GL_BGRA;
			break;
		case B_RGB32:	
			*num_bytes = 4;	
			*format = GL_BGRA;
			break;
		case B_RGB24:	
			*num_bytes = 3;	
			*format = GL_BGR;
			break;
		case B_RGB15:
		case B_RGB16:
			*num_bytes = 2;
			*format = GL_BGR;	// OpenGL supports 4444, 5551, 444 and 555, but BBitmap doesn't	
			break;	
		case B_CMAP8:
			*num_bytes = 1;
			*format = GL_COLOR_INDEX;
			break;		
		case B_GRAY8:
			*num_bytes = 1;
			*format = GL_LUMINANCE;
			break;
		default:	// all other formats are unsupported
			*num_bytes = 4;
			*format = GL_RGBA;
	}		
}

/*	FUNCTION:		ViewObject :: GLCreateTexture
	ARGUMENTS:		bitmap
	RETURN:			OpenGL texture reference
	DESCRIPTION:	Create an OpenGL texture from a BBitmap.
					The client takes ownership of the create media source.
*/
void ViewObject :: GLCreateTexture(MediaSource *media, BBitmap *bitmap)
{
	assert(media != 0);
	assert(bitmap != 0);
	
	LockGL();
	glGenTextures(1, &media->mTextureID);
	glBindTexture(GL_TEXTURE_2D, media->mTextureID);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	int num_bytes;
	GLDetermineFormat(bitmap->ColorSpace(), &media->mTextureFormat, &num_bytes);
	media->mTextureWidth = (int)(bitmap->Bounds().Width() + 1);
	media->mTextureHeight = (int)(bitmap->Bounds().Height() + 1);
	glTexImage2D(GL_TEXTURE_2D,
				0, 									// mipmap level
				num_bytes,							// internal format
				media->mTextureWidth,				// width
				media->mTextureHeight,				// height
				0,									// border
				media->mTextureFormat,				// format of pixel data
				GL_UNSIGNED_BYTE,					// data type
				(unsigned char *)bitmap->Bits());	// pixels
	UnlockGL();
}

/*	FUNCTION:		ViewObject :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Should be overriden by derived class	
*/
void ViewObject :: Render(void)
{
	LockGL();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// derived class should draw something here
	
	glFlush();
	SwapBuffers();
	UnlockGL();
}

/*	FUNCTION:		ViewObject :: DragDrop
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when user drops files in application window.
					Essentially, determine if file is image or video.	
*/
void ViewObject :: DragDrop(entry_ref *ref, float mouse_x, float mouse_y)
{
	//	Image file?
	BPath path(ref);
	BBitmap *bitmap = BTranslationUtils::GetBitmap(path.Path(), NULL);
	if (bitmap)
	{
		MediaSource *media = new MediaSource(this);
		GLCreateTexture(media, bitmap);
		if (!SurfaceUpdate(media, mouse_x, mouse_y))
			delete media;
		delete bitmap;
		return;
	}
	
	//	Video file
	Video *video = new Video(ref);
	if (video->GetStatus() == B_OK)
	{
		MediaSource *media = new MediaSource(this);
		GLCreateTexture(media, video->GetBitmap());
		media->SetVideo(video);
		if (SurfaceUpdate(media, mouse_x, mouse_y))
			video->Start();
		else
			delete media;
		return;
	}
	else
		delete video;
	
	printf("Unsupported file\n");
}

/*	FUNCTION:		ViewObject :: UpdateFrame
	ARGUMENTS:		source
	RETURN:			n/a
	DESCRIPTION:	Called by Video thread - update video texture	
*/
void ViewObject :: UpdateFrame(MediaSource *source)
{
	LockGL();
	glBindTexture(GL_TEXTURE_2D, source->mTextureID);	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, source->mTextureWidth, source->mTextureHeight,
					 source->mTextureFormat, GL_UNSIGNED_BYTE, source->mVideo->GetBitmap()->Bits());
	UnlockGL();
}

/*	FUNCTION:		ViewObject :: ToggleWireframe
	ARGUMENTS:		enable
	RETURN:			n/a
	DESCRIPTION:	Switch beteeen wireframe and fill polygon modes
*/
void ViewObject :: ToggleWireframe(bool enable)
{
	LockGL();
	if (enable)
		glPolygonMode(GL_FRONT, GL_LINE);
	else
		glPolygonMode(GL_FRONT, GL_FILL);
	UnlockGL();
}

/*	FUNCTION:		ViewObject :: GLCheckError
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Check for OpenGL errors
*/
void ViewObject :: GLCheckError()
{
	GLenum err;
	
	LockGL();
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		printf("[GL Error]  ");
		switch (err)
		{
			case GL_INVALID_ENUM:		printf("GL_INVALID_ENUM\n");		break;
			case GL_INVALID_VALUE:		printf("GL_INVALID_VALUE\n");		break;
			case GL_INVALID_OPERATION:	printf("GL_INVALID_OPERATION\n");	break;
			case GL_STACK_OVERFLOW:		printf("GL_STACK_OVERFLOW\n");		break;
			case GL_STACK_UNDERFLOW:	printf("GL_STACK_UNDERFLOW\n");		break;
			case GL_OUT_OF_MEMORY:		printf("GL_OUT_OF_MEMORY\n");		break;
			default:					printf("%d\n", err);				break;
		}
	}
	UnlockGL();
}


