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
//	File Name:		View.cpp
//	Author:			Adrian Oanca <adioanca@myrealbox.com>
//	Description:   A BView object represents a rectangular area within a window.
//					The object draws within this rectangle and responds to user
//					events that are directed at the window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Font.h>
#include <ScrollBar.h>
#include <Cursor.h>
#include <Bitmap.h>
#include <Picture.h>
#include <Polygon.h>
#include <Shape.h>
#include <Button.h>
#include <Shelf.h>
#include <MenuBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <Application.h>

// Project Includes ------------------------------------------------------------
#include <AppMisc.h>
#include <ViewAux.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <ColorUtils.h>
#include <AppServerLink.h>
#include <PortLink.h>
#include <ServerProtocol.h>

// Local Includes --------------------------------------------------------------
#include <stdio.h>

// Local Defines ---------------------------------------------------------------

//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_BVIEW
#	define BVTRACE PrintToStream()
#else
#	define BVTRACE ;
#endif

inline rgb_color _get_rgb_color( uint32 color );
inline uint32 _get_uint32_color( rgb_color c );
inline rgb_color _set_static_rgb_color( uint8 r, uint8 g, uint8 b, uint8 a=255 );
inline void _set_ptr_rgb_color( rgb_color* c, uint8 r, uint8 g, uint8 b, uint8 a=255 );
inline bool _rgb_color_are_equal( rgb_color c1, rgb_color c2 );
inline bool _is_new_pattern( const pattern& p1, const pattern& p2 );

// Globals ---------------------------------------------------------------------
static property_info viewPropInfo[] =
{
	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the view's frame rectangle.",0 },

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the view's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the view is hidden; false otherwise.",0 },

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the view.",0 },

	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the shelf.",0 },

	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of of child views.",0 },

	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_REVERSE_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_NAME_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 


// General Functions
//------------------------------------------------------------------------------

// Constructors
//------------------------------------------------------------------------------

BView::BView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BHandler( name )
{
	InitData( frame, name, resizingMode, flags );
}

//---------------------------------------------------------------------------

BView::BView(BMessage *archive)
	: BHandler( archive )
{
	uint32				resizingMode;
	uint32				flags;
	BRect				frame;

	archive->FindRect("_frame", &frame);
	if ( !(archive->FindInt32("_resize_mode", (int32*)&resizingMode) == B_OK) )
		resizingMode	= 0;

	if ( !(archive->FindInt32("_flags", (int32*)&flags) == B_OK) )
		flags			= 0;

	InitData( frame, Name(), resizingMode, flags );

	font_family			family;
	font_style			style;
	float				size;
	float				shear;
	float				rotation;
	if ( archive->FindString("_fname", 0, (const char **)&family) == B_OK){

		archive->FindString("_fname", 1, (const char **)&style);
		archive->FindFloat("_fflt", 0, &size);
		archive->FindFloat("_fflt", 1, &shear);
		archive->FindFloat("_fflt", 2, &rotation);

		BFont			font;

		font.SetSize( size );
		font.SetShear( shear );
		font.SetRotation( rotation );
		font.SetFamilyAndStyle( family, style );

		SetFont( &font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE |
						B_FONT_SHEAR | B_FONT_ROTATION );
	}

	uint32				highColor;
	uint32				lowColor;
	uint32				viewColor;
	if ( archive->FindInt32("_color", 0, (int32*)&highColor) == B_OK ){
		archive->FindInt32("_color", 1, (int32*)&lowColor);
		archive->FindInt32("_color", 2, (int32*)&viewColor);

		SetHighColor( _get_rgb_color( highColor) );
		SetLowColor( _get_rgb_color( lowColor ) );
		SetViewColor( _get_rgb_color( viewColor ) );
	}

	uint32				evMask;
	uint32				options;
	if ( archive->FindInt32("_evmask", 0, (int32*)&evMask) == B_OK ){
		archive->FindInt32("_evmask", 1, (int32*)&options);

		SetEventMask( evMask, options );
	}

	BPoint				origin;
	if ( archive->FindPoint("_origin", &origin) == B_OK)
		SetOrigin( origin );

	float				penSize;

	if ( archive->FindFloat("_psize", &penSize) == B_OK )
		SetPenSize( penSize );

	BPoint				penLocation;
	if ( archive->FindPoint("_ploc", &penLocation) == B_OK )
		MovePenTo( penLocation );

	int16				lineCap;
	int16				lineJoin;
	float				lineMiter;
	if ( archive->FindInt16("_lmcapjoin", 0, &lineCap) == B_OK){
		archive->FindInt16("_lmcapjoin", 1, &lineJoin);
		archive->FindFloat("_lmmiter", &lineMiter);

		SetLineMode( (cap_mode)lineCap, (join_mode)lineJoin, lineMiter );
	}

	int16				alphaBlend;
	int16				modeBlend;
	if ( archive->FindInt16("_blend", 0, &alphaBlend) == B_OK ){
		archive->FindInt16("_blend", 1, &modeBlend);

		SetBlendingMode( (source_alpha)alphaBlend, (alpha_function)modeBlend );
	}

	uint32				drawingMode;
	if ( archive->FindInt32("_dmod", (int32*)&drawingMode) == B_OK )
		SetDrawingMode( (drawing_mode)drawingMode );

	BMessage			msg;
	BArchivable			*obj;
	int					i = 0;
	while ( archive->FindMessage("_views", i++, &msg) == B_OK){ 
		obj			= instantiate_object(&msg);

		BView		*child;
		child		= dynamic_cast<BView *>(obj);
		if (child)
			AddChild( child ); 
	}
}

//---------------------------------------------------------------------------

BArchivable* BView::Instantiate(BMessage* data)
{
   if ( !validate_instantiation( data , "BView" ) ) 
      return NULL; 
   return new BView(data); 
}

//---------------------------------------------------------------------------

status_t BView::Archive(BMessage* data, bool deep) const
{
	status_t		retval;

	retval		= BHandler::Archive( data, deep );
	if (retval != B_OK)
		return retval;

	if ( fState->archivingFlags & B_VIEW_COORD_BIT )
		data->AddRect("_frame", Bounds().OffsetToCopy( originX, originY ) );		

	if ( fState->archivingFlags & B_VIEW_RESIZE_BIT )
		data->AddInt32("_resize_mode", ResizingMode() );

	if ( fState->archivingFlags & B_VIEW_FLAGS_BIT )
		data->AddInt32("_flags", Flags() );

	if ( fState->archivingFlags & B_VIEW_EVMASK_BIT ) {
		data->AddInt32("_evmask", fEventMask );
		data->AddInt32("_evmask", fEventOptions );
	}

	if ( fState->archivingFlags & B_VIEW_FONT_BIT ){
		BFont			font;
		font_family		family;
		font_style		style;

		GetFont( &font );

		font.GetFamilyAndStyle( &family, &style );
		data->AddString("_fname", family);
		data->AddString("_fname", style);

		data->AddFloat("_fflt", font.Size());
		data->AddFloat("_fflt", font.Shear());
		data->AddFloat("_fflt", font.Rotation());
	}

	if ( fState->archivingFlags & B_VIEW_COLORS_BIT ){
		data->AddInt32("_color", _get_uint32_color(HighColor()) );
		data->AddInt32("_color", _get_uint32_color(LowColor()) );
		data->AddInt32("_color", _get_uint32_color(ViewColor()) );
	}

//	NOTE: we do not use this flag any more
//	if ( 1 ){
//		data->AddInt32("_dbuf", 1);
//	}

	if ( fState->archivingFlags & B_VIEW_ORIGIN_BIT )
		data->AddPoint("_origin", Origin());

	if ( fState->archivingFlags & B_VIEW_PEN_SIZE_BIT )
		data->AddFloat("_psize", PenSize());

	if ( fState->archivingFlags & B_VIEW_PEN_LOC_BIT )
		data->AddPoint("_ploc", PenLocation());

	if ( fState->archivingFlags & B_VIEW_LINE_MODES_BIT )
	{
		data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if ( fState->archivingFlags & B_VIEW_BLENDING_BIT )
	{
		source_alpha		alphaSrcMode;
		alpha_function		alphaFncMode;
		GetBlendingMode( &alphaSrcMode, &alphaFncMode);

		data->AddInt16("_blend", (int16)alphaSrcMode);
		data->AddInt16("_blend", (int16)alphaFncMode);
	}

	if ( fState->archivingFlags & B_VIEW_DRAW_MODE_BIT )
		data->AddInt32("_dmod", DrawingMode());

	if (deep)
	{
		int					i		= 0;
		BView				*child	= NULL;

		while ( (child = ChildAt(i++)) != NULL)
		{
			BMessage		childArchive;
			
			retval			= child->Archive( &childArchive, deep );
			if (retval == B_OK)
				data->AddMessage( "_views", &childArchive );
		}
	}
	
	return retval;
}

//---------------------------------------------------------------------------

BView::~BView()
{
STRACE(("BView(%s)::~BView()\n", this->Name()));
	if (owner)
		debugger("Trying to delete a view that belongs to a window. Call RemoveSelf first.");

	removeSelf();

	// TODO: see about BShelf! must I delete it here? is it deleted by the window?
	
	// we also delete all its childern
	BView		*child, *_child;
	
	child		= first_child;
	while(child)
	{
		_child		= child;
		child		= child->next_sibling;
		deleteView( _child );
	}
	
	if (fVerScroller)
		fVerScroller->SetTarget( (const char*)NULL );
	
	if (fHorScroller)
		fHorScroller->SetTarget( (const char*)NULL );

	SetName( NULL );

	if (fPermanentState)
		delete fPermanentState;
	if (fState)
		delete fState;

	if(pr_state)
		free( pr_state );
	pr_state	= NULL;
}

//---------------------------------------------------------------------------

BRect BView::Bounds() const
{
	// if we have the actual coordiantes
	if (fState->flags & B_VIEW_COORD_BIT)
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_GET_COORD );
		owner->fLink->Flush();
		
		int32 rCode=SERVER_FALSE;
		owner->fLink->GetNextReply(&rCode);
		if(rCode==SERVER_TRUE)
		{
			owner->fLink->Read<float>( const_cast<float*>(&originX) );
			owner->fLink->Read<float>( const_cast<float*>(&originY) );
			owner->fLink->Read<BRect>( const_cast<BRect*>(&fBounds) );
			fState->flags		&= ~B_VIEW_COORD_BIT;
		}
	}

	return fBounds;
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BPoint* pt) const
{
	if (!parent)
		return;

	check_lock_no_pick();

	pt->x			+= originX;
	pt->y			+= originY;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToParent(BPoint pt) const
{
	if (!parent)
		return pt;

	check_lock_no_pick();
	
	BPoint			p;
	p.x				= pt.x + originX;
	p.y				= pt.y + originY;

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BPoint* pt) const
{
	if (!parent)
		return;

	check_lock_no_pick();
	
	pt->x			-= originX;
	pt->y			-= originY;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromParent(BPoint pt) const
{
	if (!parent)
		return pt;

	check_lock_no_pick();
	
	BPoint			p;
	p.x				= pt.x - originX;
	p.y				= pt.y - originY;

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BRect* r) const
{
	if (!parent)
		return;

	check_lock_no_pick();
	
	r->OffsetBy(originX, originY);
}

//---------------------------------------------------------------------------

BRect BView::ConvertToParent(BRect r) const
{
	if (!parent)
		return r;

	check_lock_no_pick();
	
	return r.OffsetByCopy(originX, originY);
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BRect* r) const
{
	if (!parent)
		return;

	check_lock_no_pick();
	
	r->OffsetBy(-originX, -originY);
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromParent(BRect r) const
{
	if (!parent)
		return r;

	check_lock_no_pick();

	return r.OffsetByCopy(-originX, -originY);
}

//---------------------------------------------------------------------------



void BView::ConvertToScreen(BPoint* pt) const
{
	if (!parent)
		return;

	do_owner_check_no_pick();

	ConvertToParent( pt );
	parent->ConvertToScreen( pt );
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToScreen(BPoint pt) const
{
	if (!parent)
		return pt;

	do_owner_check_no_pick();

	BPoint	p;

	p		= ConvertToParent( pt );
	p		= parent->ConvertToScreen( p );

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BPoint* pt) const
{
	if (!parent)
	{
		if(owner)
			return owner->ConvertFromScreen(pt);
		
		return;
	}

	do_owner_check_no_pick();

	ConvertFromParent( pt );
	parent->ConvertFromScreen( pt );
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromScreen(BPoint pt) const
{
	if (!parent)
	{
		if(owner)
			return owner->ConvertFromScreen(pt);
		
		return pt;
	}
	
	do_owner_check_no_pick();

	BPoint	p;

	p		= ConvertFromParent( pt );
	p		= parent->ConvertFromScreen( p );

	return p;
}

//---------------------------------------------------------------------------


void BView::ConvertToScreen(BRect* r) const
{
	if (!parent)
		return;

	do_owner_check_no_pick();

	ConvertToParent( r );
	parent->ConvertToScreen( r );
}

//---------------------------------------------------------------------------

BRect BView::ConvertToScreen(BRect r) const
{
	if (!parent)
		return r;

	do_owner_check_no_pick();

	BRect	rect;

	rect	= ConvertToParent( r );
	rect	= parent->ConvertToScreen( rect );

	return rect;
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BRect* r) const
{
	if (!parent)
		return;

	do_owner_check_no_pick();

	ConvertFromParent( r );
	parent->ConvertFromScreen( r );
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromScreen(BRect r) const
{
	if (!parent)
		return r;

	do_owner_check_no_pick();

	BRect	rect;

	rect	= ConvertFromParent( r );
	rect	= parent->ConvertFromScreen( rect );

	return rect;
}

//---------------------------------------------------------------------------

uint32 BView::Flags() const 
{
	check_lock_no_pick();
	return ( fFlags & ~_RESIZE_MASK_ );
}

//---------------------------------------------------------------------------

void BView::SetFlags( uint32 flags )
{
	if (Flags() == flags)
		return;
		
	if (owner)
	{
		if ( flags & B_PULSE_NEEDED )
		{
			check_lock_no_pick();
			if ( !owner->fPulseEnabled )
				owner->SetPulseRate( 500000 );
		}
			
		if ( flags & (	B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE |
						B_FRAME_EVENTS | B_SUBPIXEL_PRECISE ) )
		{
			check_lock();
			
			owner->fLink->StartMessage( AS_LAYER_SET_FLAGS );
			owner->fLink->Attach<uint32>( flags );
		}
	}
			
/*	Some useful info:
		fFlags is a unsigned long (32 bits)
		* bits 1-16 are used for BView's flags
		* bits 17-32 are used for BView' resize mask
		* _RESIZE_MASK_ is used for that. Look into View.h to see how
			it's defined
*/
	fFlags		= (flags & ~_RESIZE_MASK_) | (fFlags & _RESIZE_MASK_);
	
	fState->archivingFlags	|= B_VIEW_FLAGS_BIT;
}

//---------------------------------------------------------------------------

BRect BView::Frame() const 
{
	check_lock_no_pick();
	
	if ( fState->flags & B_VIEW_COORD_BIT ){
		Bounds();
	}

	return Bounds().OffsetToCopy( originX, originY );
}

//---------------------------------------------------------------------------

void BView::Hide()
{
	if ( owner && fShowLevel == 0)
	{
		check_lock();
		owner->fLink->StartMessage( AS_LAYER_HIDE );
	}
	fShowLevel++;
}

//---------------------------------------------------------------------------

void BView::Show()
{
	fShowLevel--;
	if (owner && fShowLevel == 0)
	{
		check_lock();
		owner->fLink->StartMessage( AS_LAYER_SHOW );
	}
}

//---------------------------------------------------------------------------

bool BView::IsFocus() const 
{
	if (owner)
	{
		check_lock_no_pick();
		return owner->CurrentFocus() == this;
	}
	else
		return false;
}

//---------------------------------------------------------------------------

bool 
BView::IsHidden(const BView *lookingFrom) const
{
	if (fShowLevel > 0)
		return true;

	// may we be egocentric?
	if (lookingFrom == this)
		return false;

	// we have the same visibility state as our
	// parent, if there is one
	if (parent)
		return parent->IsHidden(lookingFrom);

	// if we're the top view, and we're interested
	// in the "global" view, we're inheriting the
	// state of the window's visibility
	if (owner && lookingFrom == NULL)
		return owner->IsHidden();

	return false;
}

//---------------------------------------------------------------------------

bool
BView::IsHidden() const
{
	return IsHidden(NULL);
}

//---------------------------------------------------------------------------

bool BView::IsPrinting() const 
{
	return f_is_printing;
}

//---------------------------------------------------------------------------

BPoint BView::LeftTop() const 
{
	return Bounds().LeftTop();
}

//---------------------------------------------------------------------------

void BView::SetOrigin(BPoint pt) 
{
	SetOrigin( pt.x, pt.y );
}

//---------------------------------------------------------------------------

void BView::SetOrigin(float x, float y) 
{
	// TODO: maybe app_server should do a redraw? - WRITE down into specs

	if ( x==originX && y==originY )
		return;
		
	if (do_owner_check())
	{
		owner->fLink->StartMessage( AS_LAYER_SET_ORIGIN );
		owner->fLink->Attach<float>( x );
		owner->fLink->Attach<float>( y );
	}

	// invalidate this flag, to stay in sync with app_server
	fState->flags			|= B_VIEW_ORIGIN_BIT;	
	
	// our local coord system origin has changed, so when archiving we'll add this too
	fState->archivingFlags	|= B_VIEW_ORIGIN_BIT;			
}

//---------------------------------------------------------------------------

BPoint BView::Origin(void) const
{
	
	if ( fState->flags & B_VIEW_ORIGIN_BIT ) 
	{
		do_owner_check();
		
		owner->fLink->StartMessage( AS_LAYER_GET_ORIGIN );
		owner->fLink->Flush();
		
		int32 rCode = SERVER_FALSE;
		owner->fLink->GetNextReply(&rCode);
		if(rCode==SERVER_TRUE)
		{			
			owner->fLink->Read<BPoint>( &fState->coordSysOrigin );
			fState->flags			&= ~B_VIEW_ORIGIN_BIT;			
		}
	}
	
	return fState->coordSysOrigin;
}

//---------------------------------------------------------------------------

void BView::SetResizingMode(uint32 mode) 
{
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_RESIZE_MODE );
		owner->fLink->Attach<uint32>( mode );
	}

		// look at SetFlags() for more info on the below line
	fFlags			= (fFlags & ~_RESIZE_MASK_) | (mode & _RESIZE_MASK_);
	
		// our resize mode has changed, so when archiving we'll add this too
	fState->archivingFlags	|= B_VIEW_RESIZE_BIT;			
}

//---------------------------------------------------------------------------

uint32 BView::ResizingMode() const {
	return fFlags & _RESIZE_MASK_;
}

//---------------------------------------------------------------------------

void BView::SetViewCursor(const BCursor *cursor, bool sync)
{

	if (!cursor)
		return;

	if (!owner)
		debugger("View method requires owner and doesn't have one");

	check_lock();

	if (sync)
	{
		owner->fLink->StartMessage( AS_LAYER_CURSOR );
		owner->fLink->Attach<int32>( cursor->m_serverToken );
		owner->fLink->Flush();
	}
	else
	{
		owner->fLink->StartMessage( AS_LAYER_CURSOR );
		owner->fLink->Attach<int32>( cursor->m_serverToken );
	}
}

//---------------------------------------------------------------------------

void BView::Flush(void) const 
{
	if (owner)
		owner->Flush();
}

//---------------------------------------------------------------------------

void BView::Sync(void) const 
{
	do_owner_check_no_pick();
	owner->Sync();
}

//---------------------------------------------------------------------------

BWindow* BView::Window() const 
{
	return owner;
}



// Hook Functions
//---------------------------------------------------------------------------

void BView::AttachedToWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AttachedToWindow()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::AllAttached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllAttached()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::DetachedFromWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DetachedFromWindow()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::AllDetached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllDetached()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::Draw(BRect updateRect)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Draw()\n", Name()));
}

void BView::DrawAfterChildren(BRect r)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DrawAfterChildren()\n", Name()));
}

void BView::FrameMoved(BPoint new_position)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameMoved()\n", Name()));
}

void BView::FrameResized(float new_width, float new_height)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameResized()\n", Name()));
}

