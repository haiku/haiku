//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RootLayer.h
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------
#ifndef _ROOTLAYER_H_
#define _ROOTLAYER_H_

class DisplayDriver;
class RGBColor;
//class Layer;

/*!
	\class RootLayer RootLayer.h
	\brief Class used for the top layer of each workspace's Layer tree
	
	RootLayers are used to head up the top of each Layer tree and reimplement certain 
	Layer functions to act accordingly. There is only one for each workspace class.
	
*/
class RootLayer : public Layer
{
public:
									RootLayer(BRect rect, const char *layername,
										DisplayDriver *gfxdriver);
	virtual							~RootLayer();

	virtual	void					MoveBy(float x, float y);
	virtual	void					ResizeBy(float x, float y);

			void					SetDriver(DisplayDriver *driver);

			void					SetColor(const RGBColor &col);
			RGBColor				GetColor(void) const;
private:

};

#endif
