//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Layer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#include <OS.h>
#include <View.h>
#include <Message.h>
#include <AppDefs.h>
#include <Region.h>
#include <string.h>
#include <stdio.h>
#include "Layer.h"
#include "RectUtils.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "PortLink.h"
#include "ServerCursor.h"
#include "CursorManager.h"
#include "TokenHandler.h"
#include "RootLayer.h"
#include "DisplayDriver.h"
#include "Desktop.h"
#include "RGBColor.h"

//#define DEBUG_LAYER
#ifdef DEBUG_LAYER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

/*!
	\brief Constructor
	\param frame Size and placement of the Layer
	\param name Name of the layer
	\param resize Resizing flags as defined in View.h
	\param flags BView flags as defined in View.h
	\param win ServerWindow to which the Layer belongs
*/
Layer::Layer(BRect frame, const char *name, int32 token, uint32 resize,
				uint32 flags, ServerWindow *win)
{
	// frame is in _parent coordinates
	if(frame.IsValid())
		_frame		= frame;
	else
		// TODO: Decorator class should have a method witch returns the minimum frame width.
		_frame.Set(0.0f, 0.0f, 5.0f, 5.0f);

	_boundsLeftTop.Set( 0.0f, 0.0f );

	_name			= new BString(name);

	// Layer does not start out as a part of the tree
	_parent			= NULL;
	_uppersibling	= NULL;
	_lowersibling	= NULL;
	_topchild		= NULL;
	_bottomchild	= NULL;
	fRootLayer		= NULL;

	/* NOW all regions (_visible, _fullVisible, _full) are empty */
	
	_flags			= flags;
	_resize_mode	= resize;
	_hidden			= false;
	_is_updating	= false;
	_level			= 0;
	_view_token		= token;
	_layerdata		= new LayerData;
	
	_serverwin		= win;
	_cursor			= NULL;
	clipToPicture	= NULL;
// TODO: modify!
	fDriver			= desktop->GetDisplayDriver();
}

//! Destructor frees all allocated heap space
Layer::~Layer(void)
{
	if(_name)
	{
		delete _name;
		_name = NULL;
	}
	if(_layerdata)
	{
		delete _layerdata;
		_layerdata = NULL;
	}
	if (clipToPicture)
	{
		delete clipToPicture;
		clipToPicture = NULL;
// TODO: allocate and relase a ServerPicture Object.
	}
}

/*!
	\brief Adds a child to the back of the a layer's child stack.
	\param layer The layer to add as a child
	\param before Add the child in front of this layer
	\param rebuild Flag to fully rebuild all visibility regions
*/
void Layer::AddChild(Layer *layer, RootLayer *rootLayer)
{
	if( layer->_parent != NULL ) {
		printf("ERROR: AddChild(): Layer already has a _parent\n");
		return;
	}

	if (layer->GetRootLayer()){
		printf("Error: Layer already belongs to a RootLayer!\n");
		return;
	}
	
	layer->fRootLayer	= rootLayer;

		// attach layer to the tree structure
	layer->_parent		= this;
	if( _bottomchild ){
		layer->_uppersibling		= _bottomchild;
		_bottomchild->_lowersibling	= layer;
	}
	else{
		_topchild		= layer;
	}
	_bottomchild	= layer;


	layer->RebuildFullRegion();

	if ( !(layer->_hidden) ){
			// _layer->_full.Frame() is the maximum(layer's PoV), yet minimum(parent's PoV)
			// region to be rebuilt.
		RebuildChildRegions( layer->_full.Frame(), layer );

			// REDRAWING CODE:

			// RebuildChildRegion gave us a valid visible region.
		if (layer->_visible.CountRects() > 0){
			layer->Invalidate( layer->_visible );
		}
	}
}