void BView::GetPreferredSize(float* width, float* height)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::GetPreferredSize()\n", Name()));
	*width				= fBounds.Width();
	*height				= fBounds.Height();
}

void BView::ResizeToPreferred()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::ResizeToPreferred()\n", Name()));

	ResizeTo(fBounds.Width(), fBounds.Height()); 
}

void BView::KeyDown(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyDown()\n", Name()));
}

void BView::KeyUp(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyUp()\n", Name()));
}

void BView::MouseDown(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseDown()\n", Name()));
}

void BView::MouseUp(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseUp()\n", Name()));
}

void BView::MouseMoved(BPoint where, uint32 code, const BMessage* a_message)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseMoved()\n", Name()));
}

void BView::Pulse()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Pulse()\n", Name()));
}

void BView::TargetedByScrollView(BScrollView* scroll_view)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::TargetedByScrollView()\n", Name()));
}

void BView::WindowActivated(bool state)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::WindowActivated()\n", Name()));
}

// Input Functions
//---------------------------------------------------------------------------

void BView::BeginRectTracking(BRect startRect, uint32 style)
{
	if (do_owner_check()) 
	{
		owner->fLink->StartMessage( AS_LAYER_BEGIN_RECT_TRACK );
		owner->fLink->Attach<BRect>( startRect );
		owner->fLink->Attach<int32>( style );		
	}
}

//---------------------------------------------------------------------------

void BView::EndRectTracking()
{
	if (do_owner_check())
		owner->fLink->StartMessage( AS_LAYER_END_RECT_TRACK );
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BRect dragRect, BHandler* reply_to)
{
	if ( !aMessage || !dragRect.IsValid())
		return;
		
	if (!reply_to)
		reply_to = this;
	
	if (reply_to->Looper())
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();
	
	BPoint			offset;
	
	if ( !aMessage->HasInt32("buttons") )
	{
		BMessage 	*msg 	= owner->CurrentMessage();
		uint32		buttons;

		if ( msg )
		{
			if ( msg->FindInt32("buttons", (int32*)&buttons) == B_OK )
			{
			}
			else
			{
				BPoint		point;
				GetMouse(&point, &buttons, false);
			}
			BPoint		point;

			if (msg->FindPoint("be:view_where", &point) == B_OK)
				offset = point - dragRect.LeftTop();
			
		}
		else
		{
			BPoint		point;
			GetMouse(&point, &buttons, false);
		}
		
		aMessage->AddInt32("buttons", buttons);		
	}	

	_set_message_reply_( aMessage, BMessenger( reply_to, reply_to->Looper() ) );
	
	int32		bufSize		= aMessage->FlattenedSize();
	char		*buffer		= new char[ bufSize ];
	aMessage->Flatten( buffer, bufSize );

	owner->fLink->StartMessage( AS_LAYER_DRAG_RECT );
	owner->fLink->Attach<BRect>( dragRect );
	owner->fLink->Attach<BPoint>( offset );	
	owner->fLink->Attach<int32>( bufSize );
	owner->fLink->Attach( buffer, bufSize );
	owner->fLink->Flush();

	delete [] buffer;
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BBitmap* anImage, BPoint offset,
		BHandler* reply_to)
{
	DragMessage( aMessage, anImage, B_OP_COPY, offset, reply_to );
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BBitmap* anImage,
		drawing_mode dragMode, BPoint offset,BHandler* reply_to)
{
	if ( !aMessage || !anImage )
		return;
	
	if (!reply_to)
		reply_to = this;
	
	if (reply_to->Looper())
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();
	
	if ( !aMessage->HasInt32("buttons") )
	{
		BMessage 	*msg 	= owner->CurrentMessage();
		uint32		buttons;

		if ( msg )
		{
			if ( msg->FindInt32("buttons", (int32*)&buttons) == B_OK )
			{
			}
			else
			{
				BPoint		point;
				GetMouse(&point, &buttons, false);
			}
		}
		else
		{
			BPoint		point;
			GetMouse(&point, &buttons, false);
		}
		aMessage->AddInt32("buttons", buttons);		
	}	

	_set_message_reply_( aMessage, BMessenger( reply_to, reply_to->Looper() ) );
	
	int32		bufSize		= aMessage->FlattenedSize();
	char		*buffer		= new char[ bufSize ];
	aMessage->Flatten( buffer, bufSize );

	owner->fLink->StartMessage( AS_LAYER_DRAG_IMAGE );
	owner->fLink->Attach<int32>( anImage->get_server_token() );
	owner->fLink->Attach<int32>( (int32)dragMode );	
	owner->fLink->Attach<BPoint>( offset );
	owner->fLink->Attach<int32>( bufSize );
	owner->fLink->Attach( buffer, bufSize );

	delete [] buffer;

	// TODO: in app_server the bitmap refCount must be incremented
	// WRITE this into specs!!!!

	delete anImage;
}

//---------------------------------------------------------------------------

