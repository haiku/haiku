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
//	File Name:		RootLayer.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------
#include <View.h>
#include "DisplayDriver.h"
#include "Layer.h"
#include "RootLayer.h"
#include "Desktop.h"
#include "PatternHandler.h" // for pattern_union
#include "ServerConfig.h"
#include "TokenHandler.h"
#include <TokenSpace.h>
#include <stdio.h>

//! TokenHandler object used to provide IDs for RootLayers(WorkSpaces)
TokenHandler rlayer_token_handler;

//#define DEBUG_ROOTLAYER

/*!
	\brief Sets up internal variables needed by the RootLayer
	\param rect Frame of the root layer
	\param layername Name of the root layer. Not really used.
	\param gfxdriver Pointer to the related graphics driver
*/
RootLayer::RootLayer(BRect rect, const char *layername, DisplayDriver *gfxdriver)
	: Layer(rect, layername, B_NULL_TOKEN, B_FOLLOW_NONE, 0, NULL)
{
	_view_token		= rlayer_token_handler.GetToken();

	fDriver			= gfxdriver;

	_hidden			= false;
}

//! Frees all allocated heap memory (which happens to be none) ;)
RootLayer::~RootLayer()
{
}

/*!
	\brief Sets the background color of the Screen
	\param col The new background color
*/
void RootLayer::SetColor(const RGBColor &col)
{
	_layerdata->viewcolor	= col;
}

/*!
	\brief Returns the background color of the Screen
	\return The background color
*/
RGBColor RootLayer::GetColor(void) const
{	
	return _layerdata->viewcolor;
}

//! Empty function to disable moving the RootLayer
void RootLayer::MoveBy(float x, float y)
{
}

//! Reimplemented for RootLayer special case
void RootLayer::ResizeBy(float x, float y)
{
	_frame.right	+= x;
	_frame.bottom	+= y;

	_full.Set( _frame ); // NO ConvertTopTop !!!

		// We need to rebuild ALL regions because the screen is black now,
		// and ALL views/Layers need redrawing.
	RebuildRegions( _frame );

		// Invalidate the hole screen, because we've changed resolution
		// and the screen is black.
	Invalidate( _frame );
}

/*!
	\brief Assigns a particular display driver to the RootLayer
	\param d The new display driver (ignored if NULL)
*/
void RootLayer::SetDriver(DisplayDriver *driver)
{
	fDriver		= driver;
}
