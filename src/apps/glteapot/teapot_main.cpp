/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <InterfaceKit.h>
#include <stdio.h>
#include "ObjectView.h"

class QuitWindow : public BDirectWindow
{
public:
	QuitWindow(BRect r, char *name, window_type wt, ulong something);
	
	virtual bool QuitRequested();
	virtual void DirectConnected( direct_buffer_info *info );
	ObjectView *bgl;

};

QuitWindow::QuitWindow(BRect r, char *name, window_type wt, ulong something)
	: BDirectWindow(r,name,wt,something)
{ 
}

bool QuitWindow::QuitRequested()
{
printf( "closing \n" );
	bgl->EnableDirectMode( false );
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
};

void QuitWindow::DirectConnected( direct_buffer_info *info )
{
	if( bgl )
		bgl->DirectConnected( info );	
	bgl->EnableDirectMode( true );
}

int main(int argc, char **argv)
{
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
    //GLenum type = BGL_RGB | BGL_DEPTH | BGL_SINGLE;
	BRect r(0,0,300,315);
	
	BApplication *app = new BApplication("application/x-vnd.Haiku-GLTeapot");
	
	r.OffsetTo(BPoint(100,100));
	QuitWindow *win = new QuitWindow(r,"GLTeapot",B_TITLED_WINDOW,0);

	win->Lock();
	r = win->Bounds();
	r.bottom = r.top + 14;
	BMenuBar *mb = new BMenuBar(r,"main menu");

	BMenu *m;
	BMenuItem *i;
	BMessage msg (bmsgAddModel);

	mb->AddItem(m = new BMenu("File"));
	win->AddChild(mb);
	mb->ResizeToPreferred();

	r = win->Bounds();
	r.top = mb->Bounds().bottom+1;
	BView *sv = new BView(r,"subview",B_FOLLOW_ALL,0);
	win->AddChild(sv);
	r = sv->Bounds();
	ObjectView *bgl = new ObjectView(r,"objectView",B_FOLLOW_NONE,type);
	sv->AddChild(bgl);
	win->bgl = bgl;
	
	msg.AddInt32("num",256);
	m->AddItem(i = new BMenuItem("Add a teapot",new BMessage(msg), 'N'));
	i->SetTarget(bgl);	
	m->AddSeparatorItem();
	m->AddItem(i = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	i->SetTarget(be_app);
	msg.RemoveName("num");
	mb->AddItem(m = new BMenu("Options"));
	m->AddItem(i = new BMenuItem("Perspective",new BMessage(bmsgPerspective)));
	i->SetTarget(bgl);
	i->SetMarked(false);
	m->AddItem(i = new BMenuItem("FPS Display",new BMessage(bmsgFPS)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Filled polygons",new BMessage(bmsgFilled)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Lighting",new BMessage(bmsgLighting)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Backface culling",new BMessage(bmsgCulling)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Z-buffered",new BMessage(bmsgZBuffer)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Gouraud shading",new BMessage(bmsgGouraud)));
	i->SetTarget(bgl);
	i->SetMarked(true);
//	m->AddItem(i = new BMenuItem("Texture mapped",new BMessage(bmsgTextured)));
//	i->SetTarget(bgl);
	m->AddItem(i = new BMenuItem("Fog",new BMessage(bmsgFog)));
	i->SetTarget(bgl);

	BMenu *sm;
	mb->AddItem(m = new BMenu("Lights"));
	msg.what = bmsgLights;

	msg.AddInt32("num",1);
	m->AddItem(i = new BMenuItem(sm = new BMenu("Upper center"),NULL));
	i->SetTarget(bgl);
	msg.AddInt32("color",lightNone);
	sm->AddItem(i = new BMenuItem("Off",new BMessage(msg)));
	i->SetTarget(bgl);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(i = new BMenuItem("White",new BMessage(msg)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(i = new BMenuItem("Yellow",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(i = new BMenuItem("Blue",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(i = new BMenuItem("Red",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(i = new BMenuItem("Green",new BMessage(msg)));
	i->SetTarget(bgl);

	msg.RemoveName("color");

	msg.ReplaceInt32("num",2);
	m->AddItem(i = new BMenuItem(sm = new BMenu("Lower left"),NULL));
	i->SetTarget(bgl);
	msg.AddInt32("color",lightNone);
	sm->AddItem(i = new BMenuItem("Off",new BMessage(msg)));
	i->SetTarget(bgl);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(i = new BMenuItem("White",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(i = new BMenuItem("Yellow",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(i = new BMenuItem("Blue",new BMessage(msg)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(i = new BMenuItem("Red",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(i = new BMenuItem("Green",new BMessage(msg)));
	i->SetTarget(bgl);

	msg.RemoveName("color");

	msg.ReplaceInt32("num",3);
	m->AddItem(i = new BMenuItem(sm = new BMenu("Right"),NULL));
	i->SetTarget(bgl);
	msg.AddInt32("color",lightNone);
	sm->AddItem(i = new BMenuItem("Off",new BMessage(msg)));
	i->SetTarget(bgl);
	i->SetMarked(true);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(i = new BMenuItem("White",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(i = new BMenuItem("Yellow",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(i = new BMenuItem("Blue",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(i = new BMenuItem("Red",new BMessage(msg)));
	i->SetTarget(bgl);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(i = new BMenuItem("Green",new BMessage(msg)));
	i->SetTarget(bgl);

	float f = mb->Bounds().IntegerHeight()+1;
	win->SetSizeLimits(32,1024,32+f,1024+f);
	
	/*
	r = win->Bounds();
	r.top = mb->Bounds().bottom+1;
	sv->MoveTo(r.left,r.top);
	sv->ResizeTo(r.right-r.left+1,r.bottom-r.top+1);
	bgl->ResizeTo(r.right-r.left+1,r.bottom-r.top+1);
	*/
	
	win->Unlock();
	win->Show();
	
	app->Run();
	delete app;
	
	return 0;
}
