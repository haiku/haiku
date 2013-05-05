/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin <philippe.houdoin@free.fr>
 */
#ifndef _GLRENDERER_ROSTER_H
#define _GLRENDERER_ROSTER_H


#include <GLRenderer.h>

#include <map>


struct renderer_item {
	BGLRenderer* renderer;
	entry_ref	ref;
	ino_t		node;
	image_id	image;
};

typedef std::map<renderer_id, renderer_item> RendererMap;


class GLRendererRoster {
	public:
		GLRendererRoster(BGLView* view, ulong options);
		virtual ~GLRendererRoster();

		BGLRenderer* GetRenderer(int32 id = 0);

	private:
		void AddDefaultPaths();
		status_t AddPath(const char* path);
		status_t AddRenderer(BGLRenderer* renderer,
			image_id image, const entry_ref* ref, ino_t node);
		status_t CreateRenderer(const entry_ref& ref);

		RendererMap	fRenderers;
		int32		fNextID;
		BGLView*	fView;
		ulong		fOptions;
		bool		fSafeMode;
		const char*	fABISubDirectory;

};


#endif	/* _GLRENDERER_ROSTER_H */
