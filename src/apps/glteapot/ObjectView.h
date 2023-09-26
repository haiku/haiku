/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef OBJECT_VIEW_H
#define OBJECT_VIEW_H

#include <GL/glu.h>
#include <GLView.h>

#define kMsgFPS			'fps '
#define kMsgLimitFps	'lfps'
#define kMsgAddModel	'addm'
#define kMsgGouraud		'gour'
#define kMsgZBuffer		'zbuf'
#define kMsgCulling		'cull'
#define kMsgTextured	'txtr'
#define kMsgFog			'fog '
#define kMsgLighting	'lite'
#define kMsgLights		'lits'
#define kMsgFilled		'fill'
#define kMsgPerspective	'prsp'

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
class GLObject;

struct TrackingInfo {
	float		lastX;
	float		lastY;
	float		lastDx;
	float		lastDy;
	bool		isTracking;
	GLObject	*pickedObject;
	uint32		buttons;
};

class ObjectView : public BGLView {
	public:
						ObjectView(BRect rect, const char* name,
							ulong resizingMode, ulong options);
						~ObjectView();

		virtual	void	MouseDown(BPoint point);
		virtual	void	MouseUp(BPoint point);
		virtual	void	MouseMoved(BPoint point, uint32 transit, const BMessage *msg);

		virtual	void	MessageReceived(BMessage* msg);
		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();
		virtual	void	FrameResized(float width, float height);
				bool	SpinIt();
				bool	LimitFps() { return fLimitFps; };
				int		ObjectAtPoint(const BPoint &point);
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
		bool			fFps, fLimitFps, fLastGouraud, fGouraud;
		bool			fLastZbuf, fZbuf, fLastCulling, fCulling;
		bool			fLastLighting, fLighting, fLastFilled, fFilled;
		bool			fLastPersp, fPersp, fLastTextured, fTextured;
		bool			fLastFog, fFog, fForceRedraw;
		float			fLastYXRatio, fYxRatio, fFpsHistory[HISTSIZE];
		float			fObjectDistance, fLastObjectDistance;
		TrackingInfo	fTrackingInfo;
};

#endif // OBJECT_VIEW_H
