/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 */

#include <Directory.h>
#include <FindDirectory.h>
#include <image.h>
#include <Path.h>
#include <String.h>
#include "GLDispatcher.h"
#include "GLRendererRoster.h"

#include <new>
#include <string.h>



GLRendererRoster::GLRendererRoster(BGLView *view, ulong options)
	: fNextID(0),
	fView(view),
	fOptions(options)
{
        AddDefaultPaths();
}


GLRendererRoster::~GLRendererRoster()
{

}


BGLRenderer *
GLRendererRoster::GetRenderer(int32 id)
{
	RendererMap::const_iterator iterator = fRenderers.find(id);
	if (iterator == fRenderers.end())
		return NULL;

	struct renderer_item item = iterator->second;
	return item.renderer;
}


void
GLRendererRoster::AddDefaultPaths()
{
	// add user directories first, so that they can override system renderers
	const directory_which paths[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY,
	};

	for (uint32 i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
		BPath path;
		status_t status = find_directory(paths[i], &path, true);
		if (status == B_OK && path.Append("opengl") == B_OK)
			AddPath(path.Path());
	}
}


status_t
GLRendererRoster::AddPath(const char* path)
{
	BDirectory directory(path);
	status_t status = directory.InitCheck();
	if (status < B_OK)
		return status;

	node_ref nodeRef;
	status = directory.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	int32 count = 0;
	int32 files = 0;

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		if (CreateRenderer(ref) == B_OK)
			count++;

		files++;
	}

	if (files != 0 && count == 0)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
GLRendererRoster::AddRenderer(BGLRenderer* renderer,
	image_id image, const entry_ref* ref, ino_t node)
{
	renderer_item item;
	item.renderer = renderer;
	item.image = image;
	item.node = node;
	if (ref != NULL)
		item.ref = *ref;

	try {
		fRenderers[fNextID] = item;
	} catch (...) {
		return B_NO_MEMORY;
	}

	renderer->fOwningRoster = this;
	renderer->fID = fNextID++;
	return B_OK;
}


status_t
GLRendererRoster::CreateRenderer(const entry_ref& ref)
{
	BEntry entry(&ref);
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return image;

	BGLRenderer *(*instanc)(BGLView *view, ulong options, BGLDispatcher *dispatcher);

	status = get_image_symbol(image, "instanciate_gl_renderer",
		B_SYMBOL_TYPE_TEXT, (void**)&instanc);
	if (status == B_OK) {
		BGLRenderer *renderer = instanc(fView, fOptions, new BGLDispatcher());
		if (!renderer) {
			unload_add_on(image);
			return B_UNSUPPORTED;
		}
		
		if (AddRenderer(renderer, image, &ref, nodeRef.node) != B_OK) {
			renderer->Release();
			// this will delete the renderer
			unload_add_on(image);
		}
		return B_OK;
	}
	unload_add_on(image);

	return status;
}

