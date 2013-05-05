/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//	Copyright 1997, 1998, Be Incorporated, All Rights Reserved.
//
//	NodePreloader manages caching up icons from apps and prefs folder for
//	fast display of the app/prefs nav menus
//
//  Icons end up in the node cache as permanent entries -- to be able
//  to do this, each entry has to be node monitored to avoid inode
//  aliasing after a deletion, etc.
//
//  The node preloader knows which icons to preload
#ifndef __NODE_CACHE_PRELOADER__
#define __NODE_CACHE_PRELOADER__


#include <Handler.h>

#include "ObjectList.h"
#include "Model.h"


namespace BPrivate {

class NodePreloader : public BHandler {
public:
	static NodePreloader* InstallNodePreloader(const char* name, BLooper* host);
	virtual ~NodePreloader();

protected:
	NodePreloader(const char* name);
	virtual void MessageReceived(BMessage*);

	void Run();

private:
	void PreloadOne(const char* dirPath);
	virtual void Preload();
		// for now just preload apps and prefs
	Model* FindModel(node_ref) const;

	BObjectList<Model> fModelList;
	Benaphore fLock;
	volatile bool fQuitRequested;

	typedef BHandler _inherited;
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// __NODE_CACHE_PRELOADER__