void BView::GetMouse(BPoint* location, uint32* buttons, bool checkMessageQueue)
{
	do_owner_check();
	
	if (checkMessageQueue)
	{
		BMessageQueue	*mq;
		BMessage		*msg;
		int32			i = 0;
		
		mq				= Window()->MessageQueue();
		mq->Lock();
		
		while( (msg = mq->FindMessage(i++)) != NULL ) 
		{
			switch (msg->what) 
			{
				case B_MOUSE_UP:
				{
					msg->FindPoint("where", location);
					msg->FindInt32("buttons", (int32*)buttons);
					Window()->DispatchMessage( msg, Window() );
					mq->RemoveMessage( msg );
					delete msg;
					return;
					break;
				}	
				case B_MOUSE_MOVED:
				{
					msg->FindPoint("where", location);
					msg->FindInt32("buttons", (int32*)buttons);
					Window()->DispatchMessage( msg, Window() );
					mq->RemoveMessage( msg );
					delete msg;						
					return;
					break;
				}	
				case _UPDATE_:
				{
					Window()->DispatchMessage( msg, Window() );
					mq->RemoveMessage( msg );
					delete msg;
					return;
					break;
				}	
				default:
					break;
			}
		}
		mq->Unlock();
	}
	
	// If B_MOUSE_UP or B_MOUSE_MOVED has not been found in the message queue,
	// tell app_server to send us the current mouse coords and buttons.
	owner->fLink->StartMessage( AS_LAYER_GET_MOUSE_COORDS );
	owner->fLink->Flush();
	
	int32 rCode=SERVER_FALSE;
	owner->fLink->GetNextReply(&rCode);
	if(rCode==SERVER_TRUE)
	{
		owner->fLink->Read<BPoint>( location );
		owner->fLink->Read<int32>( (int32*)buttons );
	}
}

//---------------------------------------------------------------------------

void BView::MakeFocus(bool focusState)
{
	if (owner)
	{
		// if a view is in focus
		BView		*focus = owner->CurrentFocus();
		if (focus)
		{
			owner->fFocus	= NULL;
			focus->MakeFocus(false);
			owner->SetPreferredHandler(NULL);
		}
		
		// if we want to make this view the current focus view
		if (focusState)
		{
			owner->fFocus	= this;
			owner->SetPreferredHandler(this);
		}
	}
}

//---------------------------------------------------------------------------

BScrollBar* BView::ScrollBar(orientation posture) const
{
	switch (posture) 
	{
		case B_VERTICAL:
		{
			return fVerScroller;
			break;
		}
		case B_HORIZONTAL:
		{
			return fHorScroller;
			break;
		}
		default:
				return NULL;
			break;
	}
}

//---------------------------------------------------------------------------

void BView::ScrollBy(float dh, float dv)
{
	// no reason to process this further if no scroll is intended.
	if ( dh == 0 && dv == 0)
		return;

	check_lock();
	
	// if we're attached to a window tell app_server about this change
	if (owner)
	{
		owner->fLink->StartMessage( AS_LAYER_SCROLL );
		owner->fLink->Attach<float>( dh );
		owner->fLink->Attach<float>( dv );
	
		fState->flags		|= B_VIEW_COORD_BIT;		
	}
	
	// we modify our bounds rectangle by dh/dv coord units hor/ver.
	fBounds.OffsetBy(dh, dv);
	
	// then set the new values of the scrollbars
	if (fHorScroller)
		fHorScroller->SetValue( fBounds.top );
	if (fVerScroller)		
		fVerScroller->SetValue( fBounds.left );
}

//---------------------------------------------------------------------------

void BView::ScrollTo(BPoint where)
{
	ScrollBy( where.x - fBounds.left, where.y - fBounds.top );
}

//---------------------------------------------------------------------------

status_t BView::SetEventMask(uint32 mask, uint32 options)
{
	if (fEventMask == mask && fEventOptions == options)
		return B_ERROR;
		
	fEventMask			= mask | (fEventMask & 0xFFFF0000);
	fEventOptions		= options;
	
	fState->archivingFlags	|= B_VIEW_EVMASK_BIT;
	
	// TODO: modify! contact app_server!	
	
	return B_OK;
}

//---------------------------------------------------------------------------

uint32 BView::EventMask()
{
	return fEventMask;
}

//---------------------------------------------------------------------------

status_t BView::SetMouseEventMask(uint32 mask, uint32 options)
{
	fEventMask = (mask << 16) | (fEventMask & 0x0000FFFF);
	fEventOptions = (options << 16) | (options & 0x0000FFFF);
	
	// TODO: Contact app_server

	return B_OK;
}

// Graphic State Functions
//---------------------------------------------------------------------------
void BView::SetLineMode(cap_mode lineCap, join_mode lineJoin,float miterLimit)
{
	if (lineCap == fState->lineCap && lineJoin == fState->lineJoin &&
			miterLimit == fState->miterLimit)
		return;

	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_LINE_MODE );
		owner->fLink->Attach<int8>( (int8)lineCap );
		owner->fLink->Attach<int8>( (int8)lineJoin );
		owner->fLink->Attach<float>( miterLimit );
		
		fState->flags		|= B_VIEW_LINE_MODES_BIT;
	}
	
	fState->lineCap			= lineCap;
	fState->lineJoin		= lineJoin;
	fState->miterLimit		= miterLimit;
	
	fState->archivingFlags	|= B_VIEW_LINE_MODES_BIT;
}	

//---------------------------------------------------------------------------

join_mode BView::LineJoinMode() const
{
	if (fState->flags & B_VIEW_LINE_MODES_BIT)
		LineMiterLimit();
	
	return fState->lineJoin;
}

//---------------------------------------------------------------------------

cap_mode BView::LineCapMode() const
{
	if (fState->flags & B_VIEW_LINE_MODES_BIT)
		LineMiterLimit();
	
	return fState->lineCap;
}

//---------------------------------------------------------------------------

float BView::LineMiterLimit() const
{
	if ( (fState->flags & B_VIEW_LINE_MODES_BIT) && owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_GET_LINE_MODE );
		owner->fLink->Flush();
		
		int32 rCode = SERVER_FALSE;
		owner->fLink->GetNextReply( &rCode );
		if (rCode == SERVER_TRUE)
		{
			owner->fLink->Read<int8>( (int8*)&(fState->lineCap) );
			owner->fLink->Read<int8>( (int8*)&(fState->lineJoin) );
			owner->fLink->Read<float>( &(fState->miterLimit) );
		}
		
		fState->flags			&= ~B_VIEW_LINE_MODES_BIT;
	}
	
	return fState->miterLimit;
}

//---------------------------------------------------------------------------

void BView::PushState()
{
	do_owner_check();
	
	owner->fLink->StartMessage( AS_LAYER_PUSH_STATE );
	
	initCachedState();
}

//---------------------------------------------------------------------------

void BView::PopState()
{
	do_owner_check();

	owner->fLink->StartMessage( AS_LAYER_POP_STATE );

	// this avoids a compiler warning
	uint32 dummy	= 0xffffffffUL;
	
	// invalidate all flags
	fState->flags	= dummy;
}

//---------------------------------------------------------------------------

void BView::SetScale(float scale) const
{
	if (scale == fState->scale)
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_SCALE );
		owner->fLink->Attach<float>( scale );
		
		// I think that this flag won't be used after all... in 'flags' of course.
		fState->flags		|= B_VIEW_SCALE_BIT;	
	}
	
	fState->scale			= scale;

	fState->archivingFlags	|= B_VIEW_SCALE_BIT;
}
//---------------------------------------------------------------------------

float BView::Scale() const
{
	if ( (fState->flags & B_VIEW_SCALE_BIT) && owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_GET_SCALE );
		owner->fLink->Flush();
		
 		int32 rCode = SERVER_FALSE;
 		owner->fLink->GetNextReply( &rCode );
 		if (rCode == SERVER_TRUE)
 		{
			owner->fLink->Read<float>( &(fState->scale) );
		}
		
		fState->flags			&= ~B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}

//---------------------------------------------------------------------------

void BView::SetDrawingMode(drawing_mode mode)
{
	if (mode == fState->drawingMode)
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_DRAW_MODE );
		owner->fLink->Attach<int8>( (int8)mode );
		
		fState->flags		|= B_VIEW_DRAW_MODE_BIT;	
	}
	
	fState->drawingMode		= mode;

	fState->archivingFlags	|= B_VIEW_DRAW_MODE_BIT;
}

//---------------------------------------------------------------------------

drawing_mode BView::DrawingMode() const
{
	if ( (fState->flags & B_VIEW_DRAW_MODE_BIT) && owner)
	{
		check_lock();
		int8		drawingMode;

		owner->fLink->StartMessage( AS_LAYER_GET_DRAW_MODE );
		owner->fLink->Flush();
		
		int32 rCode = SERVER_FALSE;
 		owner->fLink->GetNextReply( &rCode );
 		if (rCode == SERVER_TRUE)
			owner->fLink->Read<int8>( &drawingMode );
		
		fState->drawingMode		= (drawing_mode)drawingMode;
		
		fState->flags			&= ~B_VIEW_DRAW_MODE_BIT;
	}
	
	return fState->drawingMode;
}

//---------------------------------------------------------------------------

void BView::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
	if (srcAlpha  == fState->alphaSrcMode && alphaFunc == fState->alphaFncMode)
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_BLEND_MODE );
		owner->fLink->Attach<int8>( (int8)srcAlpha );
		owner->fLink->Attach<int8>( (int8)alphaFunc );		
		
		fState->flags		|= B_VIEW_BLENDING_BIT;	
	}
	
	fState->alphaSrcMode	= srcAlpha;
	fState->alphaFncMode	= alphaFunc;	

	fState->archivingFlags	|= B_VIEW_BLENDING_BIT;
}

//---------------------------------------------------------------------------

void BView::GetBlendingMode(source_alpha* srcAlpha,	alpha_function* alphaFunc) const
{
	if ( (fState->flags & B_VIEW_BLENDING_BIT) && owner)
	{
		check_lock();
		int8		alphaSrcMode, alphaFncMode;
		
		owner->fLink->StartMessage( AS_LAYER_GET_BLEND_MODE );
		owner->fLink->Flush();
		
		int32 rCode = SERVER_FALSE;
 		owner->fLink->GetNextReply( &rCode );
 		if (rCode == SERVER_TRUE)
 		{
			owner->fLink->Read<int8>( &alphaSrcMode );
			owner->fLink->Read<int8>( &alphaFncMode );
		}
		
		fState->alphaSrcMode	= (source_alpha)alphaSrcMode;
		fState->alphaFncMode	= (alpha_function)alphaFncMode;
		
		fState->flags			&= ~B_VIEW_BLENDING_BIT;
	}
	
	if (srcAlpha)
		*srcAlpha		= fState->alphaSrcMode;
		
	if (alphaFunc)
		*alphaFunc		= fState->alphaFncMode;
}

//---------------------------------------------------------------------------

void BView::MovePenTo(BPoint pt)
{
	MovePenTo( pt.x, pt.y );
}

//---------------------------------------------------------------------------

void BView::MovePenTo(float x, float y)
{
	if (x  == fState->penPosition.x && y == fState->penPosition.y)
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_PEN_LOC );
		owner->fLink->Attach<float>( x );
		owner->fLink->Attach<float>( y );		
		
		fState->flags		|= B_VIEW_PEN_LOC_BIT;	
	}
	
	fState->penPosition.x	= x;
	fState->penPosition.y	= y;	

	fState->archivingFlags	|= B_VIEW_PEN_LOC_BIT;
}

//---------------------------------------------------------------------------

void BView::MovePenBy(float x, float y)
{
	MovePenTo(fState->penPosition.x + x, fState->penPosition.y + y);
}

//---------------------------------------------------------------------------

BPoint BView::PenLocation() const
{
	if ( (fState->flags & B_VIEW_PEN_LOC_BIT) && owner)
	{
		check_lock();

		owner->fLink->StartMessage( AS_LAYER_GET_PEN_LOC );
		owner->fLink->Flush();
		
		int32 rCode = SERVER_FALSE;
 		owner->fLink->GetNextReply( &rCode );
 		if (rCode == SERVER_TRUE)
			owner->fLink->Read<BPoint>( &(fState->penPosition) );
		
		fState->flags			&= ~B_VIEW_PEN_LOC_BIT;
	}

	return fState->penPosition;
}

//---------------------------------------------------------------------------

void BView::SetPenSize(float size){
	if (size == fState->penSize)
		return;
		
	if (owner){
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_PEN_SIZE );
		owner->fLink->Attach<float>( size );
		
		fState->flags		|= B_VIEW_PEN_SIZE_BIT;	
	}
	
	fState->penSize			= size;

	fState->archivingFlags	|= B_VIEW_PEN_SIZE_BIT;
}

//---------------------------------------------------------------------------

