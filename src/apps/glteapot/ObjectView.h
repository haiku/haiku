/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef OBJECT_VIEW_H
#define OBJECT_VIEW_H

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
						ObjectView(BRect r, char* name, ulong resizingMode,
							ulong options);
						~ObjectView();

		virtual	void	MouseDown(BPoint p);
		virtual	void	MessageReceived(BMessage* msg);
		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();
		virtual	void	FrameResized(float width, float height);
				bool	SpinIt();
				int		ObjectAtPoint(BPoint p);
		virtual	void	DrawFrame(bool noPause);
		virtual	void	Pulse();
				void	EnforceState();
				bool	RepositionView();
		
		sem_id			drawEvent;
		sem_id			quittingSem;
		
	private:				
		thread_id		fDrawThread;
		ResScroll*		fResScroll;
		BList			fObjects;
		BLocker			fObjListLock;
		uint64			fLastFrame;
		int32			fHistEntries,fOldestEntry;
		bool			fFps, fLastGouraud, fGouraud;
		bool			fLastZbuf, fZbuf, fLastCulling, fCulling;
		bool			fLastLighting, fLighting, fLastFilled, fFilled;
		bool			fLastPersp, fPersp, fLastTextured, fTextured;
		bool			fLastFog, fFog, fForceRedraw;
		float			fLastYXRatio, fYxRatio, fFpsHistory[HISTSIZE];
		float			fObjectDistance,fLastObjectDistance;
};

#endif // OBJECT_VIEW_H
