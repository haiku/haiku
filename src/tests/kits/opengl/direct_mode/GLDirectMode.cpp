// sample BGLView app from the Be Book, modified to stress direct mode support.


#include <stdio.h>

#include <Application.h>
#include <DirectWindow.h>
#include <CheckBox.h>
#include <GLView.h>
#include <GL/glu.h>

#define REDRAW_MSG	'rdrw'

class SampleGLView : public BGLView
{
public:
   SampleGLView(BRect frame, uint32 type);
   virtual void   AttachedToWindow(void);
   virtual void   FrameResized(float newWidth, float newHeight);
   virtual void   MessageReceived(BMessage * msg);
   virtual void   KeyDown(const char* bytes, int32 numBytes);
   
   void         Render(void);
   
private:
   void         gInit(void);
   void         gDraw(float rotation = 0);
   void         gReshape(int width, int height);
         
   float        width;
   float        height;
   float		rotate;
};



class SampleGLWindow : public BDirectWindow 
{
public:
   SampleGLWindow(BRect frame, uint32 type);
   ~SampleGLWindow();
   
   virtual bool QuitRequested();
   virtual void DirectConnected( direct_buffer_info *info );
   
private:
   SampleGLView   *theView;
   BMessageRunner *updateRunner;
};


class SampleGLApp : public BApplication
{
public:
   SampleGLApp();
private:
   SampleGLWindow      *theWindow;
};


SampleGLApp::SampleGLApp()
   : BApplication("application/x-vnd.Haiku-GLDirectMode")
{
   BRect windowRect;
   uint32 type = BGL_RGB|BGL_DOUBLE|BGL_DEPTH;

   windowRect.Set(50, 50, 350, 350);

   theWindow = new SampleGLWindow(windowRect, type);
}



SampleGLWindow::SampleGLWindow(BRect frame, uint32 type)
   : BDirectWindow(frame, "GLDirectMode", B_TITLED_WINDOW, 0)
{
   float minWidth = 0.0f; 
   float maxWidth = 0.0f; 
   float minHeight = 0.0f; 
   float maxHeight = 0.0f; 
 	
   GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight); 
   SetSizeLimits(50.0f, maxWidth, 50.0f, maxHeight); 
	
   BRect r = Bounds();
	
   r.InsetBy(10, 10);
   theView = new SampleGLView(r, type);
   AddChild(theView);
   Show();
   
   updateRunner = new BMessageRunner(BMessenger(theView),
   		new BMessage(REDRAW_MSG), 1000000/60 /* 60 fps */);
   
   theView->Render();
}


SampleGLWindow::~SampleGLWindow()
{
	delete updateRunner;
}


bool SampleGLWindow::QuitRequested()
{
	theView->EnableDirectMode(false);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void SampleGLWindow::DirectConnected(direct_buffer_info *info)
{
	theView->DirectConnected(info);	
	theView->EnableDirectMode(true);
}

// ----

SampleGLView::SampleGLView(BRect frame, uint32 type)
   : BGLView(frame, "SampleGLView", B_FOLLOW_ALL_SIDES, 0, type), rotate(0)
{
   width = frame.right-frame.left;
   height = frame.bottom-frame.top;
}


void SampleGLView::AttachedToWindow(void)
{
   BGLView::AttachedToWindow();
   LockGL();
   gInit();
   gReshape(width, height);
   UnlockGL();
   MakeFocus();
}


void SampleGLView::FrameResized(float newWidth, float newHeight) 
{
   BGLView::FrameResized(newWidth, newHeight);

   LockGL();

   width = newWidth;
   height = newHeight;
   
   gReshape(width,height);
      
   UnlockGL();
   Render();
}


void SampleGLView::gInit(void) 
{
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}



void SampleGLView::gDraw(float rotation)
{
  /* Clear the buffer, clear the matrix */
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  /* A step backward, then spin the cube */
  glTranslatef(0, 0, -5);
  glRotatef(rotation, 0, 0, 1);
  glRotatef(rotation, 1, 0.6, 0);

  /* We tell we want to draw quads */
  glBegin (GL_QUADS);

  /* Every four calls to glVertex, a quad is drawn */
  glColor3f (0, 0, 0); glVertex3f (-1, -1, -1);
  glColor3f (0, 0, 1); glVertex3f (-1, -1,  1);
  glColor3f (0, 1, 1); glVertex3f (-1,  1,  1);
  glColor3f (0, 1, 0); glVertex3f (-1,  1, -1);

  glColor3f (1, 0, 0); glVertex3f ( 1, -1, -1);
  glColor3f (1, 0, 1); glVertex3f ( 1, -1,  1);
  glColor3f (1, 1, 1); glVertex3f ( 1,  1,  1);
  glColor3f (1, 1, 0); glVertex3f ( 1,  1, -1);

  glColor3f (0, 0, 0); glVertex3f (-1, -1, -1);
  glColor3f (0, 0, 1); glVertex3f (-1, -1,  1);
  glColor3f (1, 0, 1); glVertex3f ( 1, -1,  1);
  glColor3f (1, 0, 0); glVertex3f ( 1, -1, -1);

  glColor3f (0, 1, 0); glVertex3f (-1,  1, -1);
  glColor3f (0, 1, 1); glVertex3f (-1,  1,  1);
  glColor3f (1, 1, 1); glVertex3f ( 1,  1,  1);
  glColor3f (1, 1, 0); glVertex3f ( 1,  1, -1);

  glColor3f (0, 0, 0); glVertex3f (-1, -1, -1);
  glColor3f (0, 1, 0); glVertex3f (-1,  1, -1);
  glColor3f (1, 1, 0); glVertex3f ( 1,  1, -1);
  glColor3f (1, 0, 0); glVertex3f ( 1, -1, -1);

  glColor3f (0, 0, 1); glVertex3f (-1, -1,  1);
  glColor3f (0, 1, 1); glVertex3f (-1,  1,  1);
  glColor3f (1, 1, 1); glVertex3f ( 1,  1,  1);
  glColor3f (1, 0, 1); glVertex3f ( 1, -1,  1);

  /* No more quads */
  glEnd ();

  /* End */
  glFlush ();
}


void SampleGLView::gReshape(int width, int height)
{
   glViewport(0, 0, width, height);
   if (height) {  // Avoid Divide By Zero error...
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(45, width / (float) height, 1, 500);
      glMatrixMode(GL_MODELVIEW);
   }
}


void SampleGLView::Render(void)
{
   LockGL();
   gDraw(rotate);
   SwapBuffers();
   UnlockGL();
}

void SampleGLView::MessageReceived(BMessage * msg)
{
	switch (msg->what) {
	case REDRAW_MSG:
		Render();
		/* Rotate a bit more */
		rotate++;
		break;

	default:	
		BGLView::MessageReceived(msg);
	}
}

void SampleGLView::KeyDown(const char *bytes, int32 numBytes)
{
	static bool moved = false;
	switch (bytes[0]) {
	case B_SPACE:
		if (moved) {
			MoveBy(-30, -30);
			moved = false;
		} else {
			MoveBy(30, 30);
			moved = true;
		}
		break;
	default:
		BView::KeyDown(bytes, numBytes);
		break;
	}
}

int main(int argc, char *argv[])
{
   SampleGLApp *app = new SampleGLApp;
   app->Run();
   delete app;
   return 0;
}
