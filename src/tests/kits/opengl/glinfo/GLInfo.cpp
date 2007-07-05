// Small app to display GL infos

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Application.h>
#include <Window.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <GLView.h>

#include <String.h>

#include <GL/gl.h>
#include <GL/glu.h>

#define GLUT_INFO 1
#ifdef GLUT_INFO
	#include <GL/glut.h>
#endif


class GLInfoWindow : public BWindow 
{
public:
   GLInfoWindow(BRect frame);
   virtual bool   QuitRequested() { be_app->PostMessage(B_QUIT_REQUESTED); return true; }
   
private:
	BGLView 			*gl;
	BOutlineListView	*list;
	BScrollView			*scroller;
};


class GLInfoApp : public BApplication
{
public:
   GLInfoApp();
private:
   GLInfoWindow      *window;
};


void AddItem(BListView *list, int level, char *text, ...)
{
	char buffer[512];
	va_list arg;
	
	va_start(arg, text);
	vsnprintf(buffer, sizeof(buffer), text, arg);
	va_end(arg);
	
	list->AddItem(new BStringItem(buffer, level));
}
 

GLInfoApp::GLInfoApp()
   : BApplication("application/x-vnd.OBOS-GLInfo")
{
   window = new GLInfoWindow(BRect(50, 50, 350, 350));
}

GLInfoWindow::GLInfoWindow(BRect frame)
   : BWindow(frame, "OpenGL Info", B_TITLED_WINDOW, 0)
{
	BRect rect = Bounds();
	char *string;

	// Add a outline list view
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	list 		= new BOutlineListView(rect, "GLInfoList", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	scroller 	= new BScrollView("GLInfoListScroller", list, B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS, false, true);
						
	gl = new BGLView(rect, "opengl", B_FOLLOW_ALL_SIDES, 0, BGL_RGB | BGL_DOUBLE);
	gl->Hide();
	AddChild(gl);
	AddChild(scroller);
	
	Show();
	
	LockLooper();
	
	gl->LockGL();

	// ---- OpenGL info
	
	AddItem(list, 0, "OpenGL");
	AddItem(list, 1, "Version: %s", glGetString(GL_VERSION));
	AddItem(list, 1, "Vendor Name: %s", glGetString(GL_VENDOR));
	AddItem(list, 1, "Renderer Name: %s", glGetString(GL_RENDERER));

	// Renderer capabilities
	
	int lights = 0;
	int clipping_planes = 0;
	int model_stack = 0;
	int projection_stack = 0;
	int texture_stack = 0;
	int max_tex3d = 0;
	int max_tex2d = 0;
	int name_stack = 0;
	int list_stack = 0;
	int max_poly = 0;
	int attrib_stack = 0;
	int buffers = 0;
	int convolution_width = 0;
	int convolution_height = 0;
	int max_index = 0;
	int max_vertex = 0;
	int texture_units = 0;

	glGetIntegerv (GL_MAX_LIGHTS, &lights);
	glGetIntegerv (GL_MAX_CLIP_PLANES, &clipping_planes);
	glGetIntegerv (GL_MAX_MODELVIEW_STACK_DEPTH, &model_stack);
	glGetIntegerv (GL_MAX_PROJECTION_STACK_DEPTH, &projection_stack);
	glGetIntegerv (GL_MAX_TEXTURE_STACK_DEPTH, &texture_stack);
	glGetIntegerv (GL_MAX_3D_TEXTURE_SIZE, &max_tex3d);
	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &max_tex2d);
	glGetIntegerv (GL_MAX_NAME_STACK_DEPTH, &name_stack);
	glGetIntegerv (GL_MAX_LIST_NESTING, &list_stack);
	glGetIntegerv (GL_MAX_EVAL_ORDER, &max_poly);
	glGetIntegerv (GL_MAX_ATTRIB_STACK_DEPTH, &attrib_stack);
	glGetIntegerv (GL_AUX_BUFFERS, &buffers);
	glGetIntegerv (GL_MAX_CONVOLUTION_WIDTH, &convolution_width);
	glGetIntegerv (GL_MAX_CONVOLUTION_HEIGHT, &convolution_height);
	glGetIntegerv (GL_MAX_ELEMENTS_INDICES, &max_index);
	glGetIntegerv (GL_MAX_ELEMENTS_VERTICES, &max_vertex);
	glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &texture_units);

	AddItem(list, 1, "Capabilities");

	AddItem(list, 2, "Auxiliary buffer(s): %d", buffers);

	AddItem(list, 2, "Model stack size: %d", model_stack);
	AddItem(list, 2, "Projection stack size: %d", projection_stack);
	AddItem(list, 2, "Texture stack size: %d", texture_stack);
	AddItem(list, 2, "Name stack size: %d", name_stack);
	AddItem(list, 2, "List stack size: %d", list_stack);
	AddItem(list, 2, "Attributes stack size: %d", attrib_stack);

	AddItem(list, 2, "Maximum 3D texture size: %d", max_tex3d);
	AddItem(list, 2, "Maximum 2D texture size: %d", max_tex2d);
	AddItem(list, 2, "Maximum texture units: %d", texture_units);

	AddItem(list, 2, "Maximum lights: %d", lights);
	AddItem(list, 2, "Maximum clipping planes: %d", clipping_planes);
	AddItem(list, 2, "Maximum evaluators equation order: %d", max_poly);
	AddItem(list, 2, "Maximum convolution: %d x %d",
		convolution_width, convolution_height);
	AddItem(list, 2, "Maximum recommended index elements: %d", max_index);
	AddItem(list, 2, "Maximum recommended vertex elements: %d", max_vertex);

	// Renderer supported extensions
	string = (char *) glGetString(GL_EXTENSIONS);
	if (string) {
		AddItem(list, 1, "Extensions");
		while (*string) {
			char extname[255];
			int n = strcspn(string, " ");
			strncpy(extname, string, n);
			extname[n] = 0;
			AddItem(list, 2, extname);
			if (! string[n])
				break;
			string += (n + 1);	// next !
		}
	}

	// ---- GLU info

	AddItem(list, 0, "GLU");
	AddItem(list, 1, "Version: %s", gluGetString(GLU_VERSION));

	string = (char *) gluGetString(GLU_EXTENSIONS);
	if (string) {
		AddItem(list, 1, "Extensions");
		while (*string) {
			char extname[255];
			int n = strcspn(string, " ");
			strncpy(extname, string, n);
			extname[n] = 0;
			AddItem(list, 2, extname);
			if (! string[n])
				break;
			string += (n + 1);	// next !
		}
	}

#ifdef GLUT_INFO
	// ---- GLUT Info
	AddItem(list, 0, "GLUT");
	AddItem(list, 1, "API version: %d", GLUT_API_VERSION);
#endif

	gl->UnlockGL();

	UnlockLooper();
}



int main(int argc, char *argv[])
{
   GLInfoApp *app = new GLInfoApp;
   app->Run();
   delete app;
   return 0;
}