/*!
	\brief Removes a layer from the child stack
	\param layer The layer to remove
	\param rebuild Flag to rebuild all visibility regions
*/
void Layer::RemoveChild(Layer *layer)
{
	if( layer->_parent == NULL ){
		printf("ERROR: RemoveChild(): Layer doesn't have a _parent\n");
		return;
	}
	if( layer->_parent != this ){
		printf("ERROR: RemoveChild(): Layer is not a child of this layer\n");
		return;
	}

	fRootLayer	= NULL;

		// cache some things. The rebuilding/redrawing process will need those.
	Layer*		cachedUpperSibling	= layer->_uppersibling;

	// Take care of _parent
	layer->_parent		= NULL;
	if( _topchild == layer )
		_topchild		= layer->_lowersibling;
	if( _bottomchild == layer )
		_bottomchild	= layer->_uppersibling;

	// Take care of siblings
	if( layer->_uppersibling != NULL )
		layer->_uppersibling->_lowersibling	= layer->_lowersibling;
	if( layer->_lowersibling != NULL )
		layer->_lowersibling->_uppersibling = layer->_uppersibling;
	layer->_uppersibling	= NULL;
	layer->_lowersibling	= NULL;

		// eliminate layer's full visible region
	if ( !(layer->_hidden) )
	{
		bool		invalidate = false;
		BRect		invalidRect;

		if (layer->_fullVisible.CountRects() > 0){
			invalidate		= true;
			invalidRect		= layer->_fullVisible.Frame();
			RebuildChildRegions( layer->_fullVisible.Frame(), cachedUpperSibling );
		}

			// REDRAWING CODE:

		if (invalidate){
			DoInvalidate( BRegion(invalidRect), cachedUpperSibling );
		}
	}
}

/*!
	\brief Removes the layer from its parent's child stack
	\param rebuild Flag to rebuild visibility regions
*/
void Layer::RemoveSelf()
{
	// A Layer removes itself from the tree (duh)
	if( _parent == NULL ){
		printf("ERROR: RemoveSelf(): Layer doesn't have a _parent\n");
		return;
	}
	_parent->RemoveChild(this);
}

bool Layer::HasChild(Layer* layer) const{
	for(Layer *lay = VirtualTopChild(); lay; lay = lay->VirtualLowerSibling()){
		if(lay == layer)
			return true;
	}
	return false;
}

/*!
	\brief Finds the first child at a given point.
	\param pt Point to look for a child
	\return non-NULL if found, NULL if not

	Find out which child gets hit if we click at a certain spot. Returns NULL
	if there are no _visible children or if the click does not hit a child layer
*/
Layer* Layer::LayerAt(const BPoint &pt)
{
	if (_visible.Contains(pt)){
		return this;
	}

	if (_fullVisible.Contains(pt)){
		Layer		*lay = NULL;
		for ( Layer* child = _bottomchild; child != NULL; child = child->_uppersibling ){
			if ( (lay = child->LayerAt( pt )) )
				return lay;
		}
	}

	return NULL;
}

/*!
	\brief Returns the size of the layer
	\return the size of the layer
*/
BRect Layer::Bounds(void) const
{
	BRect		r(_frame);
	r.OffsetTo( _boundsLeftTop );
	return r;
}

/*!
	\brief Returns the layer's size and position in its parent coordinates
	\return The layer's size and position in its parent coordinates
*/
BRect Layer::Frame(void) const
{
	return _frame;
}

