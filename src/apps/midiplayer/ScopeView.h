/*
 * Copyright (c) 2004 Matthijs Hollemans
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

#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#include <View.h>

class ScopeView : public BView
{
public:

	ScopeView();
	virtual ~ScopeView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect updateRect);

	void SetPlaying(bool flag);
	void SetEnabled(bool flag);
	void SetHaveFile(bool flag);
	void SetLoading(bool flag);
	void SetLiveInput(bool flag);

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

	bool finished;
	bool playing;
	bool enabled;
	bool haveFile;
	bool loading;
	bool liveInput;
	int32 sampleCount;
	int16* leftSamples;
	int16* rightSamples;
	thread_id threadId;
};

#endif // SCOPE_VIEW_H
