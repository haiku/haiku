/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _RENDERVIEW_H
#define _RENDERVIEW_H

#include <GLView.h>

#include <vector>


class Camera;
class MeshInstance;


class RenderView : public BGLView {
public:
					RenderView(BRect frame);
					~RenderView();

	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	FrameResized(float width, float height);
	virtual	void	ErrorCallback(unsigned long error);

protected:
	void			_InitGL();
	void			_CreateScene();
	void			_DeleteScene();
	void			_UpdateViewport();
	void			_UpdateCamera();
	bool			_Render();
	uint32			_CreateRenderThread();
	void			_StopRenderThread();
	static int32	_RenderThreadEntry(void* pointer);
	int32			_RenderLoop();

	Camera*			fMainCamera;

	typedef std::vector<MeshInstance*> MeshInstanceList;
	MeshInstanceList	fMeshInstances;

	thread_id		fRenderThread;
	bool			fStopRendering;

	BPoint			fRes, fNextRes;
	bigtime_t		fLastFrameTime;
};

#endif /* _RENDERVIEW_H */