/*!
	\brief recursively deletes all children (and grandchildren, etc) of the layer

	This is mostly used for server shutdown or deleting a workspace
*/
void Layer::PruneTree(void)
{
	Layer		*lay,
				*nextlay;

	lay			= _topchild;
	_topchild	= NULL;
	
	while(lay != NULL)
	{
		if(lay->_topchild != NULL) {
			lay->PruneTree();
		}
		nextlay				= lay->_lowersibling;
		lay->_lowersibling	= NULL;

		delete lay;
		lay					= nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
}

/*!
	\brief Finds a layer based on its token ID
	\return non-NULL if found, NULL if not
*/
Layer* Layer::FindLayer(const int32 token)
{
	// recursive search for a layer based on its view token
	Layer		*lay,
				*trylay;

	// Search child layers first
	for(lay = _topchild; lay != NULL; lay = lay->_lowersibling)
	{
		if(lay->_view_token == token)
			return lay;
	}
	
	// Hmmm... not in this layer's children. Try lower descendants
	for(lay = _topchild; lay != NULL; lay = lay->_lowersibling)
	{
		trylay		= lay->FindLayer(token);
		if(trylay)
			return trylay;
	}
	
	// Well, we got this far in the function, so apparently there is no match to be found
	return NULL;
}

/*!
	\brief Sets the layer's personal cursor. See BView::SetViewCursor
	\return The cursor associated with this layer.
		
*/
void Layer::SetLayerCursor(ServerCursor *csr)
{
	_cursor		= csr;
}

/*!
	\brief Returns the layer's personal cursor. See BView::SetViewCursor
	\return The cursor associated with this layer.
	
*/
ServerCursor *Layer::GetLayerCursor(void) const
{
	return _cursor;
}

/*!
	\brief Hook function for handling pointer transitions, much like BView::MouseMoved
	\param transit The type of event which occurred.

	The default version of this function changes the cursor to the view's cursor, if there 
	is one. If there isn't one, the default cursor is chosen
*/
void Layer::MouseTransit(uint32 transit)
{
	if(transit == B_ENTERED_VIEW)
	{
		if(_cursor)
			cursormanager->SetCursor(_cursor->ID());
		else
		{
			if(_serverwin)
				_serverwin->App()->SetAppCursor();
			else
				cursormanager->SetCursor(B_CURSOR_DEFAULT);
		}
	}
}

void Layer::DoInvalidate(const BRegion &reg, Layer *start){
//TODO: REMOVE this! For Test purposes only!
	STRACE(("**********Layer::DoInvalidate()\n"));
	if (GetRootLayer())
		GetRootLayer()->Draw(GetRootLayer()->Bounds());
	else{
		if(dynamic_cast<RootLayer*>(this))
			Draw(Bounds());
	}
	return;
//----------------

	if (_hidden || !(_fullVisible.Intersects(reg.Frame())) ){
		printf("SERVER: WARNING: Layer(%s)::DoInvalidate() 1 \n\n", _name->String());
		return;
	}

	// we're here! that means that our view is not hidden, HAS a valid '_fullVisible' region,
	// AND, of course, it intersects the update region.

		// redraw one of our regions if needed.
	if ( _visible.CountRects() > 0 ){
		BRegion			iReg(_visible);

		iReg.IntersectWith( &reg );
		if ( iReg.CountRects() > 0 ){
			if (_serverwin){
				RequestClientUpdate( iReg.Frame() );
			}
			else{
				RequestDraw( iReg.Frame() );
			}
		}
	}

	if (start == NULL){
		printf("SERVER: WARNING: Layer(%s)::DoInvalidate() 2 \n\n", _name->String());
		return;
	}

		// redraw parts of our children, if needed.
	bool			redraw = false;

		// Redraw regions for children... if needed!
	for(Layer *lay = _bottomchild; lay != NULL; lay = lay->_uppersibling)
	{
			// for layers in front of 'start' no redraw is needed.
			// so avoid unnecessary CPU cycles.
		if ( lay == start && redraw == false)
			redraw = true;

		if ( redraw && !(lay->_hidden) && (lay->_fullVisible.CountRects() > 0) ){
			BRegion			layReg(lay->_fullVisible);

			layReg.IntersectWith( &reg );
			if ( layReg.CountRects() > 0 ){
				if (lay->_serverwin){
					lay->RequestClientUpdate( layReg.Frame() );
				}
				else{
					lay->RequestDraw( layReg.Frame() );
				}
			}
		}
	}
}

/*!
	\brief Sets a region as invalid and, thus, needing to be drawn
	\param The region to invalidate
	
	All children of the layer also receive this call, so only 1 Invalidate call is 
	needed to set a section as invalid on the screen.
*/
void Layer::Invalidate(const BRegion& region)
{
	if (_parent){
		_parent->DoInvalidate(region, this);
	}
	else{
		DoInvalidate(region, _topchild);
	}
}

/*!
	\brief Sets a rectangle as invalid and, thus, needing to be drawn
	\param The rectangle to invalidate
	
	All children of the layer also receive this call, so only 1 Invalidate call is 
	needed to set a section as invalid on the screen.
*/
void Layer::Invalidate(const BRect &rect)
{
	Invalidate( BRegion(rect) );
}

/*!
	\brief Sends update requests to client(BViews)
	\param The rectangle into witch the client so draw
	
	All children of the layer also receive this call.
*/
void Layer::RequestClientUpdate(const BRect &rect){

	if (_hidden){
		// this layer has nothing visible on screen, so bail out.
		return;
	}

		// this method assumes _fullVisible.CountRects() is positive !!!!!!!!!!!!!

		// clear the area in the low color
		// only the visible area is cleared, because DisplayDriver does the clipping to it.
		// draw background, *only IF* our view color is different to B_TRANSPARENT_COLOR!
	if ( !(_layerdata->viewcolor == RGBColor(B_TRANSPARENT_COLOR)) ) {
		RGBColor tempColor(B_TRANSPARENT_COLOR);
		//_layerdata->lowcolor.SetColor( B_TRANSPARENT_COLOR );

		fDriver->StrokeRect(rect, _layerdata->pensize, tempColor);
	}

	BMessage		msg;

	msg.what		= _UPDATE_;
	msg.AddInt32("_token", _view_token);
	msg.AddRect("_rect", ConvertFromTop(rect) );

	_serverwin->SendMessageToClient( &msg );

	// This layer's BView counterpart will tell its children to redraw.
}

/*!
	\brief Ask internal server layers to do their drawing
	\param r The area that needs to be drawn. In SCREEN COORDINATES!!!
*/
void Layer::RequestDraw(const BRect &r)
{

	if (_visible.CountRects() > 0){
		RGBColor tempColor(B_TRANSPARENT_COLOR);
		//_layerdata->lowcolor.SetColor( B_TRANSPARENT_COLOR );

		fDriver->StrokeRect(r, _layerdata->pensize, tempColor);

			// draw itself.
		Draw(r);
	}

		// tell children to draw.
	for (Layer *lay = _topchild; lay != NULL; lay = lay->_lowersibling){
		if ( !(lay->_hidden) && r.Intersects(lay->_fullVisible.Frame()) ){
			BRect		newRect = (r & lay->_fullVisible.Frame());
			lay->RequestDraw( newRect );
		}
	}
}

/*!
	\brief Ask the layer to draw itself
	\param r The area that needs to be drawn
*/
void Layer::Draw(const BRect &r)
{
	// empty HOOK function.
}

//! Show the layer. Operates just like the BView call with the same name
void Layer::Show(void)
{
	if( !_hidden )
		return;
	
	_hidden		= false;

//TODO: *****!*!*!*!*!*!*!**!***REMOVE this! For Test purposes only!
STRACE(("**********Layer::Show()\n"));
		DoInvalidate(BRegion(Bounds()), NULL);
	return;
//----------------

		// rebuild layer visible region based on its parent's and those supplied by user
	if (_parent){
		_parent->RebuildChildRegions( _full.Frame(), this );
	}
	else{
		RebuildRegions( _full.Frame() );
	}

		// REDRAWING CODE:

	if (_fullVisible.CountRects() > 0)
		Invalidate( _fullVisible );
}

//! Hide the layer. Operates just like the BView call with the same name
void Layer::Hide(void)
{
	if ( _hidden )
		return;

	_hidden			= true;

//TODO: *****!*!*!*!*!*!*!**!***REMOVE this! For Test purposes only!
STRACE(("**********Layer::Hide()\n"));
		DoInvalidate(BRegion(Bounds()), NULL);
	return;
//----------------

	if (_fullVisible.CountRects() > 0){

		BRegion			cachedFullVisible = _fullVisible;

		if (_parent){
			_parent->RebuildChildRegions( _fullVisible.Frame(), this );
		}
		else{
			RebuildRegions( _full.Frame() );
		}

		// REDRAWING CODE:

		Invalidate( cachedFullVisible );
	}
}

/*!
	\brief Determines whether the layer is hidden or not
	\return true if hidden, false if not.
*/
bool Layer::IsHidden(void) const
{
	return _hidden;
}

/*!
	\brief Counts the number of children the layer has
	\return the number of children the layer has, not including grandchildren
*/
uint32 Layer::CountChildren(void) const
{
	uint32 i=0;
	Layer *lay=_topchild;
	while(lay!=NULL)
	{
		lay=lay->_lowersibling;
		i++;
	}
	return i;
}

void Layer::MoveRegionsBy(float x, float y){

		// currently just the full region needs to be moved.
	_full.OffsetBy(x, y);
			
	for (Layer *lay = _topchild; lay != NULL; lay = lay->_lowersibling){
		lay->MoveRegionsBy(x, y);
	}
}

/*!
	\brief Moves a layer in its parent coordinate space
	\param x X offset
	\param y Y offset
*/
void Layer::MoveBy(float x, float y)
{
	BRegion		oldFullVisible( _fullVisible );
//	BPoint		oldFullVisibleOrigin( _fullVisible.Frame().LeftTop() );

	_frame.OffsetBy(x, y);

	MoveRegionsBy(x, y);

	if (_parent){
		if ( !_hidden ){
				// to clear the area occupied on its parent _visible
			if (_fullVisible.CountRects() > 0){
				_hidden		= true;
				_parent->RebuildChildRegions( _fullVisible.Frame(), this );
				_hidden		= false;
			}
			_parent->RebuildChildRegions( _full.Frame(), this );
		}
	}
	else{
		if ( !_hidden )	{
			RebuildRegions( _full.Frame() );
		}
	}

		// send a message to client if requested.
	if(_flags & B_FRAME_EVENTS && _serverwin){
		BMessage		msg;
			// no locking?
		msg.what		= B_VIEW_MOVED;
		msg.AddInt64( "when", real_time_clock_usecs() );
		msg.AddInt32( "_token", _view_token );
		msg.AddPoint( "where", _frame.LeftTop() );
		
		_serverwin->SendMessageToClient( &msg );
	}

		// REDRAWING CODE:

	if ( !(_hidden) )
	{
			/* The region(on screen) that will be invalidated.
			 *	It is composed by:
			 *		the regions that were visible, and now they aren't +
			 *		the regions that are now visible, and they were not visible before.
			 *	(oldFullVisible - _fullVisible) + (_fullVisible - oldFullVisible)
			 */
		BRegion			clipReg;

			// first offset the old region so we can do the correct operations.
		oldFullVisible.OffsetBy(x, y);

			// + (oldFullVisible - _fullVisible)
		if ( oldFullVisible.CountRects() > 0 ){
			BRegion		tempReg( oldFullVisible );
			tempReg.Exclude( &_fullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// + (_fullVisible - oldFullVisible)
		if ( _fullVisible.CountRects() > 0 ){
			BRegion		tempReg( _fullVisible );
			tempReg.Exclude( &oldFullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// there is no point in redrawing what already is visible. So just copy
			// on-screen pixels to layer's new location.
		if ( (oldFullVisible.CountRects() > 0) && (_fullVisible.CountRects() > 0) ){
			BRegion		tempReg( oldFullVisible );
			tempReg.IntersectWith( &_fullVisible );

			if (tempReg.CountRects() > 0){
// TODO: when you have such a method in DisplayDriver/Clipper, uncomment!
				//fDriver->CopyBits( &tempReg, oldFullVisibleOrigin, oldFullVisibleOrigin.OffsetByCopy(x,y) );
			}
		}

			// invalidate 'clipReg' so we can see the results of this move.
		if (clipReg.CountRects() > 0){
			Invalidate( clipReg );
		}
	}
}

void Layer::ResizeRegionsBy(float x, float y){
	_frame.right		= _frame.right + x;
	_frame.bottom		= _frame.bottom + y;

	RebuildFullRegion();

		/* Send messages to BViews only if this Layer belongs to a
		 *	ServerWindow object
		 */
	BMessage		msg;
	if( _serverwin )
	{
		msg.what		= B_VIEW_RESIZED;
		msg.AddInt64( "when", real_time_clock_usecs() );
		msg.AddInt32( "_token", _view_token );
		msg.AddFloat( "width", _frame.Width() );
		msg.AddFloat( "height", _frame.Height() );
			// no need for that... it's here because of backward compatibility
		msg.AddPoint( "where", _frame.LeftTop() );
		
		_serverwin->SendMessageToClient( &msg );
		msg.MakeEmpty();
	}

	Layer		*c = _topchild; //c = short for: current

	if( c != NULL )
		while( true ){
			// action block
			{
				bool		sendMovedToMsg		= false;
				bool		sendResizedToMsg	= false;

				{
					BRect		newFrame	= c->_frame;
					uint32		rmask		= c->_resize_mode;
					if (rmask == B_FOLLOW_NONE)
						rmask		= B_FOLLOW_LEFT | B_FOLLOW_TOP;

						// resize/move horizontaly
					if (rmask & B_FOLLOW_LEFT){
						// nothing to do
					}
					else if (rmask & B_FOLLOW_RIGHT){
							newFrame.OffsetBy(x, 0.0f);
							sendMovedToMsg		= true;
						}
						else if (rmask & B_FOLLOW_LEFT_RIGHT){
								newFrame.SetRightBottom( BPoint(newFrame.right + x, newFrame.bottom) );
								sendResizedToMsg	= true;
							}
							else if (rmask & B_FOLLOW_H_CENTER){
									newFrame.OffsetBy(x/2, 0.0f);
									sendMovedToMsg		= true;
								}
								else { // illegal flag. Do nothing.
								}

						// resize/move verticaly
					if (rmask & B_FOLLOW_TOP){
						// nothing to do
					}
					else if (rmask & B_FOLLOW_BOTTOM){
							newFrame.OffsetBy(0.0f, y);
							sendMovedToMsg		= true;
						}
						else if (rmask & B_FOLLOW_TOP_BOTTOM){
								newFrame.SetRightBottom( BPoint(newFrame.right, newFrame.bottom + y) );
								sendResizedToMsg	= true;
							}
							else if (rmask & B_FOLLOW_V_CENTER){
									newFrame.OffsetBy(0.0f, y/2);
									sendMovedToMsg		= true;
								}
								else { // illegal flag. Do nothing.
								}

					if (sendMovedToMsg && !sendResizedToMsg){
						c->_full.OffsetBy(	(newFrame.left - c->_frame.left),
											(newFrame.top - c->_frame.top) );
					}

					c->_frame		= newFrame;

					if (sendResizedToMsg){
						c->RebuildFullRegion();
					}
				}

					// send message to client it requested (flags & B_FRAME_EVENTS)
				if ( c->_serverwin ){
					if ( sendMovedToMsg && (c->_flags & B_FRAME_EVENTS) ) {
						msg.what		= B_VIEW_MOVED;
						msg.AddInt64( "when", real_time_clock_usecs() );
						msg.AddInt32( "_token", c->_view_token );
						msg.AddPoint( "where", c->_frame.LeftTop() );
						
						c->_serverwin->SendMessageToClient( &msg );
						msg.MakeEmpty();
					}

					if ( sendResizedToMsg && (c->_flags & B_FRAME_EVENTS) ){
						msg.what		= B_VIEW_RESIZED;
						msg.AddInt64( "when", real_time_clock_usecs() );
						msg.AddInt32( "_token", c->_view_token );
						msg.AddFloat( "width", c->_frame.Width() );
						msg.AddFloat( "height", c->_frame.Height() );
							// no need for that... it's here because of backward compatibility
						msg.AddPoint( "where", c->_frame.LeftTop() );
			
						c->_serverwin->SendMessageToClient( &msg );
						msg.MakeEmpty();
					}
				}
			}

			// tree parsing algorithm

				// go deep
			if(	c->_topchild ){
				c = c->_topchild;
			}
				// go right or up
			else
					// go right
				if( c->_lowersibling ){
					c = c->_lowersibling;
				}
					// go up
				else{
					while( !c->_parent->_lowersibling && c->_parent != this ){
						c = c->_parent;
					}
						// that enough! We've reached this view.
					if( c->_parent == this )
						break;
						
					c = c->_parent->_lowersibling;
				}
		}
}

/*!
	\brief Resizes the layer.
	\param x X offset
	\param y Y offset
	
	This resizes the layer itself and resizes any children based on their resize
	flags.
*/
void Layer::ResizeBy(float x, float y)
{
	BRegion		oldFullVisible( _fullVisible );

	ResizeRegionsBy(x, y);

	if (_parent){
		if ( !_hidden ){
			_parent->RebuildChildRegions( _full.Frame(), this );
		}
	}
	else{
		if ( !_hidden ){
			RebuildRegions( _full.Frame() );
		}
	}

		// REDRAWING CODE:

	if ( !(_hidden) )
	{
			/* The region(on screen) that will be invalidated.
			 *	It is composed by:
			 *		the regions that were visible, and now they aren't +
			 *		the regions that are now visible, and they were not visible before.
			 *	(oldFullVisible - _fullVisible) + (_fullVisible - oldFullVisible)
			 */
		BRegion			clipReg;

			// + (oldFullVisible - _fullVisible)
		if ( oldFullVisible.CountRects() > 0 ){
			BRegion		tempReg( oldFullVisible );
			tempReg.Exclude( &_fullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// + (_fullVisible - oldFullVisible)
		if ( _fullVisible.CountRects() > 0 ){
			BRegion		tempReg( _fullVisible );
			tempReg.Exclude( &oldFullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// invalidate 'clipReg' so we can see the results of this resize operation.
		if (clipReg.CountRects() > 0){
			Invalidate( clipReg );
		}
	}
}

void Layer::RebuildChildRegions( const BRect &r, Layer* startFrom ){
		// just in case something goes wrong.
	if ( _hidden ){
		printf("\nSERVER PANIC: Layer(%s)::RebuildChildRegions() - This should NOT happen.\n\n", _name->String());
		return;
	}

		// here is the actual start of this method. :-))
	bool			rebuild = false;

	_visible		= _fullVisible;

		// Rebuild regions for children... if needed!
	for(Layer *lay = _bottomchild; lay != NULL; lay = lay->_uppersibling)
	{
			// for layers in front of 'startLayer' no region rebuild is needed.
			// so avoid unnecessary CPU cycles.
		if ( lay == startFrom && rebuild == false)
			rebuild = true;

		if ( !(lay->_hidden) ){
			if ( !rebuild ){
				// do not rebuild, but exclude lay's FULL visible region from its parent v.r.
				_visible.Exclude( &(lay->_fullVisible) );
			}
			else{
				lay->RebuildRegions( r );
			}
		}
	}
}
/*!
	\brief Rebuilds visibility regions for this and child layers
	\param include_children Flag to rebuild all children and subchildren
*/
void Layer::RebuildRegions( const BRect& r )
{
		// if we're in the rebuild area, rebuild our v.r.
	if ( _full.Intersects( r ) ){
		_visible			= _full;

		if (_parent){
			_visible.IntersectWith( &(_parent->_visible) );
				// exclude from parent's visible area.
			if ( !(_hidden) )
				_parent->_visible.Exclude( &(_visible) );
		}

		_fullVisible		= _visible;
	}
	else{
			// our visible region will stay the same

			// exclude our FULL visible region from parent's visible region.
		if ( !(_hidden) && _parent )
			_parent->_visible.Exclude( &(_fullVisible) );

			// we're not in the rebuild area so our children's v.r.s are OK. 
		return;
	}

		// Rebuild regions for children...
	for(Layer *lay = _bottomchild; lay != NULL; lay = lay->_uppersibling)
	{
		if ( !(lay->_hidden) ){
			lay->RebuildRegions( r );
		}
	}
}

void Layer::RebuildFullRegion( ){
// TODO: temporary patch?
	if (_parent && dynamic_cast<RootLayer*>(_parent) ){
	}
	else{
		_full.Set( ConvertToTop( _frame ) );
	}

	LayerData		*ld;

	ld			= _layerdata;
	do{
			// clip to user region
		if ( ld->clippReg )
			_full.IntersectWith( ld->clippReg );
	} while( (ld = ld->prevState) );

		// clip to user picture region
	if ( clipToPicture )
		if ( clipToPictureInverse ) {
			_full.Exclude( clipToPicture );
		}
		else{
			_full.IntersectWith( clipToPicture );
		}
}

//! Prints all relevant layer data to stdout
void Layer::PrintToStream(void)
{
	printf("-----------\nLayer %s\n",_name->String());
	if(_parent)
		printf("Parent: %s (%p)\n",_parent->_name->String(), _parent);
	else
		printf("Parent: NULL\n");
	if(_uppersibling)
		printf("Upper sibling: %s (%p)\n",_uppersibling->_name->String(), _uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(_lowersibling)
		printf("Lower sibling: %s (%p)\n",_lowersibling->_name->String(), _lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(_topchild)
		printf("Top child: %s (%p)\n",_topchild->_name->String(), _topchild);
	else
		printf("Top child: NULL\n");
	if(_bottomchild)
		printf("Bottom child: %s (%p)\n",_bottomchild->_name->String(), _bottomchild);
	else
		printf("Bottom child: NULL\n");
	printf("Frame: "); _frame.PrintToStream();
	printf("Token: %ld\n",_view_token);
	printf("Hide count: %s\n",_hidden?"true":"false");
	printf("Visible Areas: "); _visible.PrintToStream();
	printf("Is updating = %s\n",(_is_updating)?"yes":"no");
}

//! Prints hierarchy data to stdout
void Layer::PrintNode(void)
{
	printf("-----------\nLayer %s\n",_name->String());
	if(_parent)
		printf("Parent: %s (%p)\n",_parent->_name->String(), _parent);
	else
		printf("Parent: NULL\n");
	if(_uppersibling)
		printf("Upper sibling: %s (%p)\n",_uppersibling->_name->String(), _uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(_lowersibling)
		printf("Lower sibling: %s (%p)\n",_lowersibling->_name->String(), _lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(_topchild)
		printf("Top child: %s (%p)\n",_topchild->_name->String(), _topchild);
	else
		printf("Top child: NULL\n");
	if(_bottomchild)
		printf("Bottom child: %s (%p)\n",_bottomchild->_name->String(), _bottomchild);
	else
		printf("Bottom child: NULL\n");
	printf("Visible Areas: "); _visible.PrintToStream();
}

//! Prints the tree structure starting from this layer
void Layer::PrintTree(){

	int32		spaces = 2;
	Layer		*c = _topchild; //c = short for: current
	printf( "'%s' - token: %ld\n", _name->String(), _view_token );
	if( c != NULL )
		while( true ){
			// action block
			{
				for( int i = 0; i < spaces; i++)
					printf(" ");
				
					printf( "'%s' - token: %ld\n", c->_name->String(), c->_view_token );
			}

				// go deep
			if(	c->_topchild ){
				c = c->_topchild;
				spaces += 2;
			}
				// go right or up
			else
					// go right
				if( c->_lowersibling ){
					c = c->_lowersibling;
				}
					// go up
				else{
					while( !c->_parent->_lowersibling && c->_parent != this ){
						c = c->_parent;
						spaces -= 2;
					}
						// that enough! We've reached this view.
					if( c->_parent == this )
						break;
						
					c = c->_parent->_lowersibling;
					spaces -= 2;
				}
		}
}

/*!
	\brief Converts the rectangle to the layer's parent coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertToParent(BRect rect)
{
	return (rect.OffsetByCopy(_frame.LeftTop()));
}

/*!
	\brief Converts the region to the layer's parent coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertToParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects(); i++)
		newreg.Include( (reg->RectAt(i)).OffsetByCopy(_frame.LeftTop()) );
	return newreg;
}

/*!
	\brief Converts the rectangle from the layer's parent coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertFromParent(BRect rect)
{
	return (rect.OffsetByCopy(_frame.left*-1,_frame.top*-1));
}

/*!
	\brief Converts the region from the layer's parent coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertFromParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include((reg->RectAt(i)).OffsetByCopy(_frame.left*-1,_frame.top*-1));
	return newreg;
}

/*!
	\brief Converts the region to screen coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertToTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertToTop(reg->RectAt(i)));
	return newreg;
}

/*!
	\brief Converts the rectangle to screen coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertToTop(BRect rect)
{
	if (_parent!=NULL)
		return(_parent->ConvertToTop(rect.OffsetByCopy(_frame.LeftTop())) );
	else
		return(rect);
}

/*!
	\brief Converts the region from screen coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertFromTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromTop(reg->RectAt(i)));
	return newreg;
}

/*!
	\brief Converts the rectangle from screen coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertFromTop(BRect rect)
{
	if (_parent!=NULL)
		return(_parent->ConvertFromTop(rect.OffsetByCopy(_frame.LeftTop().x*-1,
			_frame.LeftTop().y*-1)) );
	else
		return(rect);
}

RootLayer* Layer::GetRootLayer() const{
	return fRootLayer;
}

void Layer::SetRootLayer(RootLayer* rl){
	fRootLayer	= rl;
}


/*
 @log
*/
