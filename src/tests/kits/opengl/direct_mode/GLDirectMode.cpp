// sample BGLView app from the Be Book, modified to stress direct mode support.


#include <stdio.h>

#include <Application.h>
#include <DirectWindow.h>
#include <CheckBox.h>
#include <GLView.h>

class SampleGLView : public BGLView
{
public:
   SampleGLView(BRect frame, uint32 type);
   virtual void   AttachedToWindow(void);
   virtual void   FrameResized(float newWidth, float newHeight);
   virtual void   ErrorCallback(GLenum which);
   
   void         Render(void);
   
private:
   void         gInit(void);
   void         gDraw(void);
   void         gReshape(int width, int height);
         
   float         width;
   float         height;
};



class SampleGLWindow : public BDirectWindow 
{
public:
   SampleGLWindow(BRect frame, uint32 type);
   virtual bool QuitRequested();
   virtual void DirectConnected( direct_buffer_info *info );
   
private:
   SampleGLView   *theView;
};


class SampleGLApp : public BApplication
{
public:
   SampleGLApp();
private:
   SampleGLWindow      *theWindow;
};


SampleGLApp::SampleGLApp()
   : BApplication("application/x-vnd.sample")
{
   BRect windowRect;
   uint32 type = BGL_RGB|BGL_DOUBLE;

   windowRect.Set(50, 50, 350, 350);

   theWindow = new SampleGLWindow(windowRect, type);
}



SampleGLWindow::SampleGLWindow(BRect frame, uint32 type)
   : BDirectWindow(frame, "OpenGL Test", B_TITLED_WINDOW, 0)
{
   BRect r = Bounds();
	
   r.InsetBy(10, 10);
   theView = new SampleGLView(r, type);
   AddChild(theView);
   Show();
   theView->Render();
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



SampleGLView::SampleGLView(BRect frame, uint32 type)
   : BGLView(frame, "SampleGLView", B_FOLLOW_ALL_SIDES, 0, type)
{
   width = frame.right-frame.left;
   height = frame.bottom-frame.top;
}


void SampleGLView::AttachedToWindow(void)
{
   LockGL();
   BGLView::AttachedToWindow();
   gInit();
   gReshape(width, height);
   UnlockGL();
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


void SampleGLView::ErrorCallback(GLenum whichError) 
{
//      fprintf(stderr, "Unexpected error occured (%d):\\n", whichError);
//      fprintf(stderr, "    %s\\n", gluErrorString(whichError));
}



// globals
GLenum use_stipple_mode;    // GL_TRUE to use dashed lines
GLenum use_smooth_mode;     // GL_TRUE to use anti-aliased lines
GLint linesize;             // Line width
GLint pointsize;            // Point diameter
   
float pntA[3] = {
   -160.0, 0.0, 0.0
};
float pntB[3] = {
   -130.0, 0.0, 0.0
};



void SampleGLView::gInit(void) 
{
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glLineStipple(1, 0xF0E0);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   use_stipple_mode = GL_FALSE;
   use_smooth_mode = GL_TRUE;
   linesize = 2;
   pointsize = 6;
}



void SampleGLView::gDraw(void)
{
   GLint i;
   
   glClear(GL_COLOR_BUFFER_BIT);
   glLineWidth(linesize);
   
/*

   if (use_stipple_mode) {
      glEnable(GL_LINE_STIPPLE);
   } else {
      glDisable(GL_LINE_STIPPLE);
   }
*/

   glDisable(GL_POINT_SMOOTH);


   glPushMatrix();
   
   glPointSize(pointsize);         // Set size for point

   for (i = 0; i < 360; i += 5) {
      glRotatef(5.0, 0,0,1);         // Rotate right 5 degrees

	  if (use_smooth_mode) {
		glEnable(GL_LINE_SMOOTH);
	    glEnable(GL_BLEND);
	   } else {
	      glDisable(GL_LINE_SMOOTH);
	      glDisable(GL_BLEND);
	   }

      glColor3f(1.0, 1.0, 0.0);      // Set color for line
      glBegin(GL_LINE_STRIP);         // And create the line
      	glVertex3fv(pntA);
      	glVertex3fv(pntB);
      glEnd();

      glDisable(GL_POINT_SMOOTH);
      glDisable(GL_BLEND);

      glColor3f(0.0, 1.0, 0.0);      // Set color for point
      glBegin(GL_POINTS);
      	glVertex3fv(pntA);         // Draw point at one end
      	glVertex3fv(pntB);         // Draw point at other end
      glEnd();
   }
   
   glPopMatrix();                  // Done with matrix
}


void SampleGLView::gReshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-175, 175, -175, 175, -1, 1);
   glMatrixMode(GL_MODELVIEW);
}


void SampleGLView::Render(void)
{
   LockGL();
   gDraw();
   SwapBuffers();
   UnlockGL();
}



int main(int argc, char *argv[])
{
   SampleGLApp *app = new SampleGLApp;
   app->Run();
   delete app;
   return 0;
}
