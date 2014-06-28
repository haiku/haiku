/*
 * Copyright (c) 2004 Matthijs Hollemans
 * Copyright (c) 2008-2014 Haiku, Inc. All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef _SCOPE_VIEW_H
#define _SCOPE_VIEW_H


#include <View.h>


class ScopeView : public BView {
public:
	ScopeView();
	virtual ~ScopeView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect updateRect);

	void SetPlaying(bool playing);
	void SetEnabled(bool enabled);
	void SetHaveFile(bool haveFile);
	void SetLoading(bool loading);
	void SetLiveInput(bool liveInput);

private:
	typedef BView super;

	static int32 _Thread(void* data);
	int32 Thread();

	void DrawLoading();
	void DrawNoFile();
	void DrawDisabled();
	void DrawStopped();
	void DrawPlaying();

	void DrawText(const char* text);

	bool fIsFinished;
	bool fIsPlaying;
	bool fIsEnabled;
	bool fHaveFile;
	bool fIsLoading;
	bool fIsLiveInput;

	int32 fSampleCount;
	int16* fLeftSamples;
	int16* fRightSamples;

	thread_id fScopeThreadID;
};


inline void
ScopeView::SetPlaying(bool playing)
{
	fIsPlaying = playing;
}


inline void
ScopeView::SetEnabled(bool enabled)
{
	fIsEnabled = enabled;
}


inline void
ScopeView::SetHaveFile(bool haveFile)
{
	fHaveFile = haveFile;
}


inline void
ScopeView::SetLoading(bool loading)
{
	fIsLoading = loading;
}


inline void
ScopeView::SetLiveInput(bool liveInput)
{
	fIsLiveInput = liveInput;
}


#endif // _SCOPE_VIEW_H