float BView::PenSize() const
{
	if (fState->flags & B_VIEW_PEN_SIZE_BIT)
	{
		if (owner)
		{
			check_lock();
			
			owner->fLink->StartMessage( AS_LAYER_GET_PEN_SIZE );
			owner->fLink->Flush();
			
			int32 rCode = SERVER_FALSE;
 			owner->fLink->GetNextReply( &rCode );
 			if (rCode == SERVER_TRUE)
				owner->fLink->Read<float>( &(fState->penSize) );
			
			fState->flags			&= ~B_VIEW_PEN_SIZE_BIT;
		}
	}
	return fState->penSize;
}

//---------------------------------------------------------------------------

void BView::SetHighColor(rgb_color a_color)
{
	if (_rgb_color_are_equal( fState->highColor, a_color ))
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_HIGH_COLOR );
		owner->fLink->Attach<rgb_color>( a_color );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->highColor), a_color.red, a_color.green,
											a_color.blue, a_color.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::HighColor() const
{
	if (fState->flags & B_VIEW_COLORS_BIT)
	{
		if (owner)
		{
			check_lock();
			
			owner->fLink->StartMessage( AS_LAYER_GET_COLORS );
			owner->fLink->Flush();
			
			int32 rCode = SERVER_FALSE;
	 		owner->fLink->GetNextReply( &rCode );
	 		if (rCode == SERVER_TRUE)
	 		{
				owner->fLink->Read<rgb_color>( &(fState->highColor) );
				owner->fLink->Read<rgb_color>( &(fState->lowColor) );
				owner->fLink->Read<rgb_color>( &(fState->viewColor) );				
			}
			
			fState->flags			&= ~B_VIEW_COLORS_BIT;
		}
	}
		
	return fState->highColor;
}

//---------------------------------------------------------------------------

void BView::SetLowColor(rgb_color a_color)
{
	if (_rgb_color_are_equal( fState->lowColor, a_color ))
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_LOW_COLOR );
		owner->fLink->Attach<rgb_color>( a_color );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->lowColor), a_color.red, a_color.green,
											a_color.blue, a_color.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::LowColor() const
{
	if (fState->flags & B_VIEW_COLORS_BIT)
	{
		if (owner)
		{
			// HighColor() contacts app_server and gets the high, low and view colors
			HighColor();
		}
	}
	return fState->lowColor;
}

//---------------------------------------------------------------------------

void BView::SetViewColor(rgb_color c)
{
	if (_rgb_color_are_equal( fState->viewColor, c ))
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_SET_VIEW_COLOR );
		owner->fLink->Attach<rgb_color>( c );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->viewColor), c.red, c.green,
											c.blue, c.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::ViewColor() const
{
	if (fState->flags & B_VIEW_COLORS_BIT)
	{
		if (owner)
		{
			// HighColor() contacts app_server and gets the high, low and view colors
			HighColor();
		}
	}
	return fState->viewColor;
}

//---------------------------------------------------------------------------

void BView::ForceFontAliasing(bool enable)
{
	if ( enable == fState->fontAliasing)
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_PRINT_ALIASING );
		owner->fLink->Attach<bool>( enable );
		
		// I think this flag won't be used...
		fState->flags		|= B_VIEW_FONT_ALIASING_BIT;	
	}

	fState->fontAliasing	= enable;
	
	fState->archivingFlags	|= B_VIEW_FONT_ALIASING_BIT;
}

//---------------------------------------------------------------------------

void BView::SetFont(const BFont* font, uint32 mask)
{

	if (!font || mask == 0)
		return;

	if ( mask == B_FONT_ALL )
	{
		fState->font	= *font;
	}
	else
	{
		if ( mask & B_FONT_FAMILY_AND_STYLE )
			fState->font.SetFamilyAndStyle( font->FamilyAndStyle() );
		
		if ( mask & B_FONT_SIZE )
			fState->font.SetSize( font->Size() );
	
		if ( mask & B_FONT_SHEAR )
			fState->font.SetShear( font->Shear() );
	
		if ( mask & B_FONT_ROTATION )
			fState->font.SetRotation( font->Rotation() );		
	
		if ( mask & B_FONT_SPACING )
			fState->font.SetSpacing( font->Spacing() );
	
		if ( mask & B_FONT_ENCODING )
			fState->font.SetEncoding( font->Encoding() );		
	
		if ( mask & B_FONT_FACE )
			fState->font.SetFace( font->Face() );
		
		if ( mask & B_FONT_FLAGS )
			fState->font.SetFlags( font->Flags() );
	}
	
	fState->fontFlags	= mask;

	if (owner)
	{
		check_lock();
		
		setFontState( &(fState->font), fState->fontFlags );
	}
}

//---------------------------------------------------------------------------

#if !_PR3_COMPATIBLE_
void BView::GetFont(BFont* font) const
{
	*font	= fState->font;	
}

//---------------------------------------------------------------------------

#else
void BView:GetFont(BFont* font)
{
	*font	= fState->font;
}
#endif

//---------------------------------------------------------------------------

void BView::GetFontHeight(font_height* height) const
{
	fState->font.GetHeight( height );
}

//---------------------------------------------------------------------------

void BView::SetFontSize(float size)
{
	fState->font.SetSize( size );
}

//---------------------------------------------------------------------------

float BView::StringWidth(const char* string) const
{
	return fState->font.StringWidth( string );
}

//---------------------------------------------------------------------------

float BView::StringWidth(const char* string, int32 length) const
{
	return fState->font.StringWidth( string, length );
}

//---------------------------------------------------------------------------

void BView::GetStringWidths(char* stringArray[],int32 lengthArray[],
		int32 numStrings, float widthArray[]) const
{
	// ARE these const_cast good?????
	fState->font.GetStringWidths( const_cast<const char**>(stringArray),
			 const_cast<const int32*>(lengthArray),numStrings, &*widthArray );
}

//---------------------------------------------------------------------------

void BView::TruncateString(BString* in_out, uint32 mode, float width) const
{
	fState->font.TruncateString( in_out, mode, width);
}

//---------------------------------------------------------------------------

void BView::ClipToPicture(BPicture* picture,BPoint where,bool sync)
{
	if (!picture)
		return;
		
	if (do_owner_check())
	{
		owner->fLink->StartMessage( AS_LAYER_CLIP_TO_PICTURE );
		owner->fLink->Attach<int32>( picture->token );
		owner->fLink->Attach<BPoint>( where );
		
		if (sync)
			owner->fLink->Flush();

		fState->flags		|= B_VIEW_CLIP_REGION_BIT;	
	}

	fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::ClipToInversePicture(BPicture* picture,
								 BPoint where,
								 bool sync)
{
	if (!picture)
		return;
		
	if (do_owner_check())
	{
		owner->fLink->StartMessage( AS_LAYER_CLIP_TO_INVERSE_PICTURE );
		owner->fLink->Attach<int32>( picture->token );
		owner->fLink->Attach<BPoint>( where );
		
		if (sync)
		{
			owner->fLink->Flush();
		}

		fState->flags		|= B_VIEW_CLIP_REGION_BIT;	
	}

	fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::GetClippingRegion(BRegion* region) const
{
	if (!region)
		return;

	if (fState->flags & B_VIEW_CLIP_REGION_BIT)
	{
		if (do_owner_check())
		{
			int32					noOfRects;
	
			owner->fLink->StartMessage( AS_LAYER_GET_CLIP_REGION );
	 		owner->fLink->Flush();
	 	
	 		int32 rCode = SERVER_FALSE;
	 		owner->fLink->GetNextReply( &rCode );
	 		if (rCode == SERVER_TRUE)
	 		{
				owner->fLink->Read<int32>( &noOfRects );
				
				fState->clippingRegion.MakeEmpty();
				for (int32 i = 0; i < noOfRects; i++)
				{
					BRect				rect;
					
					owner->fLink->Read<BRect>( &rect );
					
					fState->clippingRegion.Include( rect );
				}
				fState->flags			&= ~B_VIEW_CLIP_REGION_BIT;
			}
		}
	}
	*region			= fState->clippingRegion;
}

//---------------------------------------------------------------------------

void BView::ConstrainClippingRegion(BRegion* region)
{
	if (do_owner_check())
	{
		int32		noOfRects = 0;
		
		if (region)
			noOfRects	= region->CountRects();
		
		owner->fLink->StartMessage( AS_LAYER_SET_CLIP_REGION );

		// '0' means that in the app_server, there won't be any 'local'
		// clipping region (it will be = NULL)
		
		// TODO: note this in the specs
		owner->fLink->Attach<int32>( noOfRects );
		
		for (int32 i = 0; i<noOfRects; i++)
			owner->fLink->Attach<BRect>( region->RectAt(i) );
		
		// we flush here because app_server waits for all the rects
		owner->fLink->Flush();

		fState->flags			|= B_VIEW_CLIP_REGION_BIT;
		fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
	}
}

//---------------------------------------------------------------------------


// Drawing Functions
//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap,	BRect srcRect, BRect dstRect)
{
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT );
		owner->fLink->Attach<int32>( aBitmap->get_server_token() );
		owner->fLink->Attach<BRect>( srcRect );		
		owner->fLink->Attach<BRect>( dstRect );
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BRect dstRect)
{
	if ( !aBitmap || !dstRect.IsValid())
		return;
	
	DrawBitmapAsync( aBitmap, aBitmap->Bounds(), dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap)
{
	DrawBitmapAsync( aBitmap, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BPoint where)
{
	if ( !aBitmap )
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT );
		owner->fLink->Attach<int32>( aBitmap->get_server_token() );
		owner->fLink->Attach<BPoint>( where );
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap)
{
	DrawBitmap( aBitmap, PenLocation() );	
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BPoint where)
{
	if ( !aBitmap )
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT );
		owner->fLink->Attach<int32>( aBitmap->get_server_token() );
		owner->fLink->Attach<BPoint>( where );
		owner->fLink->Flush();
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect dstRect)
{
	if ( !aBitmap || !dstRect.IsValid())
		return;
	
	DrawBitmap( aBitmap, aBitmap->Bounds(), dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect srcRect, BRect dstRect)
{
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;
		
	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT );
		owner->fLink->Attach<int32>( aBitmap->get_server_token() );
		owner->fLink->Attach<BRect>( dstRect );
		owner->fLink->Attach<BRect>( srcRect );		
		owner->fLink->Flush();
	}
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar)
{
	DrawChar( aChar, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar, BPoint location)
{
	char		ch[2];
	ch[0]		= aChar;
	ch[1]		= '\0';
	
	DrawString( ch, strlen(ch), location );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, escapement_delta* delta)
{
	if ( !aString )
		return;

	DrawString( aString, strlen(aString), PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, BPoint location, escapement_delta* delta)
{
	if ( !aString )
		return;

	DrawString( aString, strlen(aString), location );	
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length, escapement_delta* delta)
{
	if ( !aString )
		return;

	DrawString( aString, length, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length, BPoint location,
		escapement_delta* delta)
{
	if ( !aString )
		return;

	if (owner)
	{
		check_lock();
		
		owner->fLink->StartMessage( AS_DRAW_STRING );
		owner->fLink->Attach<BPoint>( location );
		
		// Quite often delta will be NULL, so we have to accomodate this.
		if(delta)
			owner->fLink->Attach<escapement_delta>( *delta );
		else
		{
			escapement_delta tdelta;
			tdelta.space=0;
			tdelta.nonspace=0;
			
			owner->fLink->Attach<escapement_delta>( tdelta );
		}
		owner->fLink->AttachString( aString );		

		// this modifies our pen location, so we invalidate the flag.
		fState->flags		|= B_VIEW_PEN_LOC_BIT;
	}
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
		pattern p)
{
	if(owner)
	{
		StrokeEllipse( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
			center.y+yRadius), p );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BRect r, pattern p) 
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_ELLIPSE );
		owner->fLink->Attach<BRect>( r );
	}
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BPoint center, float xRadius, float yRadius,
		pattern p)
{
	if(owner)
	{
		FillEllipse( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
			center.y+yRadius), p );
	}
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BRect r, pattern p) 
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_ELLIPSE );
		owner->fLink->Attach<BRect>( r );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeArc(BPoint center, float xRadius, float yRadius,
		float start_angle, float arc_angle, pattern p)
{
	StrokeArc( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
		center.y+yRadius), start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------

void BView::StrokeArc(BRect r, float start_angle, float arc_angle,
		pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_ARC );
		owner->fLink->Attach<BRect>(r);
		owner->fLink->Attach<float>( start_angle );
		owner->fLink->Attach<float>( arc_angle );				
	}
	
}

//---------------------------------------------------------------------------
									  
