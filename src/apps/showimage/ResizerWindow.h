/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		yellowTAB GmbH
 *		Bernd Korz
 *      Michael Pfeiffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _resizer_window_h
#define _resizer_window_h

#include <Window.h>
#include <View.h>


class BTextControl;
class BCheckBox;

class ResizerWindow : public BWindow
{
   public:			
						ResizerWindow(BMessenger target, float width, float height );
					
		virtual void	MessageReceived(BMessage* msg);
		virtual	bool	QuitRequested();
		
		virtual			~ResizerWindow();

		// the public message command constants ('what')
		enum {
			kActivateMsg = 'RSRa',
				// activates the window
			kUpdateMsg,
				// provides the new size of the image in two float fields "width" and "height"
		};
   private:
		enum {
			kResolutionMsg = 'Rszr',
			kWidthModifiedMsg,
			kHeightModifiedMsg,
			kApplyMsg,
		};
		
		void AddControl(BView* parent, BControl* control, float column2, BRect& rect);
		void AddSeparatorLine(BView* parent, BRect& rect);
		void LeftAlign(BControl* control);
		
   		BTextControl*	fWidth;
   		BTextControl*	fHeight;
   		BCheckBox*		fAspectRatio;
   		BButton*		fApply;
   		float			fRatio;
   		BMessenger		fTarget;
};

#endif
