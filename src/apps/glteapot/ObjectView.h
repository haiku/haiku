/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef ObjectView_h
#define ObjectView_h

#include <GL/glu.h>
#include <GLView.h>

#define bmsgFPS	        'fps '
#define bmsgAddModel	'addm'
#define bmsgGouraud		'gour'
#define bmsgZBuffer		'zbuf'
#define bmsgCulling 	'cull'
#define bmsgTextured	'txtr'
#define bmsgFog			'fog '
#define bmsgLighting	'lite'
#define bmsgLights		'lits'
#define bmsgFilled		'fill'
#define bmsgPerspective	'prsp'

enum lights {
	lightNone = 0,
	lightWhite,
	lightYellow,
	lightRed,
	lightBlue,
	lightGreen
};

#define HISTSIZE 10

class ResScroll;
class ObjectView : public BGLView {
public:
sem_id			drawEvent;
sem_id			quittingSem;
thread_id		drawThread;
ResScroll *		resScroll;

BList			objects;
BLocker         objListLock;

float           yxRatio,lastYXRatio;
uint64          lastFrame;
float           fpsHistory[HISTSIZE];
int32           histEntries,oldestEntry;
bool            fps;
bool			gouraud, lastGouraud;
bool			zbuf,lastZbuf;
bool			culling,lastCulling;
bool			lighting,lastLighting;
bool			filled,lastFilled;
bool			persp,lastPersp;
bool			lastTextured,textured;
bool			lastFog,fog;
bool			forceRedraw;
float			objectDistance,lastObjectDistance;

				ObjectView(BRect r, char *name, 
					ulong resizingMode, ulong options);
				~ObjectView();

virtual	void	MouseDown(BPoint p);
virtual	void	MessageReceived(BMessage *msg);
virtual	void	AttachedToWindow();
virtual	void	DetachedFromWindow();
virtual	void	FrameResized(float width, float height);
		bool	SpinIt();
		int		ObjectAtPoint(BPoint p);
virtual	void	DrawFrame(bool noPause);
virtual	void	Pulse();
		void	EnforceState();
		bool	RepositionView();
};

#endif