void BView::FillArc(BPoint center,float xRadius, float yRadius,
		float start_angle, float arc_angle,	pattern p)
{
	FillArc( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
		center.y+yRadius), start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------

void BView::FillArc(BRect r, float start_angle, float arc_angle,
		pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_ARC );
		owner->fLink->Attach<BRect>(r);
		owner->fLink->Attach<float>( start_angle );
		owner->fLink->Attach<float>( arc_angle );				
	}
	
}

//---------------------------------------------------------------------------

void BView::StrokeBezier(BPoint* controlPoints, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_BEZIER );
		owner->fLink->Attach<BPoint>( controlPoints[0] );
		owner->fLink->Attach<BPoint>( controlPoints[1] );
		owner->fLink->Attach<BPoint>( controlPoints[2] );
		owner->fLink->Attach<BPoint>( controlPoints[3] );
	}
}

//---------------------------------------------------------------------------

void BView::FillBezier(BPoint* controlPoints, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_BEZIER );
		owner->fLink->Attach<BPoint>( controlPoints[0] );
		owner->fLink->Attach<BPoint>( controlPoints[1] );
		owner->fLink->Attach<BPoint>( controlPoints[2] );
		owner->fLink->Attach<BPoint>( controlPoints[3] );
	}
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPolygon* aPolygon,bool closed, pattern p)
{
	if(!aPolygon)
		return;
	
	StrokePolygon(aPolygon->fPts, aPolygon->fCount, aPolygon->Frame(), closed, p);
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts,bool closed, pattern p)
{
	StrokePolygon( ptArray, numPts, closed, p );
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
		bool closed, pattern p)
{
	if ( !ptArray )
		return;
	
	if(numPts<=2)
		return;
	
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		BPolygon pol(ptArray,numPts);
		pol.MapTo(pol.Frame(),bounds);
		
		owner->fLink->StartMessage( AS_STROKE_POLYGON );
		owner->fLink->Attach<BRect>(pol.Frame());
		owner->fLink->Attach<int8>( closed );
		owner->fLink->Attach<int32>( pol.fCount );
		owner->fLink->Attach(pol.fPts,pol.fCount * sizeof(BPoint) );
	}
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPolygon* aPolygon,pattern p)
{
	if ( !aPolygon )
		return;
		
	if ( aPolygon->fCount <= 2 )
		return;

	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_POLYGON );
		owner->fLink->Attach<int32>( aPolygon->fCount );
		owner->fLink->Attach(aPolygon->fPts,aPolygon->fCount * sizeof(BPoint) );
	}
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts, pattern p)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
		pattern p)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	pol.MapTo( pol.Frame(), bounds);
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::StrokeRect(BRect r, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
			
		owner->fLink->StartMessage( AS_STROKE_RECT );
		owner->fLink->Attach<BRect>( r );
	}
}

//---------------------------------------------------------------------------

void BView::FillRect(BRect r, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_RECT );
		owner->fLink->Attach<BRect>( r );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeRoundRect(BRect r, float xRadius, float yRadius,
		pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_ROUNDRECT );
		owner->fLink->Attach<BRect>( r );
		owner->fLink->Attach<float>( xRadius );
		owner->fLink->Attach<float>( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::FillRoundRect(BRect r, float xRadius, float yRadius,
		pattern p)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_ROUNDRECT );
		owner->fLink->Attach<BRect>( r );
		owner->fLink->Attach<float>( xRadius );
		owner->fLink->Attach<float>( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::FillRegion(BRegion* a_region, pattern p)
{
	if ( !a_region )
		return;

	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		int32			rectsNo = a_region->CountRects();
		
		owner->fLink->StartMessage( AS_FILL_REGION );
		owner->fLink->Attach<int32>( rectsNo );

		for (int32 i = 0; i<rectsNo; i++)
			owner->fLink->Attach<BRect>( a_region->RectAt(i) );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
		BRect bounds, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_TRIANGLE );
		owner->fLink->Attach<BPoint>( pt1 );
		owner->fLink->Attach<BPoint>( pt2 );
		owner->fLink->Attach<BPoint>( pt3 );
		owner->fLink->Attach<BRect>( bounds );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (owner)
	{
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect		bounds(pt1, pt1);
		
		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;
		
		if (pt2.y < bounds.top)
			bounds.top = pt2.y;
		
		if (pt2.x > bounds.right)
			bounds.right = pt2.x;
		
		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;
		
		if (pt3.y < bounds.top)
			bounds.top = pt3.y;
		
		if (pt3.x > bounds.right)
			bounds.right = pt3.x;
		
		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		
		
		StrokeTriangle( pt1, pt2, pt3, bounds, p );
	}
}

//---------------------------------------------------------------------------

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (owner)
	{
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect		bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		FillTriangle( pt1, pt2, pt3, bounds, p );
	}
}

//---------------------------------------------------------------------------

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
		BRect bounds, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_TRIANGLE );
		owner->fLink->Attach<BPoint>( pt1 );
		owner->fLink->Attach<BPoint>( pt2 );
		owner->fLink->Attach<BPoint>( pt3 );
		owner->fLink->Attach<BRect>( bounds );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint toPt, pattern p)
{
	StrokeLine( PenLocation(), toPt, p);
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint pt0, BPoint pt1, pattern p)
{
	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_STROKE_LINE );
		owner->fLink->Attach<BPoint>( pt0 );
		owner->fLink->Attach<BPoint>( pt1 );
		
		// this modifies our pen location, so we invalidate the flag.
		fState->flags		|= B_VIEW_PEN_LOC_BIT;			
	}
}

//---------------------------------------------------------------------------

void BView::StrokeShape(BShape* shape, pattern p)
{
	if ( !shape )
		return;

	shape_data		*sd = (shape_data*)shape->fPrivateData;
	if ( sd->opCount == 0 || sd->ptCount == 0)
		return;

	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
	
		owner->fLink->StartMessage( AS_STROKE_SHAPE );
		owner->fLink->Attach<BRect>( shape->Bounds() );
		owner->fLink->Attach<int32>( sd->opCount );
		owner->fLink->Attach<int32>( sd->ptCount );
		owner->fLink->Attach( sd->opList, sd->opCount );
		owner->fLink->Attach( sd->ptList, sd->ptCount );
	}
}

//---------------------------------------------------------------------------

void BView::FillShape(BShape* shape, pattern p)
{
	if ( !shape )
		return;

	shape_data		*sd = (shape_data*)(shape->fPrivateData);
	if ( sd->opCount == 0 || sd->ptCount == 0)
		return;

	if (owner)
	{
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->fLink->StartMessage( AS_FILL_SHAPE );
		owner->fLink->Attach<BRect>( shape->Bounds() );
		owner->fLink->Attach<int32>( sd->opCount );
		owner->fLink->Attach<int32>( sd->ptCount );
		owner->fLink->Attach( sd->opList, sd->opCount );
		owner->fLink->Attach( sd->ptList, sd->ptCount );
	}
}

//---------------------------------------------------------------------------

void BView::BeginLineArray(int32 count)
{
	if (owner)
	{
		if (count <= 0)
			debugger("Calling BeginLineArray with a count <= 0");

		check_lock_no_pick();
		
		if (comm)
		{
			delete [] comm->array;
			delete comm;
		}

		comm			= new _array_data_;

		comm->maxCount	= count;
		comm->count		= 0;
		comm->array		= new _array_hdr_[count];
	}
}

//---------------------------------------------------------------------------

void BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (owner)
	{
		if (!comm)
			debugger("BeginLineArray must be called before using AddLine");

		check_lock_no_pick();
		
		if (comm->count < comm->maxCount)
		{
			comm->array[ comm->count ].startX	= pt0.x;
			comm->array[ comm->count ].startY	= pt0.y;
			comm->array[ comm->count ].endX		= pt1.x;
			comm->array[ comm->count ].endY		= pt1.y;
			_set_ptr_rgb_color( &(comm->array[ comm->count ].color),
								col.red, col.green, col.blue, col.alpha );
			
			comm->count++;
		}
	}
}

//---------------------------------------------------------------------------

void BView::EndLineArray()
{
	if(owner){
		if (!comm)
			debugger("Can't call EndLineArray before BeginLineArray");

		check_lock();

		owner->fLink->StartMessage( AS_LAYER_LINE_ARRAY );
		owner->fLink->Attach<int32>( comm->count );
		owner->fLink->Attach(comm->array,comm->count * sizeof(_array_hdr_) );

		delete [] comm->array;
		delete comm;
		comm = NULL;
	}
}

//---------------------------------------------------------------------------

void BView::BeginPicture(BPicture* a_picture)
{
	if (do_owner_check())
	{
		if (a_picture && a_picture->usurped == NULL)
		{
			a_picture->usurp(cpicture);
			cpicture = a_picture;

			owner->fLink->StartMessage( AS_LAYER_BEGIN_PICTURE );
		}
	}
}

//---------------------------------------------------------------------------

void BView::AppendToPicture(BPicture* a_picture)
{
	check_lock();
	
	if (a_picture && a_picture->usurped == NULL)
	{
		int32 token = a_picture->token;
		 
		if (token == -1)
		{
			BeginPicture(a_picture);
		}
		else
		{
			a_picture->usurped = cpicture;
			a_picture->set_token(-1);
			owner->fLink->StartMessage(AS_LAYER_APPEND_TO_PICTURE);
			owner->fLink->Attach<int32>( token );
		} 
	}
}

//---------------------------------------------------------------------------

BPicture* BView::EndPicture()
{
	if (do_owner_check())
	{
		if (cpicture)
		{
			int32			token;

			owner->fLink->StartMessage(AS_LAYER_END_PICTURE);
			owner->fLink->Flush();
			
 			int32 rCode = SERVER_FALSE;
 			owner->fLink->GetNextReply( &rCode );
 			if (rCode == SERVER_TRUE)
 			{
				if(owner->fLink->Read<int32>( &token ) == B_OK)
				{
					BPicture *a_picture = cpicture;
					cpicture = a_picture->step_down();
					a_picture->set_token(token);
					return a_picture;
				}
			} 
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------

void BView::SetViewBitmap(const BBitmap* bitmap, BRect srcRect, BRect dstRect,
		uint32 followFlags, uint32 options)
{
	setViewImage(bitmap, srcRect, dstRect, followFlags, options);
}

//---------------------------------------------------------------------------

void BView::SetViewBitmap(const BBitmap* bitmap,uint32 followFlags,uint32 options)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();
 
 	rect.OffsetTo(0, 0);

	setViewImage(bitmap, rect, rect, followFlags, options);
}

//---------------------------------------------------------------------------

void BView::ClearViewBitmap()
{
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}

//---------------------------------------------------------------------------

status_t BView::SetViewOverlay(const BBitmap* overlay, BRect srcRect, BRect dstRect,
		rgb_color *colorKey, uint32 followFlags, uint32 options)
{
	status_t err = setViewImage(overlay, srcRect, dstRect, followFlags,
			options | 0x4);
	
	// TODO: Incomplete?
	
	// read the color that will be treated as transparent
	owner->fLink->Read<rgb_color>( colorKey );
 
	return err;
}

//---------------------------------------------------------------------------

status_t BView::SetViewOverlay(const BBitmap* overlay, rgb_color* colorKey,
		uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (overlay)
		rect = overlay->Bounds();
 
 	rect.OffsetTo(0, 0);

	status_t err = setViewImage(overlay, rect, rect, followFlags,
			options | 0x4);
	
	// TODO: Incomplete?
	
	// read the color that will be treated as transparent
	owner->fLink->Read<rgb_color>( colorKey );
	
	return err;
}

//---------------------------------------------------------------------------

void BView::ClearViewOverlay()
{
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}

//---------------------------------------------------------------------------

void BView::CopyBits(BRect src, BRect dst)
{
	if ( !src.IsValid() || !dst.IsValid() )
		return;

	if (do_owner_check())
	{
		owner->fLink->StartMessage( AS_LAYER_COPY_BITS);
		owner->fLink->Attach<BRect>( src );
		owner->fLink->Attach<BRect>( dst );
	}
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const BPicture* a_picture)
{
	if (!a_picture)
		return;

	status_t 	err;
	
	DrawPictureAsync(a_picture, PenLocation());
	owner->fLink->Attach<int32>( SERVER_TRUE );
	owner->fLink->Flush();

	int32 rCode = SERVER_FALSE;
	owner->fLink->GetNextReply( &rCode );
	if (rCode == SERVER_TRUE)
		owner->fLink->Read<int32>( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const BPicture* a_picture, BPoint where)
{
	if (!a_picture)
		return;
	
	status_t 	err;
	
	DrawPictureAsync( a_picture, where );
	owner->fLink->Attach<int32>( SERVER_TRUE );
	owner->fLink->Flush();
	
	int32 rCode = SERVER_FALSE;
	owner->fLink->GetNextReply( &rCode );
	if (rCode == SERVER_TRUE)
		owner->fLink->Read<int32>( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	status_t 	err;
	
	DrawPictureAsync( filename, offset, where );
	owner->fLink->Attach<int32>( SERVER_TRUE );
	owner->fLink->Flush();
	
	int32 rCode = SERVER_FALSE;
	owner->fLink->GetNextReply( &rCode );
	if (rCode == SERVER_TRUE)
		owner->fLink->Read<int32>( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const BPicture* a_picture)
{
	if (!a_picture)
		return;

	DrawPictureAsync(a_picture, PenLocation());
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const BPicture* a_picture, BPoint where)
{
	if (!a_picture)
		return;

	if (do_owner_check() && a_picture->token > 0) 
	{
		owner->fLink->StartMessage( AS_LAYER_DRAW_PICTURE );
		owner->fLink->Attach<int32>( a_picture->token );
		owner->fLink->Attach<BPoint>( where );
	}
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	// TODO: test and implement
}

//---------------------------------------------------------------------------

void BView::Invalidate(BRect invalRect)
{
	if ( !invalRect.IsValid() )
		return;

	if (owner)
	{
		check_lock();

		owner->fLink->StartMessage( AS_LAYER_INVAL_RECT );
		owner->fLink->Attach<BRect>( invalRect );
		owner->fLink->Flush();
	}
}

//---------------------------------------------------------------------------

void BView::Invalidate(const BRegion* invalRegion)
{
	if ( !invalRegion )
		return;

	if (owner)
	{
		check_lock();
		int32				noOfRects = 0;
		noOfRects			= const_cast<BRegion*>(invalRegion)->CountRects();
		
		owner->fLink->StartMessage( AS_LAYER_INVAL_REGION );
		owner->fLink->Attach<int32>( noOfRects );
		
		for (int i=0; i<noOfRects; i++)
			owner->fLink->Attach<BRect>( const_cast<BRegion*>(invalRegion)->RectAt(i) );
		
		owner->fLink->Flush();
	}
}

//---------------------------------------------------------------------------

void BView::Invalidate()
{
	Invalidate( Bounds() );
}

//---------------------------------------------------------------------------

void BView::InvertRect(BRect r)
{

	if (owner)
	{
		check_lock();

		owner->fLink->StartMessage( AS_LAYER_INVERT_RECT );
		owner->fLink->Attach<BRect>( r );
	}
}


// View Hierarchy Functions
//---------------------------------------------------------------------------
void BView::AddChild(BView* child, BView* before)
{
	STRACE(("BView(%s)::AddChild(child='%s' before='%s')\n",
 		this->Name() ? this->Name(): "NULL",
 		child && child->Name() ? child->Name(): "NULL",
 		before && before->Name() ? before->Name(): "NULL"));
	
	if ( !child )
		return;

	if (child->parent != NULL)
		debugger("AddChild failed - the view already has a parent.");
	
	bool	lockedByAddChild = false;
	if ( owner && !(owner->IsLocked()) ){
		owner->Lock();
		lockedByAddChild	= true;		
	}
		
	if ( !addToList( child, before ) )
		debugger("AddChild failed - cannot find 'before' view.");	

	if ( owner )
	{
		check_lock();
	
 		STRACE(("BView(%s)::AddChild(child='%s' before='%s')... contacting app_server\n",
 				this->Name() ? this->Name(): "NULL",
 				child && child->Name() ? child->Name(): "NULL",
 				before && before->Name() ? before->Name(): "NULL"));
	
		child->setOwner( this->owner);
		attachView( child );
	
		if ( lockedByAddChild )		
			owner->Unlock();
	}
	
//	BVTRACE;
//	PrintTree();
//	PrintToStream();
}

//---------------------------------------------------------------------------

bool BView::RemoveChild(BView* child)
{
	STRACE(("BView(%s)::RemoveChild(%s)\n", this->Name(), child->Name() ));
	if (!child)
		return false;
	
	bool	rv = child->removeSelf();

//	BVTRACE;
//	PrintTree();	

	return rv;
}

//---------------------------------------------------------------------------

int32 BView::CountChildren() const
{
	uint32		noOfChildren 	= 0;
	BView		*aChild			= first_child;
	
	while ( aChild != NULL )
	{
		noOfChildren++;
		aChild		= aChild->next_sibling;
	}
	
	return noOfChildren;
}

//---------------------------------------------------------------------------

BView* BView::ChildAt(int32 index) const
{
	int32		noOfChildren 	= 0;
	BView		*aChild			= first_child;
	
	while ( aChild != NULL && noOfChildren < index ) 
	{
		noOfChildren++;
		aChild		= aChild->next_sibling;
	}
	
	return aChild;
}

//---------------------------------------------------------------------------

BView* BView::NextSibling() const
{
	return next_sibling;
}

//---------------------------------------------------------------------------

BView* BView::PreviousSibling() const
{
	return prev_sibling;	
}

//---------------------------------------------------------------------------

bool BView::RemoveSelf()
{
	return removeSelf();
}

//---------------------------------------------------------------------------

BView* BView::Parent() const
{
	return parent;
}

//---------------------------------------------------------------------------

BView* BView::FindView(const char* name) const
{
	return findView(this, name);
}

//---------------------------------------------------------------------------

void BView::MoveBy(float dh, float dv)
{
	MoveTo( originX + dh, originY + dv );
}

//---------------------------------------------------------------------------

void BView::MoveTo(BPoint where)
{
	MoveTo(where.x, where.y);
}

//---------------------------------------------------------------------------

void BView::MoveTo(float x, float y)
{

	if ( x == originX && y == originY )	
		return;

	// BeBook says we should do this. We'll do it without. So...
//	x		= roundf( x );
//	y		= roundf( y );
	
	if (owner)
	{
		check_lock();
		owner->fLink->StartMessage( AS_LAYER_MOVETO );
		owner->fLink->Attach<float>( x );
		owner->fLink->Attach<float>( y );
		
		fState->flags		|= B_VIEW_COORD_BIT;
	}
	
	originX		= x;
	originY		= y;
}

//---------------------------------------------------------------------------

void BView::ResizeBy(float dh, float dv)
{
	ResizeTo( fBounds.right + dh, fBounds.bottom + dv );
}

//---------------------------------------------------------------------------

void BView::ResizeTo(float width, float height)
{
	if ( width == fBounds.Width() && height == fBounds.Height() )
		return;

	// BeBook says we should do this. We'll do it without. So...
//	width		= roundf( width );
//	height		= roundf( height );
	
	if (owner)
	{
		check_lock();
		owner->fLink->StartMessage( AS_LAYER_RESIZETO );
		owner->fLink->Attach<float>( width );
		owner->fLink->Attach<float>( height );
		
		fState->flags		|= B_VIEW_COORD_BIT;
	}
	
	fBounds.right	= fBounds.left + width;
	fBounds.bottom	= fBounds.top + height;
}

//---------------------------------------------------------------------------
// Inherited Methods (virtual)
//---------------------------------------------------------------------------

status_t BView::GetSupportedSuites(BMessage* data)
{
	status_t err = B_OK;
	if (!data)
		err = B_BAD_VALUE;

	if (!err)
	{
		err = data->AddString("Suites", "suite/vnd.Be-view");
		if (!err)
		{
			BPropertyInfo propertyInfo(viewPropInfo);
			err = data->AddFlat("message", &propertyInfo);

			if (!err)
				err = BHandler::GetSupportedSuites(data);
		}
	}
	return err;
}

//------------------------------------------------------------------------------

BHandler* BView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
		int32 what,	const char* property)
{
	if (msg->what == B_WINDOW_MOVE_BY)
		return this;
	if (msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(viewPropInfo);
	switch (propertyInfo.FindMatch(msg, index, specifier, what, property))
	{
		case B_ERROR:
			break;
		
		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
			return this;
		
		
		case 4:
		{
			if (fShelf)
			{
				msg->PopSpecifier();
				return fShelf;
			}
			else
			{
				BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32( "error", B_NAME_NOT_FOUND );
				replyMsg.AddString( "message", "This window doesn't have a self");
				msg->SendReply( &replyMsg );
				return NULL;
			}
			break;
		}
		
		case 6:
		case 7:
		case 8:
		{
			if (first_child)
			{
				BView		*child;
				switch( msg->what )
				{
					case B_INDEX_SPECIFIER:
					{
						int32	index;
						msg->FindInt32("data", &index);
						child	= ChildAt( index );
						break;
					}	
					case B_REVERSE_INDEX_SPECIFIER:
					{
						int32	rindex;
						msg->FindInt32("data", &rindex);
						child	= ChildAt( CountChildren() - rindex );
						break;
					}	
					case B_NAME_SPECIFIER:
					{
						const char	*name;
						msg->FindString("data", &name);
						child	= FindView( name );
						delete name;
						break;
					}	
					default:
					{
						child	= NULL;
						break;
					}
				}
				
				if ( child != NULL )
				{
					msg->PopSpecifier();
					return child;
				}
				else
				{
					BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
					replyMsg.AddInt32( "error", B_BAD_INDEX );
					replyMsg.AddString( "message", "Cannot find view at/with specified index/name.");
					msg->SendReply( &replyMsg );
					return NULL;
				}
			}
			else
			{
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32( "error", B_NAME_NOT_FOUND );
				replyMsg.AddString( "message", "This window doesn't have children.");
				msg->SendReply( &replyMsg );
				return NULL;
			}
			break;
		}
		default:
			break;
	}
	return BHandler::ResolveSpecifier(msg, index, specifier, what, property);
}

//---------------------------------------------------------------------------
void BView::MessageReceived( BMessage *msg )
{ 
	BMessage			specifier;
	int32				what;
	const char*			prop;
	int32				index;
	status_t			err;

	if (!msg->HasSpecifiers())
	{
		BHandler::MessageReceived( msg );
		return;		
	}
	
	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK)
	{
		BMessage			replyMsg;

		switch (msg->what)
		{
			case B_GET_PROPERTY:
			{
				replyMsg.what		= B_NO_ERROR;
				replyMsg.AddInt32( "error", B_OK );
				
				if (strcmp(prop, "Frame") ==0 )
					replyMsg.AddRect( "result", Frame());				
				else 
				if (strcmp(prop, "Hidden") ==0 )
					replyMsg.AddBool( "result", IsHidden());				
		
				break;
			}
			case B_SET_PROPERTY:
			{
				if (strcmp(prop, "Frame") ==0 )
				{
					BRect			newFrame;
					if (msg->FindRect( "data", &newFrame ) == B_OK)
					{
						MoveTo( newFrame.LeftTop() );
						ResizeTo( newFrame.right, newFrame.bottom);
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else
					{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				else
				if (strcmp(prop, "Hidden") ==0 )
				{
					bool newHiddenState;
					if (msg->FindBool( "data", &newHiddenState ) == B_OK)
					{
						if ( !IsHidden() && newHiddenState == true )
						{
							Hide();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
							
						}
						else
						if ( IsHidden() && newHiddenState == false )
						{
							Show();
							
							replyMsg.what = B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
						}
						else
						{
							replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
							replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
							replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
						}
					}
					else
					{
						replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				break;
			}
			case B_COUNT_PROPERTIES:
			{
				if (strcmp(prop, "View") ==0 )
				{
					replyMsg.what		= B_NO_ERROR;
					replyMsg.AddInt32( "error", B_OK );
					replyMsg.AddInt32( "result", CountChildren());
				}
				break;	
			}
		}

		msg->SendReply( &replyMsg );

	}
	else
	{
		BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32( "error" , B_BAD_SCRIPT_SYNTAX );
		replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
		
		msg->SendReply( &replyMsg );
	} // end if(err==B_OK)

} 

//---------------------------------------------------------------------------

status_t BView::Perform(perform_code d, void* arg)
{
	return B_BAD_VALUE;
}

//---------------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------------

void BView::InitData(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
{
	// Info: The name of the view is set by BHandler constructor
	
	STRACE(("BView::InitData: enter\n"));
	
	// initialize members		
	fFlags				= (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);

	originX				= frame.left;
	originY				= frame.top;

	owner				= NULL;
	parent				= NULL;
	next_sibling		= NULL;
	prev_sibling		= NULL;
	first_child			= NULL;

	fShowLevel			= 0;
	top_level_view		= false;

	cpicture			= NULL;
	comm				= NULL;

	fVerScroller		= NULL;
	fHorScroller		= NULL;

	f_is_printing		= false;

	fPermanentState		= NULL;
	fState				= new ViewAttr;

	fBounds				= frame.OffsetToCopy(0.0, 0.0);
	fShelf				= NULL;
	pr_state			= NULL;

	fEventMask			= 0;
	fEventOptions		= 0;
	
	// call initialization methods.
	initCachedState();
}

//---------------------------------------------------------------------------

void BView::removeCommArray()
{
	if( comm )
	{
		delete [] comm->array;
		delete comm;
		comm	= NULL;
	}
}

//---------------------------------------------------------------------------

void BView::setOwner(BWindow *theOwner)
{
	if (!theOwner)
		removeCommArray();
	 
	if (owner != theOwner && owner)
	{
		if (owner->fFocus == this)
			MakeFocus( false );

		if (owner->fLastMouseMovedView == this)
			owner->fLastMouseMovedView = NULL;
		 
		owner->RemoveHandler(this);
		if (fShelf)
			owner->RemoveHandler(fShelf);
	}
	 
	if (theOwner && theOwner != owner)
	{
		theOwner->AddHandler(this);
		if (fShelf)
			theOwner->AddHandler(fShelf);
		 
		if (top_level_view)
			SetNextHandler(theOwner);
		else
			SetNextHandler(parent);
	}
	 
	owner = theOwner;
	 
	for (BView *child = first_child; child != NULL; child = child->next_sibling)
		child->setOwner(theOwner);
}

//---------------------------------------------------------------------------

bool BView::removeSelf()
{
	STRACE(("BView(%s)::removeSelf()...\n", this->Name() ));
	
/*
	# check for dirty flags 						- by updateCachedState()
	# check for dirty flags on 'child' children		- by updateCachedState()
	# handle if in middle of Begin/EndLineArray()	- by setOwner(NULL)
	# remove trom the main tree						- by removeFromList()
	# handle if child is the default button			- HERE
	# handle if child is the focus view				- by setOwner(NULL)
	# handle if child is the menu bar				- HERE
	# handle if child token is = fLastViewToken		- by setOwner(NULL)
	# contact app_server							- HERE
	# set a new owner = NULL						- by setOwner(NULL)
*/
 	bool		returnValue = true;

	if (!parent)
	{
		STRACE(("BView(%s)::removeSelf()... NO parent\n", this->Name()));
		return false;
	}

	if (owner)
	{
		check_lock();
	
		updateCachedState();
		
		if (owner->fDefaultButton == this)
			owner->SetDefaultButton( NULL );
		
		if (owner->fKeyMenuBar == this)
			owner->fKeyMenuBar = NULL;
		
		if (owner->fLastViewToken == _get_object_token_(this))
			owner->fLastViewToken = B_NULL_TOKEN;
	
		callDetachHooks( this );

		BWindow			*ownerZ = owner;

		setOwner( NULL );
			
		ownerZ->fLink->StartMessage( AS_LAYER_DELETE );
	}

	returnValue		= removeFromList();
	
	parent			= NULL;
	next_sibling	= NULL;
	prev_sibling	= NULL;

	STRACE(("DONE: BView(%s)::removeSelf()\n", this->Name()));
	
	return returnValue;
}

//---------------------------------------------------------------------------

bool BView::callDetachHooks( BView *aView )
{
//	check_clock();

	// call the hook function:
	aView->DetachedFromWindow();
	
	// we attach all its children
	BView		*child;
	child		= aView->first_child;
	
	while( child ) 
	{
		aView->callDetachHooks(child);
		child		= child->next_sibling;
	}
	
	// call the hook function:
	aView->AllDetached();

	return true;
}

//---------------------------------------------------------------------------

bool BView::removeFromList()
{

	if (parent->first_child == this)
	{
		parent->first_child = next_sibling;
		 
		if (next_sibling)
			next_sibling->prev_sibling	= NULL;
	}
	else
	{
		prev_sibling->next_sibling	= next_sibling;

		if (next_sibling)
			next_sibling->prev_sibling	= prev_sibling;
	}
	return true;
}

//---------------------------------------------------------------------------

bool BView::addToList(BView *aView, BView *before)
{
	if ( !aView )
		return false;

	BView		*current 	= first_child;
	BView		*last		= current;
	
	while( current && current != before)
	{
		last		= current;
		current		= current->next_sibling;
	}
	
	if( !current && before )
		return false;
	
	// we're at begining of the list, OR between two elements
	if( current )
	{
		if ( current == first_child )
		{
			aView->next_sibling		= current;
			current->prev_sibling	= aView;
			first_child				= aView;
		}
		else
		{
			aView->next_sibling		= current;
			aView->prev_sibling		= current->prev_sibling;
			current->prev_sibling->next_sibling		= aView;
			current->prev_sibling	= aView;
		}
	}
	else
	{
		// we have reached the end of the list
		
		// if last!=NULL then we add to the end. Otherwise, aView is the
		// first chiild in the list
		if ( last )
		{
			last->next_sibling		= aView;
			aView->prev_sibling		= last;
		}
		else
			first_child		= aView;
	}
	
	aView->parent = this;
	
	return true;
}

//---------------------------------------------------------------------------

bool BView::attachView(BView *aView)
{
	// LEAVE the following line commented!!!
//	check_lock();

/*
	INFO:
	
	'check_lock()' checks for a lock on the window and then, sets
	 	BWindow::fLastViewToken to the one of the view which called check_lock(),
	 	and sends it to the app_server to be the view for which current actions
 		are made.
*/
	
	if (aView->top_level_view)
		owner->fLink->StartMessage( AS_LAYER_CREATE_ROOT );
	else
 		owner->fLink->StartMessage( AS_LAYER_CREATE );
	owner->fLink->Attach<int32>( _get_object_token_( aView ) );
	owner->fLink->AttachString( aView->Name() );
	owner->fLink->Attach<BRect>( aView->Frame() );
	owner->fLink->Attach<uint32>( aView->ResizingMode() );
	owner->fLink->Attach<uint32>( aView->Flags() );
	owner->fLink->Attach<bool>( aView->IsHidden(aView) );
	owner->fLink->Attach<int32>( aView->CountChildren() );
	
	aView->setCachedState();
	
	// call the hook function:
	aView->AttachedToWindow();
	
	// we attach all its children
	BView		*child;
	child		= aView->first_child;
	
	while( child ) 
	{
		aView->attachView(child);
		child		= child->next_sibling;
	}
	
	// call the hook function:
	aView->AllAttached();
	owner->fLink->Flush();
	
	return true;
}

//---------------------------------------------------------------------------

void BView::deleteView( BView* aView)
{

	BView		*child;
	child		= aView->first_child;
	
	while( child ) 
	{
		deleteView(child);
		child		= child->next_sibling;
	}
	
	delete aView;
}

//---------------------------------------------------------------------------

BView* BView::findView(const BView* aView, const char* viewName) const
{
	if ( strcmp( viewName, aView->Name() ) == 0)
		return const_cast<BView*>(aView);

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child ) 
		{
			BView*		view = NULL;
			if ( (view = findView( child, viewName )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------

void BView::setCachedState()
{
	setFontState( &(fState->font), fState->fontFlags );
	
	owner->fLink->StartMessage( AS_LAYER_SET_STATE );
	owner->fLink->Attach<BPoint>( fState->penPosition );
	owner->fLink->Attach<float>( fState->penSize );
	owner->fLink->Attach<rgb_color>( fState->highColor );
	owner->fLink->Attach<rgb_color>( fState->lowColor );
	owner->fLink->Attach<rgb_color>( fState->viewColor );
	owner->fLink->Attach<pattern>( fState->patt );	
	owner->fLink->Attach<int8>( (int8)fState->drawingMode );
	owner->fLink->Attach<BPoint>( fState->coordSysOrigin );
	owner->fLink->Attach<int8>( (int8)fState->lineJoin );
	owner->fLink->Attach<int8>( (int8)fState->lineCap );
	owner->fLink->Attach<float>( fState->miterLimit );
	owner->fLink->Attach<int8>( (int8)fState->alphaSrcMode );
	owner->fLink->Attach<int8>( (int8)fState->alphaFncMode );
	owner->fLink->Attach<float>( fState->scale );
	owner->fLink->Attach<bool>( fState->fontAliasing );
	
	// we send the 'local' clipping region... if we have one...
	int32		noOfRects	= fState->clippingRegion.CountRects();
	
	owner->fLink->Attach<int32>( noOfRects );
	for (int32 i = 0; i < noOfRects; i++)
		owner->fLink->Attach<BRect>( fState->clippingRegion.RectAt(i) );

	//	Although we might have a 'local' clipping region, when we call
	//	BView::GetClippingRegion() we ask for the 'global' one and it
	//	is kept on server, so we must invalidate B_VIEW_CLIP_REGION_BIT flag
	
	fState->flags			= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::setFontState(const BFont* font, uint16 mask)
{
	do_owner_check();
	
	owner->fLink->StartMessage( AS_LAYER_SET_FONT_STATE );
	owner->fLink->Attach<uint16>( mask );

	// always present.
	if ( mask & B_FONT_FAMILY_AND_STYLE )
	{
		uint32		fontID;
		fontID		= font->FamilyAndStyle( );
		
		owner->fLink->Attach<uint32>( fontID );
	}
	
	if ( mask & B_FONT_SIZE )
		owner->fLink->Attach<float>( font->Size() );

	if ( mask & B_FONT_SHEAR )
		owner->fLink->Attach<float>( font->Shear() );

	if ( mask & B_FONT_ROTATION )
		owner->fLink->Attach<float>( font->Rotation() );

	if ( mask & B_FONT_SPACING )
		owner->fLink->Attach<uint8>( font->Spacing() );	// uint8

	if ( mask & B_FONT_ENCODING )
		owner->fLink->Attach<uint8>( font->Encoding() ); // uint8

	if ( mask & B_FONT_FACE )
		owner->fLink->Attach<uint16>( font->Face() );	// uint16
	
	if ( mask & B_FONT_FLAGS )
		owner->fLink->Attach<uint32>( font->Flags() ); // uint32
}

//---------------------------------------------------------------------------

BShelf *BView::shelf() const
{
	return fShelf;
}

//---------------------------------------------------------------------------

void BView::set_shelf(BShelf *shelf)
{
	// TODO: is this all that needs done?
	fShelf=shelf;
}

//---------------------------------------------------------------------------

void BView::initCachedState()
{
	fState->font			= *be_plain_font;

	fState->penPosition.Set( 0.0, 0.0 );
	fState->penSize			= 1.0;

	fState->highColor.red	= 0;
	fState->highColor.blue	= 0;
	fState->highColor.green	= 0;
	fState->highColor.alpha	= 255;

	fState->lowColor.red	= 255;
	fState->lowColor.blue	= 255;
	fState->lowColor.green	= 255;
	fState->lowColor.alpha	= 255;

	fState->viewColor.red	= 255;
	fState->viewColor.blue	= 255;
	fState->viewColor.green	= 255;
	fState->viewColor.alpha	= 255;
	
	fState->patt			= B_SOLID_HIGH;

	fState->drawingMode		= B_OP_COPY;

	// clippingRegion is empty by default

	fState->coordSysOrigin.Set( 0.0, 0.0 );

	fState->lineCap			= B_BUTT_CAP;
	fState->lineJoin		= B_BEVEL_JOIN;
	fState->miterLimit		= B_DEFAULT_MITER_LIMIT;

	fState->alphaSrcMode	= B_PIXEL_ALPHA;
	fState->alphaFncMode	= B_ALPHA_OVERLAY;

	fState->scale			= 1.0;

	fState->fontAliasing	= false;
	
/*
	INFO: We include(invalidate) only B_VIEW_CLIP_REGION_BIT flag
	because we should get the clipping region from app_server.
	The other flags do not need to be included because the data they
	represent is already in sync with app_server - app_server uses the
	same init(default) values.
*/
	fState->flags			= B_VIEW_CLIP_REGION_BIT;
	
	// (default) flags used to determine witch fields to archive
	fState->archivingFlags	= B_VIEW_COORD_BIT;
}

//---------------------------------------------------------------------------
void BView::updateCachedState()
{
	STRACE(("BView(%s)::updateCachedState()\n", Name() ));

	// fail if we do not have an owner
 	do_owner_check();
 	
 	owner->fLink->StartMessage( AS_LAYER_GET_STATE );
 	owner->fLink->Flush();

	int32 rCode = SERVER_FALSE;
	owner->fLink->GetNextReply( &rCode );

	if (rCode != SERVER_TRUE)
		return;

	uint32		fontID;
	float		size;
	float		shear;
	float		rotation;
	uint8		spacing;
	uint8		encoding;
	uint16		face;
	uint32		flags;
	int32		noOfRects;
	BRect		rect;	
	
	// read and set the font state
	owner->fLink->Read<int32>( (int32*)&fontID );
	owner->fLink->Read<float>( &size );
	owner->fLink->Read<float>( &shear );
	owner->fLink->Read<float>( &rotation );
	owner->fLink->Read<int8>( (int8*)&spacing );
	owner->fLink->Read<int8>( (int8*)&encoding );
	owner->fLink->Read<int16>( (int16*)&face );
	owner->fLink->Read<int32>( (int32*)&flags );

	fState->fontFlags		= B_FONT_ALL;
	fState->font.SetFamilyAndStyle( fontID );
	fState->font.SetSize( size );
	fState->font.SetShear( shear );
	fState->font.SetRotation( rotation );
	fState->font.SetSpacing( spacing );
	fState->font.SetEncoding( encoding );
	fState->font.SetFace( face );
	fState->font.SetFlags( flags );

	// read and set view's state						 	
	owner->fLink->Read<BPoint>( &(fState->penPosition) );
	owner->fLink->Read<float>( &(fState->penSize) );	
	owner->fLink->Read<rgb_color>( &(fState->highColor) );
	owner->fLink->Read<rgb_color>( &(fState->lowColor) );
	owner->fLink->Read<rgb_color>( &(fState->viewColor) );
	owner->fLink->Read<pattern>( &(fState->patt) );
	owner->fLink->Read<BPoint>( &(fState->coordSysOrigin) );	
	owner->fLink->Read<int8>( (int8*)&(fState->drawingMode) );
	owner->fLink->Read<int8>( (int8*)&(fState->lineCap) );
	owner->fLink->Read<int8>( (int8*)&(fState->lineJoin) );
	owner->fLink->Read<float>( &(fState->miterLimit) );
	owner->fLink->Read<int8>( (int8*)&(fState->alphaSrcMode) );
	owner->fLink->Read<int8>( (int8*)&(fState->alphaFncMode) );
	owner->fLink->Read<float>( &(fState->scale) );
	owner->fLink->Read<bool>( &(fState->fontAliasing) );

	owner->fLink->Read<int32>( &noOfRects );
		
	fState->clippingRegion.MakeEmpty();
	for (int32 i = 0; i < noOfRects; i++)
	{
		owner->fLink->Read<BRect>( &rect );
		fState->clippingRegion.Include( rect );
	}
	
	owner->fLink->Read<float>( &originX );
	owner->fLink->Read<float>( &originY );
	owner->fLink->Read<BRect>( &fBounds );

	fState->flags			= B_VIEW_CLIP_REGION_BIT;
	
	STRACE(("BView(%s)::updateCachedState() - DONE\n", Name() ));	
}

//---------------------------------------------------------------------------

status_t BView::setViewImage(const BBitmap *bitmap, BRect srcRect,
        BRect dstRect, uint32 followFlags, uint32 options)
{
	if (!do_owner_check())
		return B_ERROR;
	 
	int32		serverToken = bitmap ? bitmap->get_server_token() : -1;
	status_t	err;
	 
	owner->fLink->StartMessage( AS_LAYER_SET_VIEW_IMAGE );
	owner->fLink->Attach<int32>( serverToken );
	owner->fLink->Attach<BRect>( srcRect );
	owner->fLink->Attach<BRect>( dstRect );
	owner->fLink->Attach<int32>( followFlags );
	owner->fLink->Attach<int32>( options );
	owner->fLink->Flush();
	
	
	// TODO: this needs fixed between here and the server.
	// The server should return whatever error code is needed, whether it
	// is B_OK or whatever, not SERVER_TRUE.
	
	int32 rCode = SERVER_FALSE;
	owner->fLink->GetNextReply( &rCode );
	if (rCode != SERVER_TRUE)
		return B_ERROR;
	
	owner->fLink->Read<status_t>( &err );
	 
	return err;
}
 
//---------------------------------------------------------------------------

void BView::SetPattern(pattern pat)
{
	if (owner)
	{
		check_lock();

		owner->fLink->StartMessage( AS_LAYER_SET_PATTERN );
		owner->fLink->Attach<pattern>( pat );
	}
	
	fState->patt		= pat;
}
 
//---------------------------------------------------------------------------

bool BView::do_owner_check() const
{
	STRACE(("BView(%s)::do_owner_check()...", Name()));
	int32			serverToken = _get_object_token_(this);

	if(!owner)
	{
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	owner->AssertLocked();

	if (owner->fLastViewToken != serverToken)
	{
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		owner->fLink->StartMessage( AS_SET_CURRENT_LAYER );
		owner->fLink->Attach<int32>( serverToken );

		owner->fLastViewToken = serverToken;
	}
	else
		STRACE(("this is the lastViewToken\n"));

	return true;
}

//---------------------------------------------------------------------------

void BView::check_lock() const
{
	STRACE(("BView(%s)::check_lock()...", Name() ? Name(): "NULL"));
	int32			serverToken = _get_object_token_(this);
	
	if (!owner)
	{
		STRACE(("quiet1\n"));
		return;
	}

	owner->AssertLocked();

	if (owner->fLastViewToken != serverToken)
	{
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		owner->fLink->StartMessage( AS_SET_CURRENT_LAYER );
		owner->fLink->Attach<int32>( serverToken );

		owner->fLastViewToken = serverToken;
	}
	else
	{
		STRACE(("quiet2\n"));
	}
}

//---------------------------------------------------------------------------

void BView::check_lock_no_pick() const
{
	if (owner)
		owner->AssertLocked();
}

//---------------------------------------------------------------------------

bool BView::do_owner_check_no_pick() const
{
	if (owner)
	{
		owner->AssertLocked();
		return true;
	}
	else
	{
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}

//---------------------------------------------------------------------------

void BView::_ReservedView2(){}
void BView::_ReservedView3(){}
void BView::_ReservedView4(){}
void BView::_ReservedView5(){}
void BView::_ReservedView6(){}
void BView::_ReservedView7(){}
void BView::_ReservedView8(){}

#if !_PR3_COMPATIBLE_
void BView::_ReservedView9(){}
void BView::_ReservedView10(){}
void BView::_ReservedView11(){}
void BView::_ReservedView12(){}
void BView::_ReservedView13(){}
void BView::_ReservedView14(){}
void BView::_ReservedView15(){}
void BView::_ReservedView16(){}
#endif


//---------------------------------------------------------------------------

inline rgb_color _get_rgb_color( uint32 color )
{
	rgb_color		c;
	c.red			= (color & 0xFF000000) >> 24;
	c.green			= (color & 0x00FF0000) >> 16;
	c.blue			= (color & 0x0000FF00) >> 8;
	c.alpha			= (color & 0x000000FF);

	return c;
}

//---------------------------------------------------------------------------

inline uint32 _get_uint32_color( rgb_color c )
{
	uint32			color;
	color			= (c.red << 24) +
					  (c.green << 16) +
					  (c.blue << 8) +
					  c.alpha;
	return color;
}

//---------------------------------------------------------------------------

inline rgb_color _set_static_rgb_color( uint8 r, uint8 g, uint8 b, uint8 a )
{
	rgb_color		color;
	color.red		= r;
	color.green		= g;
	color.blue		= b;
	color.alpha		= a;

	return color;
}

//---------------------------------------------------------------------------

inline void _set_ptr_rgb_color( rgb_color* c, uint8 r, uint8 g,uint8 b, uint8 a )
{
	c->red			= r;
	c->green		= g;
	c->blue			= b;
	c->alpha		= a;
}

//---------------------------------------------------------------------------

inline bool _rgb_color_are_equal( rgb_color c1, rgb_color c2 )
{
	return _get_uint32_color( c1 ) == _get_uint32_color( c2 );
}

//---------------------------------------------------------------------------

inline bool _is_new_pattern( const pattern& p1, const pattern& p2 )
{
	if ( memcmp( &p1, &p2, sizeof(pattern) ) == 0 )
		return false;
	else
		return true;
}

//---------------------------------------------------------------------------

void BView::PrintToStream()
{
	printf("BView::PrintToStream()\n");
	printf("\tName: %s\
\tParent: %s\
\tFirstChild: %s\
\tNextSibling: %s\
\tPrevSibling: %s\
\tOwner(Window): %s\
\tToken: %ld\
\tFlags: %ld\
\tView origin: (%f,%f)\
\tView Bounds rectangle: (%f,%f,%f,%f)\
\tShow level: %d\
\tTopView?: %s\
\tBPicture: %s\
\tVertical Scrollbar %s\
\tHorizontal Scrollbar %s\
\tIs Printing?: %s\
\tShelf?: %s\
\tEventMask: %ld\
\tEventOptions: %ld\n",
	Name(),
	parent? parent->Name() : "NULL",
	first_child? first_child->Name() : "NULL",
	next_sibling? next_sibling->Name() : "NULL",
	prev_sibling? prev_sibling->Name() : "NULL",
	owner? owner->Name() : "NULL",
	_get_object_token_(this),
	fFlags,
	originX, originY,
	fBounds.left, fBounds.top, fBounds.right, fBounds.bottom,
	fShowLevel,
	top_level_view? "YES" : "NO",
	cpicture? "YES" : "NULL",
	fVerScroller? "YES" : "NULL",
	fHorScroller? "YES" : "NULL",
	f_is_printing? "YES" : "NO",
	fShelf? "YES" : "NO",
	fEventMask,
	fEventOptions);
	
	printf("\tState status:\
\t\tLocalCoordianteSystem: (%f,%f)\
\t\tPenLocation: (%f,%f)\
\t\tPenSize: %f\
\t\tHighColor: [%d,%d,%d,%d]\
\t\tLowColor: [%d,%d,%d,%d]\
\t\tViewColor: [%d,%d,%d,%d]\
\t\tPattern: %llx\
\t\tDrawingMode: %d\
\t\tLineJoinMode: %d\
\t\tLineCapMode: %d\
\t\tMiterLimit: %f\
\t\tAlphaSource: %d\
\t\tAlphaFuntion: %d\
\t\tScale: %f\
\t\t(Print)FontAliasing: %s\
\t\tFont Info:\n",
	fState->coordSysOrigin.x, fState->coordSysOrigin.y,
	fState->penPosition.x, fState->penPosition.y,
	fState->penSize,
	fState->highColor.red, fState->highColor.blue, fState->highColor.green, fState->highColor.alpha,
	fState->lowColor.red, fState->lowColor.blue, fState->lowColor.green, fState->lowColor.alpha,
	fState->viewColor.red, fState->viewColor.blue, fState->viewColor.green, fState->viewColor.alpha,
	*((uint64*)&(fState->patt)),
	fState->drawingMode,
	fState->lineJoin,
	fState->lineCap,
	fState->miterLimit,
	fState->alphaSrcMode,
	fState->alphaFncMode,
	fState->scale,
	fState->fontAliasing? "YES" : "NO");

	fState->font.PrintToStream();
	
	// TODO: also print the line array.
}

//---------------------------------------------------------------------------

void BView::PrintTree()
{
	int32		spaces = 2;
	BView		*c = first_child; //c = short for: current
	printf( "'%s'\n", Name() );
	if( c != NULL )
	{
		while( true )
		{
			// action block
			{
				for( int i = 0; i < spaces; i++)
					printf(" ");
				
				printf( "'%s'\n", c->Name() );
			}
			
			// go deep
			if(	c->first_child )
			{
				c = c->first_child;
				spaces += 2;
			}
			else
			{
				// go right
				if( c->next_sibling )
				{
					c = c->next_sibling;
				}
				else
				{
					// go up
					while( !c->parent->next_sibling && c->parent != this )
					{
						c = c->parent;
						spaces -= 2;
					}
					
					// that enough! We've reached this view.
					if( c->parent == this )
						break;
						
					c = c->parent->next_sibling;
					spaces -= 2;
				}
			}
		}
	}
}

ViewAttr::ViewAttr(void)
{
	font=*be_plain_font;
	fontFlags=font.Flags();

	penPosition.Set(0,0);
	penSize=1.0;
	
	// This probably needs to be set to bounds by the owning BView
	clippingRegion.MakeEmpty();
	
	SetRGBColor(&highColor,0,0,0);
	SetRGBColor(&lowColor,255,255,255);
	SetRGBColor(&viewColor,255,255,255);
	
	patt=B_SOLID_HIGH;
	drawingMode=B_OP_COPY;
	
	coordSysOrigin.Set(0,0);

	lineJoin=B_BUTT_JOIN;
	lineCap=B_BUTT_CAP;
	miterLimit=B_DEFAULT_MITER_LIMIT;

	alphaSrcMode=B_CONSTANT_ALPHA;
	alphaFncMode=B_ALPHA_OVERLAY;
	
	scale=1.0;
	fontAliasing=false;

	// flags used for synchronization with app_server
	// TODO: set this to the default value, whatever that is
	flags=B_VIEW_CLIP_REGION_BIT;
	
	// TODO: find out what value this should have.
	archivingFlags=B_VIEW_COORD_BIT;
}

//---------------------------------------------------------------------------

/* TODO:
 	-implement SetDiskMode(). What's with this method? What does it do? test!
 		does it has something to do with DrawPictureAsync( filename* .. )?
	-implement DrawAfterChildren()
*/
