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

// Project Includes ------------------------------------------------------------
#include <AppMisc.h>
#include <ViewAux.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <Session.h>
#include <ServerProtocol.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
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
/*
NOTE: this flag is not used any more
	uint32				dBuf;
	if ( archive->FindInt32("_dbuf", (int32*)&dBuf) == B_OK ){
	}
*/
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

BArchivable* BView::Instantiate(BMessage* data){
   if ( !validate_instantiation( data , "BView" ) ) 
      return NULL; 
   return new BView(data); 
}

//---------------------------------------------------------------------------

status_t BView::Archive(BMessage* data, bool deep = true) const{
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
/*
NOTE: we do not use this flag any more
	if ( 1 ){
		data->AddInt32("_dbuf", 1);
	}
*/
	if ( fState->archivingFlags & B_VIEW_ORIGIN_BIT )
		data->AddPoint("_origin", Origin());

	if ( fState->archivingFlags & B_VIEW_PEN_SIZE_BIT )
		data->AddFloat("_psize", PenSize());

	if ( fState->archivingFlags & B_VIEW_PEN_LOC_BIT )
		data->AddPoint("_ploc", PenLocation());

	if ( fState->archivingFlags & B_VIEW_LINE_MODES_BIT ){
		data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if ( fState->archivingFlags & B_VIEW_BLENDING_BIT ){
		source_alpha		alphaSrcMode;
		alpha_function		alphaFncMode;
		GetBlendingMode( &alphaSrcMode, &alphaFncMode);

		data->AddInt16("_blend", (int16)alphaSrcMode);
		data->AddInt16("_blend", (int16)alphaFncMode);
	}

	if ( fState->archivingFlags & B_VIEW_DRAW_MODE_BIT )
		data->AddInt32("_dmod", DrawingMode());

	if (deep){
		int					i		= 0;
		BView				*child	= NULL;

		while ( (child = ChildAt(i++)) != NULL){ 
			BMessage		childArchive;

			retval			= child->Archive( &childArchive, deep );
			if (retval == B_OK)
				data->AddMessage( "_views", &childArchive );
		}
	}
}

//---------------------------------------------------------------------------

BView::~BView(){
	if (owner)
		debugger("Trying to delete a view that belongs to a window. Call RemoveSelf first.");

	removeSelf();

// TODO: see about BShelf! must I delete it here? is it deleted by the window?

		// we also delete all its childern
	BView		*child, *_child;
	
	child		= first_child;
	while(child){
		_child		= child;
		child		= child->next_sibling;
		deleteView( _child );
	}

	if (fVerScroller){
		fVerScroller->SetTarget( (const char*)NULL );
	}
	if (fHorScroller){
		fHorScroller->SetTarget( (const char*)NULL );
	}

	SetName( NULL );

	if (fPermanentState)
		delete fPermanentState;
	if (fState)
		delete fState;

	free( pr_state );
	pr_state	= NULL;
}

//---------------------------------------------------------------------------

BRect BView::Bounds() const{
		// if we have the actual coordiantes
	if (fState->flags & B_VIEW_COORD_BIT)
	if (owner){
		check_lock();

		owner->session->WriteInt32( AS_LAYER_GET_COORD );
		owner->session->Sync();
		
		owner->session->ReadFloat( const_cast<float*>(&originX) );
		owner->session->ReadFloat( const_cast<float*>(&originY) );
		owner->session->ReadRect( const_cast<BRect*>(&fBounds) );

		fState->flags		&= ~B_VIEW_COORD_BIT;
	}

	return fBounds;
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BPoint* pt) const{
	check_lock_no_pick();

	pt->x			+= originX;
	pt->y			+= originY;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToParent(BPoint pt) const{
	check_lock_no_pick();
	
	BPoint			p;
	p.x				= pt.x + originX;
	p.y				= pt.y + originY;

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BPoint* pt) const{
	check_lock_no_pick();
	
	pt->x			-= originX;
	pt->y			-= originY;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromParent(BPoint pt) const{
	check_lock_no_pick();
	
	BPoint			p;
	p.x				= pt.x - originX;
	p.y				= pt.y - originY;

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BRect* r) const{
	check_lock_no_pick();
	
	r->OffsetBy(originX, originY);
}

//---------------------------------------------------------------------------

BRect BView::ConvertToParent(BRect r) const{
	check_lock_no_pick();
	
	return r.OffsetByCopy(originX, originY);
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BRect* r) const{
	check_lock_no_pick();
	
	r->OffsetBy(-originX, -originY);
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromParent(BRect r) const{
	check_lock_no_pick();

	return r.OffsetByCopy(-originX, -originY);
}

//---------------------------------------------------------------------------



void BView::ConvertToScreen(BPoint* pt) const{
	do_owner_check_no_pick();

	if (top_level_view)
		Window()->ConvertToScreen( pt );
	else {
		ConvertToParent( pt );
		parent->ConvertToScreen( pt );
	}
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToScreen(BPoint pt) const{
	do_owner_check_no_pick();

	BPoint			p;

	if (top_level_view)
		p			= Window()->ConvertToScreen( pt );
	else {
		p			= ConvertToParent( pt );
		p			= parent->ConvertToScreen( p );
	}

	return p;
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BPoint* pt) const{
	do_owner_check_no_pick();

	if (top_level_view)
		Window()->ConvertFromScreen( pt );
	else {
		ConvertFromParent( pt );
		parent->ConvertFromScreen( pt );
	}
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromScreen(BPoint pt) const{
	do_owner_check_no_pick();

	BPoint			p;

	if (top_level_view)
		p			= Window()->ConvertFromScreen( pt );
	else {
		p			= ConvertFromParent( pt );
		p			= parent->ConvertFromScreen( p );
	}

	return p;
}

//---------------------------------------------------------------------------


void BView::ConvertToScreen(BRect* r) const{
	do_owner_check_no_pick();

	if (top_level_view)
		Window()->ConvertToScreen( r );
	else {
		ConvertToParent( r );
		parent->ConvertToScreen( r );
	}
}

//---------------------------------------------------------------------------

BRect BView::ConvertToScreen(BRect r) const{
	do_owner_check_no_pick();

	BRect			rect;

	if (top_level_view)
		rect		= Window()->ConvertToScreen( r );
	else {
		rect		= ConvertToParent( r );
		rect		= parent->ConvertToScreen( rect );
	}

	return rect;
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BRect* r) const{
	do_owner_check_no_pick();

	if (top_level_view)
		Window()->ConvertFromScreen( r );
	else {
		ConvertFromParent( r );
		parent->ConvertFromScreen( r );
	}
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromScreen(BRect r) const{
	do_owner_check_no_pick();

	BRect			rect;

	if (top_level_view)
		rect		= Window()->ConvertFromScreen( r );
	else {
		rect		= ConvertFromParent( r );
		rect		= parent->ConvertFromScreen( rect );
	}

	return rect;
}

//---------------------------------------------------------------------------

uint32 BView::Flags() const {
	check_lock_no_pick();
	return ( fFlags & ~_RESIZE_MASK_ );
}

//---------------------------------------------------------------------------

void BView::SetFlags( uint32 flags ){
	if (Flags() == flags)
		return;
		
	if (owner){
		if ( flags & B_PULSE_NEEDED ){
			check_lock_no_pick();
			if ( !owner->fPulseEnabled )
				owner->SetPulseRate( 500000 );
		}
			
		if ( flags & (	B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE |
						B_FRAME_EVENTS | B_SUBPIXEL_PRECISE ) )
		{
			check_lock();
			
			owner->session->WriteInt32( AS_LAYER_SET_FLAGS );
			owner->session->WriteInt32( flags );
		}
	}
			
/*	Some usefull info:
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

BRect BView::Frame() const {
	check_lock_no_pick();
	
	if ( fState->flags & B_VIEW_COORD_BIT ){
		Bounds();
	}

	return Bounds().OffsetToCopy( originX, originY );
}

//---------------------------------------------------------------------------

void BView::Hide(){
// TODO: You may hide it by relocating with -17000 coord units to the left???
	if ( owner && fShowLevel == 0){
		check_lock();
			
		owner->session->WriteInt32( AS_LAYER_HIDE );
	}

	fShowLevel++;
}

//---------------------------------------------------------------------------

void BView::Show(){

	fShowLevel--;

	if (owner && fShowLevel == 0){
		check_lock();
			
		owner->session->WriteInt32( AS_LAYER_SHOW );
	}
}

//---------------------------------------------------------------------------

bool BView::IsFocus() const {
	if (owner){
		check_lock_no_pick();
		return owner->CurrentFocus() == this;
	}
	else
		return false;
}

//---------------------------------------------------------------------------

bool BView::IsHidden() const{
	if (fShowLevel > 0)
		return true;

	if (owner)
		if (owner->IsHidden())
			return true;

	if (parent)
		return parent->IsHidden();

	return false;
}

//---------------------------------------------------------------------------

bool BView::IsPrinting() const {
	return f_is_printing;
}

//---------------------------------------------------------------------------

BPoint BView::LeftTop() const {
	return Bounds().LeftTop();
}

//---------------------------------------------------------------------------

void BView::SetOrigin(BPoint pt) {
	SetOrigin( pt.x, pt.y );
}

//---------------------------------------------------------------------------

void BView::SetOrigin(float x, float y) {
// TODO: maybe app_server should do a redraw? - WRITE down into specs

	if ( x==originX && y==originY )
		return;
		
	if (do_owner_check()){
		owner->session->WriteInt32( AS_LAYER_SET_ORIGIN );
		owner->session->WriteFloat( x );
		owner->session->WriteFloat( y );
	}

		// invalidate this flag, to stay in sync with app_server
	fState->flags			|= B_VIEW_ORIGIN_BIT;	
	
		// our local coord system origin has changed, so when archiving we'll add this too
	fState->archivingFlags	|= B_VIEW_ORIGIN_BIT;			
}

//---------------------------------------------------------------------------

BPoint BView::Origin(void) const {
	
	if ( fState->flags & B_VIEW_ORIGIN_BIT ) {
			do_owner_check();

			owner->session->WriteInt32( AS_LAYER_GET_ORIGIN );
			owner->session->Sync();
			
			owner->session->ReadPoint( &fState->coordSysOrigin );
	
			fState->flags			&= ~B_VIEW_ORIGIN_BIT;			
	}

	return fState->coordSysOrigin;
}

//---------------------------------------------------------------------------

void BView::SetResizingMode(uint32 mode) {
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_RESIZE_MODE );
		owner->session->WriteInt32( mode );
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

void BView::SetViewCursor(const BCursor *cursor, bool sync=true) {

	if (!cursor)
		return;

	check_lock();
// TODO: test: what happens if a view is NOT attached to a window? 
//		will this method have any effect???
//		Info: I cannot test now... BApplication's looper must be started
	if (owner)
		if (sync){
			owner->session->WriteInt32( AS_LAYER_CURSOR );
			owner->session->WriteInt32( cursor->m_serverToken );
			owner->session->Sync();
		}
		else{
			owner->session->WriteInt32( AS_LAYER_CURSOR );
			owner->session->WriteInt32( cursor->m_serverToken );
		}
}

//---------------------------------------------------------------------------

void BView::Flush(void) const {
	if (owner)
		owner->Flush();
}

//---------------------------------------------------------------------------

void BView::Sync(void) const {
	do_owner_check_no_pick();
	owner->Sync();
}

//---------------------------------------------------------------------------

BWindow* BView::Window() const {
	return owner;
}



// Hook Functions
//---------------------------------------------------------------------------

void BView::AttachedToWindow(){
	// HOOK function
}

//---------------------------------------------------------------------------

void BView::AllAttached(){
	// HOOK function
}

//---------------------------------------------------------------------------

void BView::DetachedFromWindow(){
	// HOOK function
}

//---------------------------------------------------------------------------

void BView::AllDetached(){
	// HOOK function
}

//---------------------------------------------------------------------------

void BView::Draw(BRect updateRect){
	// HOOK function
}

void BView::DrawAfterChildren(BRect r){
	// HOOK function
}

void BView::FrameMoved(BPoint new_position){
	// HOOK function
}

void BView::FrameResized(float new_width, float new_height){
	// HOOK function
}

void BView::GetPreferredSize(float* width, float* height){
	// HOOK function
	*width				= fBounds.Width();
	*height				= fBounds.Height();
}

void BView::ResizeToPreferred(){
	// HOOK function
	ResizeTo(fBounds.Width(), fBounds.Height()); 
}

void BView::KeyDown(const char* bytes, int32 numBytes){
	// HOOK function
}

void BView::KeyUp(const char* bytes, int32 numBytes){
	// HOOK function
}

void BView::MouseDown(BPoint where){
	// HOOK function
}

void BView::MouseUp(BPoint where){
	// HOOK function
}

void BView::MouseMoved(BPoint where, uint32 code, const BMessage* a_message){
	// HOOK function
}

void BView::Pulse(){
	// HOOK function
}

void BView::TargetedByScrollView(BScrollView* scroll_view){
	// HOOK function
}

void BView::WindowActivated(bool state){
	// HOOK function
}

// Input Functions
//---------------------------------------------------------------------------

void BView::BeginRectTracking(BRect startRect,
									  uint32 style = B_TRACK_WHOLE_RECT)
{
	if (do_owner_check()) {
		owner->session->WriteInt32( AS_LAYER_BEGIN_RECT_TRACK );
		owner->session->WriteRect( startRect );
		owner->session->WriteInt32( style );		
	}
}

//---------------------------------------------------------------------------

void BView::EndRectTracking(){
	if (do_owner_check()) {
		owner->session->WriteInt32( AS_LAYER_END_RECT_TRACK );
	}
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BRect dragRect,
								BHandler* reply_to = NULL)
{
	if ( !aMessage || !dragRect.IsValid())
		return;
		
	if (!reply_to)
		reply_to = this;
	
	if (reply_to->Looper())
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();
	
	BPoint			offset;
	
	if ( !aMessage->HasInt32("buttons") ){
		BMessage 	*msg 	= owner->CurrentMessage();
		uint32		buttons;

		if ( msg ){
			if ( msg->FindInt32("buttons", (int32*)&buttons) == B_OK ){
			}
			else{
				BPoint		point;
				GetMouse(&point, &buttons, false);
			}
			BPoint		point;

			if (msg->FindPoint("be:view_where", &point) == B_OK)
				offset = point - dragRect.LeftTop();
			
		}
		else{
			BPoint		point;
			GetMouse(&point, &buttons, false);
		}
		aMessage->AddInt32("buttons", buttons);		
	}	

	_set_message_reply_( aMessage, BMessenger( reply_to, reply_to->Looper() ) );
	
	int32		bufSize		= aMessage->FlattenedSize();
	char		*buffer		= new char[ bufSize ];
	aMessage->Flatten( buffer, bufSize );

	owner->session->WriteInt32( AS_LAYER_DRAG_RECT );
	owner->session->WriteRect( dragRect );
	owner->session->WritePoint( offset );	
	owner->session->WriteInt32( bufSize );
	owner->session->WriteData( buffer, bufSize );
	owner->session->Sync();

	delete buffer;
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BBitmap* anImage, BPoint offset,
								BHandler* reply_to = NULL)
{
	DragMessage( aMessage, anImage, B_OP_COPY, offset, reply_to );
}

//---------------------------------------------------------------------------

void BView::DragMessage(BMessage* aMessage, BBitmap* anImage,
								drawing_mode dragMode, BPoint offset,
								BHandler* reply_to = NULL)
{
	if ( !aMessage || !anImage )
		return;
	
	if (!reply_to)
		reply_to = this;
	
	if (reply_to->Looper())
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();
	
	if ( !aMessage->HasInt32("buttons") ){
		BMessage 	*msg 	= owner->CurrentMessage();
		uint32		buttons;

		if ( msg ){
			if ( msg->FindInt32("buttons", (int32*)&buttons) == B_OK ){
			}
			else{
				BPoint		point;
				GetMouse(&point, &buttons, false);
			}
		}
		else{
			BPoint		point;
			GetMouse(&point, &buttons, false);
		}
		aMessage->AddInt32("buttons", buttons);		
	}	

	_set_message_reply_( aMessage, BMessenger( reply_to, reply_to->Looper() ) );
	
	int32		bufSize		= aMessage->FlattenedSize();
	char		*buffer		= new char[ bufSize ];
	aMessage->Flatten( buffer, bufSize );

	owner->session->WriteInt32( AS_LAYER_DRAG_IMAGE );
	owner->session->WriteInt32( anImage->get_server_token() );
	owner->session->WriteInt32( (int32)dragMode );	
	owner->session->WritePoint( offset );
	owner->session->WriteInt32( bufSize );
	owner->session->WriteData( buffer, bufSize );

	delete buffer;
/* TODO: in app_server the bitmap refCount must be incremented
 * WRITE this into specs!!!!
 */
	delete anImage;
}

//---------------------------------------------------------------------------

void BView::GetMouse(BPoint* location, uint32* buttons,
							 bool checkMessageQueue = true)
{
	do_owner_check();
	
	if (checkMessageQueue) {
		BMessageQueue	*mq;
		BMessage		*msg;
		int32			i = 0;
		
		mq				= Window()->MessageQueue();
		mq->Lock();
		
		while( (msg = mq->FindMessage(i++)) != NULL ) {
			switch (msg->what) {
				case B_MOUSE_UP:
						msg->FindPoint("where", location);
						msg->FindInt32("buttons", (int32*)buttons);
						Window()->DispatchMessage( msg, Window() );
						mq->RemoveMessage( msg );
						delete msg;
						return;
					break;
					
				case B_MOUSE_MOVED:
						msg->FindPoint("where", location);
						msg->FindInt32("buttons", (int32*)buttons);
						Window()->DispatchMessage( msg, Window() );
						mq->RemoveMessage( msg );
						delete msg;						
						return;
					break;
					
				case _UPDATE_:
						Window()->DispatchMessage( msg, Window() );
						mq->RemoveMessage( msg );
						delete msg;
						return;
					break;
					
				default:
					break;
			}
		}
		mq->Unlock();
	}
	
		// If B_MOUSE_UP or B_MOUSE_MOVED has not been found in the message queue,
		//		tell app_server to send us the current mouse coords and buttons.
	owner->session->WriteInt32( AS_LAYER_GET_MOUSE_COORDS );
	owner->session->Sync();
	
	owner->session->ReadPoint( location );
	owner->session->ReadInt32( (int32*)buttons );
}

//---------------------------------------------------------------------------

void BView::MakeFocus(bool focusState = true){
	if (owner){
			// if a view is in focus
		BView		*focus = owner->CurrentFocus();
		if (focus) {
			owner->fFocus	= NULL;
			focus->MakeFocus(false);
			owner->SetPreferredHandler(NULL);
		}
			// if we want to make this view the current focus view
		if (focusState){
			owner->fFocus	= this;
			owner->SetPreferredHandler(this);
		}
	}
}

//---------------------------------------------------------------------------

void BView::MessageReceived(BMessage* msg){
	BMessage			specifier;
	int32				what;
	const char*			prop;
	int32				index;
	status_t			err;

	if (msg->HasSpecifiers()){

	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK)
	{
		BMessage			replyMsg;

		switch (msg->what)
		{
		case B_GET_PROPERTY:{
				replyMsg.what		= B_NO_ERROR;
				replyMsg.AddInt32( "error", B_OK );
				
				if (strcmp(prop, "Frame") == 0 )
				{
					replyMsg.AddRect( "result", Frame());
				}
				else if (strcmp(prop, "Hidden") == 0 )
				{
					replyMsg.AddInt32( "result", IsHidden());
				}
			}break;

		case B_SET_PROPERTY:{
				if (strcmp(prop, "Frame") == 0 )
				{
					BRect			newFrame;
					if (msg->FindRect( "data", &newFrame ) == B_OK){
						MoveTo( newFrame.LeftTop() );
						ResizeTo( newFrame.right, newFrame.bottom);
					
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Hidden") == 0 )
				{
					bool			newHiddenState;
					if (msg->FindBool( "data", &newHiddenState ) == B_OK){
						if ( newHiddenState == true ){
							if ( !IsHidden() )
								Hide();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
/* TODO: a test!!!
 * 	Hide a view through a program, let's say... 3 times,
 * 		then try to send a scripting message to unhide that view.
 * 	Does the view become visible after the first call or after the 3rd????
 */							
						}
						else if ( newHiddenState == false ){
							if ( IsHidden() )
								Show();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
						}
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
			
			}break;
		}
		msg->SendReply( &replyMsg );
	}
	else{
		BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32( "error" , B_BAD_SCRIPT_SYNTAX );
		replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
		
		msg->SendReply( &replyMsg );
	}

	} // END: if (msg->HasSpecifiers())
	else
		BHandler::MessageReceived( msg );

}

//---------------------------------------------------------------------------

BScrollBar* BView::ScrollBar(orientation posture) const{
	switch (posture) {
		case B_VERTICAL:
				return fVerScroller;
			break;
		case B_HORIZONTAL:
				return fHorScroller;
			break;
		default:
				return NULL;
			break;
	}
}

//---------------------------------------------------------------------------

void BView::ScrollBy(float dh, float dv){
		// no reason to process this further if no scroll is intended.
	if ( dh == 0 && dv == 0){
		return;
	}

	check_lock();
		// if we're attached to a window tell app_server about this change
	if (owner) {
		owner->session->WriteInt32( AS_LAYER_SCROLL );
		owner->session->WriteFloat( dh );
		owner->session->WriteFloat( dv );

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

void BView::ScrollTo(BPoint where){
	ScrollBy( where.x - fBounds.left, where.y - fBounds.top );
}

//---------------------------------------------------------------------------

status_t BView::SetEventMask(uint32 mask, uint32 options=0){
	if (fEventMask == mask && fEventOptions == options)
		return B_ERROR;
		
	fEventMask			= mask;
	fEventOptions		= options;
	
	fState->archivingFlags	|= B_VIEW_EVMASK_BIT;
// TODO: modify! contact app_server!	

	return B_OK;
}

//---------------------------------------------------------------------------

uint32 BView::EventMask(){
	return fEventMask;
}

//---------------------------------------------------------------------------

status_t BView::SetMouseEventMask(uint32 mask, uint32 options=0){
	if (fEventMask == mask && fEventOptions == options) {
		return B_ERROR;
	}
	
	if (owner && owner->CurrentMessage()->what == B_MOUSE_DOWN ) {
			// we'll store the current mask and options in the upper bits of our:
		fEventMask		|= ( ((fEventMask << 16) & 0xFFFF0000) | mask );
		fEventOptions	|= ( ((fEventOptions << 16) & 0xFFFF0000) | options );
	}
	
/* TODO: write this in ... where B_MOUSE_UP is handled... :)
	fEventMask		= ((fEventMask >> 16) & 0x0000FFFF);
	fEventOptions	= ((fEventOptions >> 16) & 0x0000FFFF);
*/
// TODO: modify! contact app_server!

	return B_OK;
}

// Graphic State Functions
//---------------------------------------------------------------------------
void BView::SetLineMode(cap_mode lineCap, join_mode lineJoin,
						float miterLimit = B_DEFAULT_MITER_LIMIT)
{
	if (lineCap == fState->lineCap && lineJoin == fState->lineJoin &&
			miterLimit == fState->miterLimit)
		return;

	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_LINE_MODE );
		owner->session->WriteInt8( (int8)lineCap );
		owner->session->WriteInt8( (int8)lineJoin );
		owner->session->WriteFloat( miterLimit );
		
		fState->flags		|= B_VIEW_LINE_MODES_BIT;
	}
	
	fState->lineCap			= lineCap;
	fState->lineJoin		= lineJoin;
	fState->miterLimit		= miterLimit;
	
	fState->archivingFlags	|= B_VIEW_LINE_MODES_BIT;
}	

//---------------------------------------------------------------------------

join_mode BView::LineJoinMode() const{
	if (fState->flags & B_VIEW_LINE_MODES_BIT)
		LineMiterLimit();
	
	return fState->lineJoin;
}

//---------------------------------------------------------------------------

cap_mode BView::LineCapMode() const{
	if (fState->flags & B_VIEW_LINE_MODES_BIT)
		LineMiterLimit();
	
	return fState->lineCap;
}

//---------------------------------------------------------------------------

float BView::LineMiterLimit() const{
	if (fState->flags & B_VIEW_LINE_MODES_BIT)
	if (owner)
	{
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_GET_LINE_MODE );
		owner->session->Sync();
		
		owner->session->ReadInt8( (int8*)&(fState->lineCap) );
		owner->session->ReadInt8( (int8*)&(fState->lineJoin) );
		owner->session->ReadFloat( &(fState->miterLimit) );
		
		fState->flags			&= ~B_VIEW_LINE_MODES_BIT;
	}
	
	return fState->miterLimit;
}

//---------------------------------------------------------------------------

void BView::PushState(){
	do_owner_check();
	
	owner->session->WriteInt32( AS_LAYER_PUSH_STATE );
	
	initCachedState();
}

//---------------------------------------------------------------------------

void BView::PopState(){
	do_owner_check();

	owner->session->WriteInt32( AS_LAYER_POP_STATE );

		// this avoids a compiler warning
	uint32 dummy	= 0xffffffffUL;
		// invalidate all flags
	fState->flags	= dummy;
}

//---------------------------------------------------------------------------

void BView::SetScale(float scale) const{
	if (scale == fState->scale)
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_SCALE );
		owner->session->WriteFloat( scale );
		
			// I think that this flag won't be used after all... in 'flags' of course.
		fState->flags		|= B_VIEW_SCALE_BIT;	
	}
	
	fState->scale			= scale;

	fState->archivingFlags	|= B_VIEW_SCALE_BIT;
}
//---------------------------------------------------------------------------

float BView::Scale(){
	if (fState->flags & B_VIEW_SCALE_BIT)
	if (owner)
	{
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_GET_SCALE );
		owner->session->Sync();
		
		owner->session->ReadFloat( &(fState->scale) );

		fState->flags			&= ~B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}

//---------------------------------------------------------------------------

void BView::SetDrawingMode(drawing_mode mode){
	if (mode == fState->drawingMode)
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_DRAW_MODE );
		owner->session->WriteInt8( (int8)mode );
		
		fState->flags		|= B_VIEW_DRAW_MODE_BIT;	
	}
	
	fState->drawingMode		= mode;

	fState->archivingFlags	|= B_VIEW_DRAW_MODE_BIT;
}

//---------------------------------------------------------------------------

drawing_mode BView::DrawingMode() const{
	if (fState->flags & B_VIEW_DRAW_MODE_BIT)
	if (owner)
	{
		check_lock();

		owner->session->WriteInt32( AS_LAYER_GET_DRAW_MODE );
		owner->session->Sync();
		
		owner->session->ReadInt8( (int8*)&(fState->drawingMode) );
		
		fState->flags			&= ~B_VIEW_DRAW_MODE_BIT;
	}
	
	return fState->drawingMode;
}

//---------------------------------------------------------------------------

void BView::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc){
	if (srcAlpha  == fState->alphaSrcMode &&
			alphaFunc == fState->alphaFncMode)
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_BLEND_MODE );
		owner->session->WriteInt8( (int8)srcAlpha );
		owner->session->WriteInt8( (int8)alphaFunc );		
		
		fState->flags		|= B_VIEW_BLENDING_BIT;	
	}
	
	fState->alphaSrcMode	= srcAlpha;
	fState->alphaFncMode	= alphaFunc;	

	fState->archivingFlags	|= B_VIEW_BLENDING_BIT;
}

//---------------------------------------------------------------------------

void BView::GetBlendingMode(source_alpha* srcAlpha,	alpha_function* alphaFunc) const{
	if (fState->flags & B_VIEW_BLENDING_BIT)
	if (owner)
	{
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_GET_BLEND_MODE );
		owner->session->Sync();
		
		owner->session->ReadInt8( (int8*)&(fState->alphaSrcMode) );
		owner->session->ReadInt8( (int8*)&(fState->alphaFncMode) );
		
		fState->flags			&= ~B_VIEW_BLENDING_BIT;
	}
	
	if (srcAlpha)
		*srcAlpha		= fState->alphaSrcMode;
		
	if (alphaFunc)
		*alphaFunc		= fState->alphaFncMode;
}

//---------------------------------------------------------------------------

void BView::MovePenTo(BPoint pt){
	MovePenTo( pt.x, pt.y );
}

//---------------------------------------------------------------------------

void BView::MovePenTo(float x, float y){
	if (x  == fState->penPosition.x &&
			y == fState->penPosition.y)
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_PEN_LOC );
		owner->session->WriteFloat( x );
		owner->session->WriteFloat( y );		
		
		fState->flags		|= B_VIEW_PEN_LOC_BIT;	
	}
	
	fState->penPosition.x	= x;
	fState->penPosition.y	= y;	

	fState->archivingFlags	|= B_VIEW_PEN_LOC_BIT;
}

//---------------------------------------------------------------------------

void BView::MovePenBy(float x, float y){
	MovePenTo(fState->penPosition.x + x, fState->penPosition.y + y);
}

//---------------------------------------------------------------------------

BPoint BView::PenLocation() const{
	if (fState->flags & B_VIEW_PEN_LOC_BIT)
	if (owner)
	{
		check_lock();

		owner->session->WriteInt32( AS_LAYER_GET_PEN_LOC );
		owner->session->Sync();
		
		owner->session->ReadPoint( &(fState->penPosition) );
		
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
		
		owner->session->WriteInt32( AS_LAYER_SET_PEN_SIZE );
		owner->session->WriteFloat( size );
		
		fState->flags		|= B_VIEW_PEN_SIZE_BIT;	
	}
	
	fState->penSize			= size;

	fState->archivingFlags	|= B_VIEW_PEN_SIZE_BIT;
}

//---------------------------------------------------------------------------

float BView::PenSize() const{
	if (fState->flags & B_VIEW_PEN_SIZE_BIT)
	if (owner)
	{
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_GET_PEN_SIZE );
		owner->session->Sync();
		
		owner->session->ReadFloat( &(fState->penSize) );
		
		fState->flags			&= ~B_VIEW_PEN_SIZE_BIT;
	}
	
	return fState->penSize;
}

//---------------------------------------------------------------------------

void BView::SetHighColor(rgb_color a_color){
	if (_rgb_color_are_equal( fState->highColor, a_color ))
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_HIGH_COLOR );
		owner->session->WriteData( &a_color, sizeof(rgb_color) );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->highColor), a_color.red, a_color.green,
											a_color.blue, a_color.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::HighColor() const{
	if (fState->flags & B_VIEW_COLORS_BIT)
	if (owner)
	{
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_GET_COLORS );
		owner->session->Sync();
		
		owner->session->ReadData( &(fState->highColor), sizeof(rgb_color) );
		owner->session->ReadData( &(fState->lowColor), sizeof(rgb_color) );
		owner->session->ReadData( &(fState->viewColor), sizeof(rgb_color) );				
		
		fState->flags			&= ~B_VIEW_COLORS_BIT;
	}
	
	return fState->highColor;
}

//---------------------------------------------------------------------------

void BView::SetLowColor(rgb_color a_color){
	if (_rgb_color_are_equal( fState->lowColor, a_color ))
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_LOW_COLOR );
		owner->session->WriteData( &a_color, sizeof(rgb_color) );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->lowColor), a_color.red, a_color.green,
											a_color.blue, a_color.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::LowColor() const{
	if (fState->flags & B_VIEW_COLORS_BIT)
	if (owner){
			// HighColor() contacts app_server and gets the high, low and view colors
		HighColor();
	}
	
	return fState->lowColor;
}

//---------------------------------------------------------------------------

void BView::SetViewColor(rgb_color c){
	if (_rgb_color_are_equal( fState->viewColor, c ))
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_SET_VIEW_COLOR );
		owner->session->WriteData( &c, sizeof(rgb_color) );
		
		fState->flags		|= B_VIEW_COLORS_BIT;	
	}

	_set_ptr_rgb_color( &(fState->viewColor), c.red, c.green,
											c.blue, c.alpha);

	fState->archivingFlags	|= B_VIEW_COLORS_BIT;
}

//---------------------------------------------------------------------------

rgb_color BView::ViewColor() const{
	if (fState->flags & B_VIEW_COLORS_BIT)
	if (owner){
			// HighColor() contacts app_server and gets the high, low and view colors
		HighColor();
	}
	
	return fState->viewColor;
}

//---------------------------------------------------------------------------

void BView::ForceFontAliasing(bool enable){
	if ( enable == fState->fontAliasing)
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_PRINT_ALIASING );
		owner->session->WriteBool( enable );

			// I think this flag won't be used...
		fState->flags		|= B_VIEW_FONT_ALIASING_BIT;	
	}

	fState->fontAliasing	= enable;

	fState->archivingFlags	|= B_VIEW_FONT_ALIASING_BIT;
}

//---------------------------------------------------------------------------

void BView::SetFont(const BFont* font, uint32 mask = B_FONT_ALL){

	if (!font || mask == 0)
		return;

	if ( mask == B_FONT_ALL ){
		fState->font	= *font;
	}
	else{
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

	if (owner){
		check_lock();
		
		setFontState( &(fState->font), fState->fontFlags );
	}
}

//---------------------------------------------------------------------------

#if !_PR3_COMPATIBLE_
void BView::GetFont(BFont* font) const{
	*font	= fState->font;	
}

//---------------------------------------------------------------------------

#else
void BView:GetFont(BFont* font){
	*font	= fState->font;
}
#endif

//---------------------------------------------------------------------------

void BView::GetFontHeight(font_height* height) const{
	return fState->font.GetHeight( height );
}

//---------------------------------------------------------------------------

void BView::SetFontSize(float size){
	fState->font.SetSize( size );
}

//---------------------------------------------------------------------------

float BView::StringWidth(const char* string) const{
	return fState->font.StringWidth( string );
}

//---------------------------------------------------------------------------

float BView::StringWidth(const char* string, int32 length) const{
	return fState->font.StringWidth( string, length );
}

//---------------------------------------------------------------------------

void BView::GetStringWidths(char* stringArray[],int32 lengthArray[],
								int32 numStrings, float widthArray[]) const
{
	fState->font.GetStringWidths( const_cast<const char**>(stringArray),
								  const_cast<const int32*>(lengthArray),
								  numStrings, &*widthArray );
		// ARE these const_cast good?????
}

//---------------------------------------------------------------------------

void BView::TruncateString(BString* in_out, uint32 mode, float width) const{
	fState->font.TruncateString( in_out, mode, width);
}

//---------------------------------------------------------------------------

void BView::ClipToPicture(BPicture* picture,
						  BPoint where = B_ORIGIN,
						  bool sync = true)
{
	if ( picture == NULL )
		return;
		
	if (do_owner_check()){
		
		owner->session->WriteInt32( AS_LAYER_CLIP_PICTURE );
		owner->session->WriteInt32( picture->token );
		owner->session->WritePoint( where );
		
		if (sync){
			owner->session->Sync();
		}

		fState->flags		|= B_VIEW_CLIP_REGION_BIT;	
	}

	fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::ClipToInversePicture(BPicture* picture,
								 BPoint where = B_ORIGIN,
								 bool sync = true)
{
	if ( picture == NULL )
		return;
		
	if (do_owner_check()){
		
		owner->session->WriteInt32( AS_LAYER_CLIP_INVERSE_PICTURE );
		owner->session->WriteInt32( picture->token );
		owner->session->WritePoint( where );
		
		if (sync){
			owner->session->Sync();
		}

		fState->flags		|= B_VIEW_CLIP_REGION_BIT;	
	}

	fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::GetClippingRegion(BRegion* region) const{
	if ( region == NULL )
		return;

	if (fState->flags & B_VIEW_CLIP_REGION_BIT)
	if (do_owner_check())
	{
		int32					noOfRects;

		owner->session->WriteInt32( AS_LAYER_GET_CLIP_REGION );
		owner->session->Sync();
		owner->session->ReadInt32( &noOfRects );
		
		fState->clippingRegion.MakeEmpty();
		for (int32 i = 0; i < noOfRects; i++){
			BRect				rect;
			
			owner->session->ReadRect( &rect );
			
			fState->clippingRegion.Include( rect );
		}
		
		fState->flags			&= ~B_VIEW_CLIP_REGION_BIT;
	}
	
	*region			= fState->clippingRegion;
}

//---------------------------------------------------------------------------

void BView::ConstrainClippingRegion(BRegion* region){
	if (do_owner_check()){
		int32		noOfRects = 0;
		
		if (region)
			noOfRects	= region->CountRects();
		
		owner->session->WriteInt32( AS_LAYER_SET_CLIP_REGION );
			/* '0' means that in the app_server, there won't be any 'local'
			 * clipping region (it will be = NULL)
			 */
// TODO: note this in the specs!!!!!!			 
		owner->session->WriteInt32( noOfRects );
		
		for (int32 i = 0; i<noOfRects; i++){
			owner->session->WriteRect( region->RectAt(i) );
		}
			// we flush here because app_server waits for all the rects
		owner->session->Sync();

		fState->flags			|= B_VIEW_CLIP_REGION_BIT;
		fState->archivingFlags	|= B_VIEW_CLIP_REGION_BIT;
	}
}

//---------------------------------------------------------------------------


// Drawing Functions
//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap,	BRect srcRect, BRect dstRect){
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT );
		owner->session->WriteInt32( aBitmap->get_server_token() );
		owner->session->WriteRect( dstRect );
		owner->session->WriteRect( srcRect );		
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BRect dstRect){
	if ( !aBitmap || !dstRect.IsValid())
		return;
	
	DrawBitmapAsync( aBitmap, aBitmap->Bounds(), dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap){
	DrawBitmapAsync( aBitmap, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BPoint where){
	if ( !aBitmap )
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT );
		owner->session->WriteInt32( aBitmap->get_server_token() );
		owner->session->WritePoint( where );
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap){
	DrawBitmap( aBitmap, PenLocation() );	
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BPoint where){
	if ( !aBitmap )
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT );
		owner->session->WriteInt32( aBitmap->get_server_token() );
		owner->session->WritePoint( where );
		owner->session->Sync();
	}
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect dstRect){
	if ( !aBitmap || !dstRect.IsValid())
		return;
	
	DrawBitmap( aBitmap, aBitmap->Bounds(), dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect srcRect, BRect dstRect){
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;
		
	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT );
		owner->session->WriteInt32( aBitmap->get_server_token() );
		owner->session->WriteRect( dstRect );
		owner->session->WriteRect( srcRect );		
		owner->session->Sync();
	}
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar){
	DrawChar( aChar, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar, BPoint location){
	char		ch[2];
	ch[0]		= aChar;
	ch[1]		= '\0';
	
	DrawString( ch, strlen(ch), location );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString,
						escapement_delta* delta = NULL)
{
	if ( !aString )
		return;

	DrawString( aString, strlen(aString), PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, BPoint location,
						escapement_delta* delta = NULL)
{
	if ( !aString )
		return;

	DrawString( aString, strlen(aString), location );	
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length,
						escapement_delta* delta = NULL)
{
	if ( !aString )
		return;

	DrawString( aString, length, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length, BPoint location,
									   escapement_delta* delta = NULL)
{
	if ( !aString )
		return;

	if (owner){
		check_lock();
		
		owner->session->WriteInt32( AS_DRAW_STRING );
		owner->session->WritePoint( location );
		owner->session->WriteData( delta, sizeof(escapement_delta) );		
		owner->session->WriteString( aString );		

			// this modifies our pen location, so we invalidate the flag.
		fState->flags		|= B_VIEW_PEN_LOC_BIT;
	}
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BPoint center,
							float xRadius, float yRadius,
							pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_ELLIPSE );
		owner->session->WritePoint( center );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BRect r, pattern p = B_SOLID_HIGH) {
	if (owner)
	StrokeEllipse( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
					r.Width()/2, r.Height()/2, p );
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BPoint center,
						float xRadius, float yRadius,
						pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_ELLIPSE );
		owner->session->WritePoint( center );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BRect r, pattern p = B_SOLID_HIGH) {
	if (owner)
	FillEllipse( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
					r.Width()/2, r.Height()/2, p );

}

//---------------------------------------------------------------------------

void BView::StrokeArc(BPoint center,
					  float xRadius, float yRadius,
					  float start_angle, float arc_angle,
					  pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_ARC );
		owner->session->WritePoint( center );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
		owner->session->WriteFloat( start_angle );
		owner->session->WriteFloat( arc_angle );				
	}
}

//---------------------------------------------------------------------------

void BView::StrokeArc(BRect r,
					  float start_angle, float arc_angle,
					  pattern p = B_SOLID_HIGH)
{
	if (owner)
	StrokeArc( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
				r.Width()/2, r.Height()/2,
				start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------
									  
void BView::FillArc(BPoint center,
					float xRadius, float yRadius,
					float start_angle, float arc_angle,
					pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_ARC );
		owner->session->WritePoint( center );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
		owner->session->WriteFloat( start_angle );
		owner->session->WriteFloat( arc_angle );				
	}
}

//---------------------------------------------------------------------------

void BView::FillArc(BRect r,
					float start_angle, float arc_angle,
					pattern p = B_SOLID_HIGH)
{
	if (owner)
	FillArc( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
				r.Width()/2, r.Height()/2,
				start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------

void BView::StrokeBezier(BPoint* controlPoints, pattern p = B_SOLID_HIGH){
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_BEZIER );
		owner->session->WritePoint( controlPoints[0] );
		owner->session->WritePoint( controlPoints[1] );
		owner->session->WritePoint( controlPoints[2] );
		owner->session->WritePoint( controlPoints[3] );
	}
}

//---------------------------------------------------------------------------

void BView::FillBezier(BPoint* controlPoints, pattern p = B_SOLID_HIGH){
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_BEZIER );
		owner->session->WritePoint( controlPoints[0] );
		owner->session->WritePoint( controlPoints[1] );
		owner->session->WritePoint( controlPoints[2] );
		owner->session->WritePoint( controlPoints[3] );
	}
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPolygon* aPolygon,
						  bool closed = true, pattern p = B_SOLID_HIGH)
{
	if ( !aPolygon )
		return;
	
	if ( aPolygon->fCount <= 2 )
		return;

	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_POLYGON );
		owner->session->WriteInt8( closed );
		owner->session->WriteInt32( aPolygon->fCount );
		owner->session->WriteData(	aPolygon->fPts,
									aPolygon->fCount * sizeof(BPoint) );
	}
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts,
						  bool closed = true, pattern p = B_SOLID_HIGH)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	StrokePolygon( &pol, closed, p );
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
						  bool closed = true, pattern p = B_SOLID_HIGH)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	pol.MapTo( pol.Frame(), bounds);
	StrokePolygon( &pol, closed, p );
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPolygon* aPolygon,
						pattern p = B_SOLID_HIGH)
{
	if ( !aPolygon )
		return;
		
	if ( aPolygon->fCount <= 2 )
		return;

	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_POLYGON );
		owner->session->WriteInt32( aPolygon->fCount );
		owner->session->WriteData(	aPolygon->fPts,
									aPolygon->fCount * sizeof(BPoint) );
	}
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts,
						pattern p = B_SOLID_HIGH)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
						pattern p = B_SOLID_HIGH)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	pol.MapTo( pol.Frame(), bounds);
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::StrokeRect(BRect r, pattern p = B_SOLID_HIGH){
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
			
		owner->session->WriteInt32( AS_STROKE_RECT );
		owner->session->WriteRect( r );
	}
}

//---------------------------------------------------------------------------

void BView::FillRect(BRect r, pattern p = B_SOLID_HIGH){
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_RECT );
		owner->session->WriteRect( r );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeRoundRect(BRect r, float xRadius, float yRadius,
							pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_ROUNDRECT );
		owner->session->WriteRect( r );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::FillRoundRect(BRect r, float xRadius, float yRadius,
						  pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_ROUNDRECT );
		owner->session->WriteRect( r );
		owner->session->WriteFloat( xRadius );
		owner->session->WriteFloat( yRadius );
	}
}

//---------------------------------------------------------------------------

void BView::FillRegion(BRegion* a_region, pattern p= B_SOLID_HIGH){
	if ( !a_region )
		return;

	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		int32			rectsNo = a_region->CountRects();
		
		owner->session->WriteInt32( AS_FILL_REGION );
		owner->session->WriteInt32( rectsNo );

		for (int32 i = 0; i<rectsNo; i++){
			owner->session->WriteRect( a_region->RectAt(i) );
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
						   BRect bounds, pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_TRIANGLE );
		owner->session->WritePoint( pt1 );
		owner->session->WritePoint( pt2 );
		owner->session->WritePoint( pt3 );
			// ???: Do we need this?
		owner->session->WriteRect( bounds );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
						   pattern p = B_SOLID_HIGH)
{
	if (owner){
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

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
						 pattern p = B_SOLID_HIGH)
{
	if (owner){
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
						 BRect bounds, pattern p = B_SOLID_HIGH)
{
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_TRIANGLE );
		owner->session->WritePoint( pt1 );
		owner->session->WritePoint( pt2 );
		owner->session->WritePoint( pt3 );
			// ???: Do we need this?
		owner->session->WriteRect( bounds );
	}
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint toPt, pattern p = B_SOLID_HIGH){
	StrokeLine( PenLocation(), toPt, p);
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint pt0, BPoint pt1, pattern p = B_SOLID_HIGH){
	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_STROKE_LINE );
		owner->session->WritePoint( pt0 );
		owner->session->WritePoint( pt1 );
		
			// this modifies our pen location, so we invalidate the flag.
		fState->flags		|= B_VIEW_PEN_LOC_BIT;			
	}
}

//---------------------------------------------------------------------------

void BView::StrokeShape(BShape* shape, pattern p = B_SOLID_HIGH){
	if ( !shape )
		return;

	shape_data		*sd = (shape_data*)shape->fPrivateData;
	if ( sd->opCount == 0 || sd->ptCount == 0)
		return;

	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
	
		owner->session->WriteInt32( AS_STROKE_SHAPE );
			// ???: Do we need this?
		owner->session->WriteRect( shape->Bounds() );
		owner->session->WriteInt32( sd->opCount );
		owner->session->WriteInt32( sd->ptCount );
		owner->session->WriteData( sd->opList, sd->opCount );
		owner->session->WriteData( sd->ptList, sd->ptCount );
	}
}

//---------------------------------------------------------------------------

void BView::FillShape(BShape* shape, pattern p = B_SOLID_HIGH){
	if ( !shape )
		return;

	shape_data		*sd = (shape_data*)(shape->fPrivateData);
	if ( sd->opCount == 0 || sd->ptCount == 0)
		return;

	if (owner){
		check_lock();
		
		if ( _is_new_pattern( fState->patt, p ) )
			SetPattern( p );
		
		owner->session->WriteInt32( AS_FILL_SHAPE );
			// ???: Do we need this?
		owner->session->WriteRect( shape->Bounds() );
		owner->session->WriteInt32( sd->opCount );
		owner->session->WriteInt32( sd->ptCount );
		owner->session->WriteData( sd->opList, sd->opCount );
		owner->session->WriteData( sd->ptList, sd->ptCount );
	}
}

//---------------------------------------------------------------------------

void BView::BeginLineArray(int32 count){
	if (owner){
		if (count <= 0)
			debugger("Calling BeginLineArray with a count <= 0");

		check_lock_no_pick();
		
		if (comm){
			delete comm->array;
			delete comm;
		}

		comm			= new _array_data_;

		comm->maxCount	= count;
		comm->count		= 0;
		comm->array		= new _array_hdr_[count];
	}
}

//---------------------------------------------------------------------------

void BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col){
	if (owner){
		if (!comm)
			debugger("Can't call AddLine before BeginLineArray");

		check_lock_no_pick();
		
		if (comm->count < comm->maxCount){
			comm->array[ comm->count ].startX	= pt0.x;
			comm->array[ comm->count ].startX	= pt0.y;
			comm->array[ comm->count ].endX		= pt1.x;
			comm->array[ comm->count ].endX		= pt1.y;
			_set_ptr_rgb_color( &(comm->array[ comm->count ].color),
								col.red, col.green, col.blue, col.alpha );
			
			comm->count++;
		}
	}
}

//---------------------------------------------------------------------------

void BView::EndLineArray(){
	if(owner){
		if (!comm)
			debugger("Can't call EndLineArray before BeginLineArray");

		check_lock();

		owner->session->WriteInt32( AS_LAYER_LINE_ARRAY );
		owner->session->WriteInt32( comm->count );
		owner->session->WriteData(	comm->array,
									comm->count * sizeof(_array_hdr_) );

		delete comm->array;
		delete comm;
	}
}

//---------------------------------------------------------------------------

void BView::BeginPicture(BPicture* a_picture){
	if (do_owner_check()){
		if (a_picture && a_picture->usurped == NULL){
			a_picture->usurp(cpicture);
			cpicture = a_picture;

			owner->session->WriteInt32( AS_LAYER_BEGIN_PICTURE );
		}
	}
}

//---------------------------------------------------------------------------

void BView::AppendToPicture(BPicture* a_picture){
	check_lock();
	
	if (a_picture && a_picture->usurped == NULL){
		int32 token = a_picture->token;
		 
		if (token == -1){
			BeginPicture(a_picture);
		}
		else{
			a_picture->usurped = cpicture;
			a_picture->set_token(-1);
			owner->session->WriteInt32(AS_LAYER_APPEND_TO_PICTURE);
			owner->session->WriteInt32( token );
		} 
	}
}

//---------------------------------------------------------------------------

BPicture* BView::EndPicture(){
	if (do_owner_check()){
		if (cpicture){
			int32			token;

			owner->session->WriteInt32(AS_LAYER_END_PICTURE);
			owner->session->Sync();
			
			owner->session->ReadInt32( &token );
			 
			BPicture *a_picture = cpicture;
			cpicture = a_picture->step_down();
			a_picture->set_token(token);
			 
			return a_picture;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

//---------------------------------------------------------------------------

void BView::SetViewBitmap(const BBitmap* bitmap,
						  BRect srcRect, BRect dstRect,
						  uint32 followFlags = B_FOLLOW_TOP|B_FOLLOW_LEFT,
						  uint32 options = B_TILE_BITMAP)
{
	setViewImage(bitmap, srcRect, dstRect, followFlags, options);
}

//---------------------------------------------------------------------------

void BView::SetViewBitmap(const BBitmap* bitmap,
						  uint32 followFlags = B_FOLLOW_TOP|B_FOLLOW_LEFT,
						  uint32 options = B_TILE_BITMAP)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();
 
 	rect.OffsetTo(0, 0);

	setViewImage(bitmap, rect, rect, followFlags, options);
}

//---------------------------------------------------------------------------

void BView::ClearViewBitmap(){
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}

//---------------------------------------------------------------------------

status_t BView::SetViewOverlay(const BBitmap* overlay,
							   BRect srcRect, BRect dstRect,
							   rgb_color* colorKey,
							   uint32 followFlags = B_FOLLOW_TOP|B_FOLLOW_LEFT,
							   uint32 options = 0)
{
	status_t err = setViewImage(overlay, srcRect, dstRect, followFlags,
								options | 0x4);
		// read the color that will be treated as transparent
	owner->session->ReadData( colorKey, sizeof(rgb_color) );
 
	return err;
}

//---------------------------------------------------------------------------

status_t BView::SetViewOverlay(const BBitmap* overlay, rgb_color* colorKey,
							   uint32 followFlags = B_FOLLOW_TOP|B_FOLLOW_LEFT,
							   uint32 options = 0)
{
	BRect rect;
 	if (overlay)
		rect = overlay->Bounds();
 
 	rect.OffsetTo(0, 0);

	status_t err = setViewImage(overlay, rect, rect, followFlags,
								options | 0x4);
		// read the color that will be treated as transparent
	owner->session->ReadData( colorKey, sizeof(rgb_color) );
 
	return err;
}

//---------------------------------------------------------------------------

void BView::ClearViewOverlay(){
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}

//---------------------------------------------------------------------------

void BView::CopyBits(BRect src, BRect dst){
	if ( !src.IsValid() || !dst.IsValid() )
		return;

	if (do_owner_check()){
			owner->session->WriteInt32(AS_LAYER_COPY_BITS);
			owner->session->WriteRect( src );
			owner->session->WriteRect( dst );
	}
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const BPicture* a_picture){
	if (!a_picture)
		return;

	status_t 	err;
	
	DrawPictureAsync(a_picture, PenLocation());
	owner->session->WriteInt32( SERVER_TRUE );
	owner->session->Sync();
	
	owner->session->ReadInt32( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const BPicture* a_picture, BPoint where){
	if (!a_picture)
		return;
	
	status_t 	err;
	
	DrawPictureAsync( a_picture, where );
	owner->session->WriteInt32( SERVER_TRUE );
	owner->session->Sync();
	
	owner->session->ReadInt32( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPicture(const char* filename, long offset, BPoint where){
	if (!filename)
		return;

	status_t 	err;
	
	DrawPictureAsync( filename, offset, where );
	owner->session->WriteInt32( SERVER_TRUE );
	owner->session->Sync();
	
	owner->session->ReadInt32( &err );
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const BPicture* a_picture){
	if (!a_picture)
		return;

	DrawPictureAsync(a_picture, PenLocation());
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const BPicture* a_picture, BPoint where){
	if (!a_picture)
		return;

	if (do_owner_check() && a_picture->token > 0) {
		owner->session->WriteInt32( AS_LAYER_DRAW_PICTURE );
		owner->session->WriteInt32( a_picture->token );
		owner->session->WritePoint( where );
	}
}

//---------------------------------------------------------------------------

void BView::DrawPictureAsync(const char* filename, long offset, BPoint where){
	if (!filename)
		return;

// TODO: test, implement
}

//---------------------------------------------------------------------------

void BView::Invalidate(BRect invalRect){
	if ( !invalRect.IsValid() )
		return;

	if (owner){
		check_lock();

		owner->session->WriteInt32( AS_LAYER_INVAL_RECT );
		owner->session->WriteRect( invalRect );
			// ... becasue we want to see the results ASAP
		owner->session->Sync();
	}
}

//---------------------------------------------------------------------------

void BView::Invalidate(const BRegion* invalRegion){
	if ( !invalRegion )
		return;

	if (owner){
		check_lock();
		int32				noOfRects = 0;
		noOfRects			= const_cast<BRegion*>(invalRegion)->CountRects();
		
		owner->session->WriteInt32( AS_LAYER_INVAL_REGION );
		owner->session->WriteInt32( noOfRects );
		
		for (int i=0; i<noOfRects; i++){
			owner->session->WriteRect( const_cast<BRegion*>(invalRegion)->RectAt(i) );
		}
			// ... becasue we want to see the results ASAP
		owner->session->Sync();
	}
}

//---------------------------------------------------------------------------

void BView::Invalidate(){
	Invalidate( Bounds() );
}

//---------------------------------------------------------------------------

void BView::InvertRect(BRect r){

	if (owner){
		check_lock();

		owner->session->WriteInt32( AS_LAYER_INVERT_RECT );
		owner->session->WriteRect( r );
	}
}


// View Hierarchy Functions
//---------------------------------------------------------------------------
void BView::AddChild(BView* child, BView* before = NULL){
	if ( !child )
		return;

	if (child->parent != NULL)
		debugger("AddChild failed - the view already belonged to someone else.");
	
	if (owner)
		owner->Lock();
		
	if ( !addToList( child, before ) )
		debugger("AddChild failed - cannot find 'before' view.");	

	if (owner){
		check_lock();

		child->setOwner( this->owner);
		attachView( child );
		
		owner->Unlock();
	}	
}

//---------------------------------------------------------------------------

bool BView::RemoveChild(BView* child){

	if (!child)
		return false;
	
	return child->removeSelf();
}

//---------------------------------------------------------------------------

int32 BView::CountChildren() const{
	uint32		noOfChildren 	= 0;
	BView		*aChild			= first_child;
	
	while ( aChild != NULL ) {
		noOfChildren++;
		aChild		= aChild->next_sibling;
	}
	
	return noOfChildren;
}

//---------------------------------------------------------------------------

BView* BView::ChildAt(int32 index) const{
	uint32		noOfChildren 	= 0;
	BView		*aChild			= first_child;
	
	while ( aChild != NULL && noOfChildren < index ) {
		noOfChildren++;
		aChild		= aChild->next_sibling;
	}
	
	return aChild;
}

//---------------------------------------------------------------------------

BView* BView::NextSibling() const{
	return next_sibling;
}

//---------------------------------------------------------------------------

BView* BView::PreviousSibling() const{
	return prev_sibling;	
}

//---------------------------------------------------------------------------

bool BView::RemoveSelf(){
	return removeSelf();
}

//---------------------------------------------------------------------------

BView* BView::Parent() const{
	return parent;
}

//---------------------------------------------------------------------------

BView* BView::FindView(const char* name) const{
	return findView(this, name);
}

//---------------------------------------------------------------------------

void BView::MoveBy(float dh, float dv){
	MoveTo( originX + dh, originY + dv );
}

//---------------------------------------------------------------------------

void BView::MoveTo(BPoint where){
	MoveTo(where.x, where.y);
}

//---------------------------------------------------------------------------

void BView::MoveTo(float x, float y){

	if ( x==0.0 && y==0.0 )	
		return;
	
	check_lock();
	
	if (owner){
		owner->session->WriteInt32( AS_LAYER_MOVETO );
		owner->session->WriteFloat( x );
		owner->session->WriteFloat( y );
		
		fState->flags		|= B_VIEW_COORD_BIT;
	}
	
	originX		= x;
	originY		= y;
}

//---------------------------------------------------------------------------

// Private Functions
//---------------------------------------------------------------------------

void BView::InitData(BRect frame, const char *name, uint32 resizingMode, uint32 flags){

		// Info: The name of the view is set by BHandler constructor
		
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

	fVerScroller		= NULL;
	fHorScroller		= NULL;

	f_is_printing		= false;

	fPermanentState		= NULL;
	fState				= NULL;	// ALLMOST allways use this member!!!

	fBounds				= frame.OffsetToCopy(0.0, 0.0);
	fShelf				= NULL;

	fEventMask			= 0;
	fEventOptions		= 0;
}

//---------------------------------------------------------------------------

void BView::removeCommArray(){
	if( comm ){
		delete comm->array;
		delete comm;
		comm	= NULL;
	}
}

//---------------------------------------------------------------------------

void BView::setOwner(BWindow *theOwner)
{
	if (!theOwner){
		removeCommArray();
	}
	 
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

bool BView::removeSelf(){
/* 		# check for dirty flags 						- by updateCachedState()
 * 		# check for dirty flags on 'child' children		- by updateCachedState()
 *		# handle if in middle of Begin/EndLineArray()	- by setOwner(NULL)
 * 		# remove trom the main tree						- by removeFromList()
 * 		# handle if child is the default button			- HERE
 * 		# handle if child is the focus view				- by setOwner(NULL)
 * 		# handle if child is the menu bar				- HERE
 * 		# handle if child token is = fLastViewToken		- by setOwner(NULL)
 * 		# contact app_server							- HERE
 * 		# set a new owner = NULL						- by setOwner(NULL)
 */
 	bool		returnValue = true;

	check_lock();
	
	if (!parent)
		return false;
	
	if (owner){

		if (owner->fDefaultButton == this){
			owner->SetDefaultButton( NULL );
		}
		
		if (owner->fKeyMenuBar == this){
			owner->fKeyMenuBar = NULL;
		}
		
		if (owner->fLastViewToken == _get_object_token_(this)){
			owner->fLastViewToken = B_NULL_TOKEN;
		}
	
		callDetachHooks( this );
		updateCachedState();

		BWindow			*ownerZ = owner;

		setOwner( NULL );
			
		ownerZ->session->WriteInt32( AS_LAYER_DELETE );
	}

	returnValue		= removeFromList();
	
	parent			= NULL;
	next_sibling	= NULL;
	prev_sibling	= NULL;
	
	return returnValue;
}

//---------------------------------------------------------------------------

bool BView::callDetachHooks( BView *aView ){
//	check_clock();

		// call the hook function:
	aView->DetachedFromWindow();
	
		// we attach all its children
	BView		*child;
	child		= aView->first_child;
	
	while( child ) {
		aView->callDetachHooks(child);
		child		= child->next_sibling;
	}
		// call the hook function:
	aView->AllDetached();

	return true;
}

//---------------------------------------------------------------------------

bool BView::removeFromList(){
	BView		*current;

	if (parent->first_child == this){
		parent->first_child = next_sibling;
		 
		if (next_sibling)
			next_sibling->prev_sibling	= NULL;
	}
	else{
		prev_sibling->next_sibling	= next_sibling;

		if (next_sibling)
			next_sibling->prev_sibling	= prev_sibling;
	}
	return true;
}

//---------------------------------------------------------------------------

bool BView::addToList(BView *aView, BView *before = NULL){
	if ( !aView )
		return false;

	BView		*current = first_child;
	
	while( current && current->next_sibling != before )
		current	= current->next_sibling;

	if ( !current && before )
		return false;
	
	if ( current ){
		aView->next_sibling			= current->next_sibling;
		current->next_sibling		= aView;
		aView->prev_sibling			= current;
		
		if( aView->next_sibling )
			aView->next_sibling->prev_sibling		= aView;
	}
	else{
		first_child		= aView;
	}
	
	aView->parent		= this;
	
	return true;
}

//---------------------------------------------------------------------------

bool BView::attachView(BView *aView){
//	check_lock();

/* INFO:
 *	'check_lock()' checks for a lock on the window and then, modifies
 * 		BWindow::fLastViewToken with the one of the view witch called check_lock()
 * 		, and sends it to the app_server to be the view for witch current actions
 * 		are made.
 * 	This is a good solution for attaching a view to the server, but, this is done
 * 		many times, and because I suspect app_server holds ALL the 'Layer' pointers
 * 		in a hash list indexed by view tokens, a costly search action is made
 * 		for each view to be attached.
 * 	I think a better solution(also programming-wise), is to attach the number
 * 		of a view's children. Although this requires a list <iteration>, the
 * 		time to do it will be much smaller than searching a hash list.
 * 	On the server, only a recursive method is needed for building the tree...
 */
	
	owner->session->WriteInt32( AS_LAYER_CREATE );
	owner->session->WriteInt32( _get_object_token_( aView ) );
	owner->session->WriteRect( aView->Frame() );
	owner->session->WriteInt32( aView->ResizingMode() );
	owner->session->WriteInt32( aView->Flags() );
	owner->session->WriteInt32( aView->CountChildren() );
	
	setCachedState();

		// call the hook function:
	aView->AttachedToWindow();
	
		// we attach all its children
	BView		*child;
	child		= aView->first_child;
	
	while( child ) {
		aView->attachView(child);
		child		= child->next_sibling;
	}
		// call the hook function:
	aView->AllAttached();
	
	return true;
}

//---------------------------------------------------------------------------

void BView::deleteView( BView* aView){

	BView		*child;
	child		= aView->first_child;
	
	while( child ) {
		deleteView(child);
		child		= child->next_sibling;
	}
	
	delete aView;
}

//---------------------------------------------------------------------------

BView* BView::findView(const BView* aView, const char* viewName) const{
	if ( strcmp( viewName, aView->Name() ) == 0)
		return const_cast<BView*>(aView);

	BView			*child;
	if ( child = aView->first_child ){
		while ( child ) {
			BView*		view;
			if ( view == findView( child, viewName ) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------

void BView::setCachedState(){
	setFontState( &(fState->font), fState->fontFlags );
	
	owner->session->WriteInt32( AS_LAYER_SET_STATE );
	owner->session->WritePoint( fState->penPosition );
	owner->session->WriteFloat( fState->penSize );
	owner->session->WriteData( &(fState->highColor), sizeof(rgb_color) );
	owner->session->WriteData( &(fState->lowColor), sizeof(rgb_color) );
	owner->session->WriteData( &(fState->viewColor), sizeof(rgb_color) );
	owner->session->WriteData( &(fState->patt), sizeof(pattern) );	
	owner->session->WriteInt8( (int8)fState->drawingMode );
	owner->session->WritePoint( fState->coordSysOrigin );
	owner->session->WriteInt8( (int8)fState->lineJoin );
	owner->session->WriteInt8( (int8)fState->lineCap );
	owner->session->WriteFloat( fState->miterLimit );
	owner->session->WriteInt8( (int8)fState->alphaSrcMode );
	owner->session->WriteInt8( (int8)fState->alphaFncMode );
	owner->session->WriteFloat( fState->scale );
	owner->session->WriteBool( fState->fontAliasing );
		// we send the 'local' clipping region... if we have one...
	int32		noOfRects	= fState->clippingRegion.CountRects();
	
	owner->session->WriteInt32( noOfRects );
	for (int32 i = 0; i < noOfRects; i++){
		owner->session->WriteRect( fState->clippingRegion.RectAt(i) );
	}

		/* Although we might have a 'local' clipping region, when we call
		 * 		BView::GetClippingRegion(...);
		 *    we ask for the 'global' one; and that is kept on server, so we
		 *    must invalidate B_VIEW_CLIP_REGION_BIT flag!
		 */
	fState->flags			= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

void BView::setFontState(const BFont* font, uint16 mask){

	owner->session->WriteInt32( AS_LAYER_SET_FONT_STATE );
	owner->session->WriteInt32( mask );

		// always present.
	if ( mask & B_FONT_FAMILY_AND_STYLE ){
		uint32		fontID;
		fontID		= font->FamilyAndStyle( );
		
		owner->session->WriteInt32( fontID );
	}
	
	if ( mask & B_FONT_SIZE ){
		owner->session->WriteFloat( font->Size() );
	}

	if ( mask & B_FONT_SHEAR ){
		owner->session->WriteFloat( font->Shear() );
	}

	if ( mask & B_FONT_ROTATION ){
		owner->session->WriteFloat( font->Rotation() );
	}

	if ( mask & B_FONT_SPACING ){
		owner->session->WriteInt8( font->Spacing() );	// uint8
	}

	if ( mask & B_FONT_ENCODING ){
		owner->session->WriteInt8( font->Encoding() ); // uint8
	}

	if ( mask & B_FONT_FACE ){
		owner->session->WriteInt16( font->Face() );	// uint16
	}
	
	if ( mask & B_FONT_FLAGS ){
		owner->session->WriteInt32( font->Flags() ); // uint32
	}
}

//---------------------------------------------------------------------------

void BView::initCachedState(){
	fState->font			= be_plain_font;

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
	
	memcpy( &(fState->patt), &B_SOLID_HIGH, sizeof(pattern) );

	fState->drawingMode		= B_OP_COPY;

	// clippingRegion is empty by default

	fState->coordSysOrigin.Set( 0.0, 0.0 );

	fState->lineCap			= B_BUTT_CAP;
	fState->lineJoin		= B_BEVEL_JOIN;
	fState->miterLimit		= B_DEFAULT_MITER_LIMIT;

	fState->alphaSrcMode	= B_PIXEL_ALPHA;
	fState->alphaFncMode	= B_ALPHA_OVERLAY;

	fState->scale			= 1.0;

	fState->fontAliasing	= true;

		/* INFO: We include(invalidate) only B_VIEW_CLIP_REGION_BIT flag,
		 * because we should get the clipping region from app_server.
		 * 		The other flags do not need to be included because the data they
		 * represent is already in sync with app_server - app_server uses the
		 * same init(default) values.
		 */
	fState->flags			= B_VIEW_CLIP_REGION_BIT;
		// (default) flags used to determine witch fields to archive
	fState->archivingFlags	= B_VIEW_COORD_BIT;
}

//---------------------------------------------------------------------------

void BView::updateCachedState(){
		// fail if we do not have an owner
 	do_owner_check();
 	
 	owner->session->WriteInt32( AS_LAYER_GET_STATE );
 	owner->Sync();

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
	
	owner->session->ReadInt16( (int16*)&(fState->fontFlags) );

		// always present.
	if ( fState->fontFlags & B_FONT_FAMILY_AND_STYLE ){
		owner->session->ReadInt32( (int32*)&fontID );
		fState->font.SetFamilyAndStyle( fontID );
	}
	
	if ( fState->fontFlags & B_FONT_SIZE ){
		owner->session->ReadFloat( &size );
		fState->font.SetSize( size );
	}

	if ( fState->fontFlags & B_FONT_SHEAR ){
		owner->session->ReadFloat( &shear );
		fState->font.SetShear( shear );
	}

	if ( fState->fontFlags & B_FONT_ROTATION ){
		owner->session->ReadFloat( &rotation );
		fState->font.SetRotation( rotation );
	}

	if ( fState->fontFlags & B_FONT_SPACING ){
		owner->session->ReadInt8( (int8*)&spacing );
		fState->font.SetSpacing( spacing );
	}

	if ( fState->fontFlags & B_FONT_ENCODING ){
		owner->session->ReadInt8( (int8*)&encoding );
		fState->font.SetEncoding( encoding );
	}

	if ( fState->fontFlags & B_FONT_FACE ){
		owner->session->ReadInt16( (int16*)&face );
		fState->font.SetFace( face );
	}
	
	if ( fState->fontFlags & B_FONT_FLAGS ){
		owner->session->ReadInt32( (int32*)&flags );
		fState->font.SetFlags( flags );
	}
 	
	owner->session->ReadPoint( &(fState->penPosition) );
	owner->session->ReadFloat( &(fState->penSize) );	
	owner->session->ReadData( &(fState->highColor), sizeof(rgb_color) );
	owner->session->ReadData( &(fState->lowColor), sizeof(rgb_color) );
	owner->session->ReadData( &(fState->viewColor), sizeof(rgb_color) );
	owner->session->ReadData( &(fState->patt), sizeof(pattern) );
	owner->session->ReadPoint( &(fState->coordSysOrigin) );	
	owner->session->ReadInt8( (int8*)&(fState->drawingMode) );
	owner->session->ReadInt8( (int8*)&(fState->lineCap) );
	owner->session->ReadInt8( (int8*)&(fState->lineJoin) );
	owner->session->ReadFloat( &(fState->miterLimit) );
	owner->session->ReadInt8( (int8*)&(fState->alphaSrcMode) );
	owner->session->ReadInt8( (int8*)&(fState->alphaFncMode) );
	owner->session->ReadFloat( &(fState->scale) );
	owner->session->ReadBool( &(fState->fontAliasing) );

	owner->session->ReadInt32( &noOfRects );
		
	fState->clippingRegion.MakeEmpty();
	for (int32 i = 0; i < noOfRects; i++){
		owner->session->ReadRect( &rect );
		fState->clippingRegion.Include( rect );
	}
	//------------------
	
	owner->session->ReadFloat( &originX );
	owner->session->ReadFloat( &originY );
	owner->session->ReadRect( &fBounds );

	fState->flags			= B_VIEW_CLIP_REGION_BIT;
}

//---------------------------------------------------------------------------

status_t BView::setViewImage(const BBitmap *bitmap, BRect srcRect,
        BRect dstRect, uint32 followFlags, uint32 options)
{
	if (!do_owner_check())
		return B_ERROR;
	 
	int32		serverToken = bitmap ? bitmap->get_server_token() : -1;
	status_t	err;
	 
	owner->session->WriteInt32( AS_LAYER_SET_VIEW_IMAGE );
	owner->session->WriteInt32( serverToken );
	owner->session->WriteRect( srcRect );
	owner->session->WriteRect( dstRect );
	owner->session->WriteInt32( followFlags );
	owner->session->WriteInt32( options );
	owner->session->Sync();
	owner->session->ReadData( &err, sizeof(status_t) );
	 
	return err;
}
 
//---------------------------------------------------------------------------

void BView::SetPattern(pattern pat){
	if (owner){
		check_lock();

		owner->session->WriteInt32( AS_LAYER_SET_PATTERN );
		owner->session->WriteData( &pat, sizeof(pattern) );
	}
	
	fState->patt		= pat;
}
 
//---------------------------------------------------------------------------

bool BView::do_owner_check() const{
	uint32			serverToken = _get_object_token_(this);

	if (owner){
		owner->AssertLocked();

		if (owner->fLastViewToken != serverToken){
			owner->session->WriteInt32( AS_SET_CURRENT_LAYER );
			owner->session->WriteInt32( serverToken );

			owner->fLastViewToken = serverToken;
		}
		return true;
	}
	else{
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}

//---------------------------------------------------------------------------

void BView::check_lock() const{
	uint32			serverToken = _get_object_token_(this);

	if (owner){
		owner->AssertLocked();

		if (owner->fLastViewToken != serverToken){
			owner->session->WriteInt32( AS_SET_CURRENT_LAYER );
			owner->session->WriteInt32( serverToken );

			owner->fLastViewToken = serverToken;
		}
	}
}

//---------------------------------------------------------------------------

void BView::check_lock_no_pick() const{
	if (owner)
		owner->AssertLocked();
}

//---------------------------------------------------------------------------

bool BView::do_owner_check_no_pick() const{
	if (owner){
		owner->AssertLocked();
		return true;
	}
	else{
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------

inline rgb_color _get_rgb_color( uint32 color ){
	rgb_color		c;
	c.red			= (color & 0xFF000000) >> 24;
	c.green			= (color & 0x00FF0000) >> 16;
	c.blue			= (color & 0x0000FF00) >> 8;
	c.alpha			= (color & 0x000000FF);

	return c;
}

//---------------------------------------------------------------------------

inline uint32 _get_uint32_color( rgb_color c ){
	uint32			color;
	color			= (c.red << 24) +
					  (c.green << 16) +
					  (c.blue << 8) +
					  c.alpha;
	return color;
}

//---------------------------------------------------------------------------

inline rgb_color _set_static_rgb_color( uint8 r, uint8 g, uint8 b, uint8 a=255 ){
	rgb_color		color;
	color.red		= r;
	color.green		= g;
	color.blue		= b;
	color.alpha		= a;

	return color;
}

//---------------------------------------------------------------------------

inline void _set_ptr_rgb_color( rgb_color* c, uint8 r, uint8 g,
								uint8 b, uint8 a=255 )
{
	c->red			= r;
	c->green		= g;
	c->blue			= b;
	c->alpha		= a;
}

//---------------------------------------------------------------------------

inline bool _rgb_color_are_equal( rgb_color c1, rgb_color c2 ){
	return _get_uint32_color( c1 ) == _get_uint32_color( c2 );
}

//---------------------------------------------------------------------------

inline bool _is_new_pattern( const pattern& p1, const pattern& p2 ){
	if ( memcmp( &p1, &p2, sizeof(pattern) ) == 0 )
		return false;
	else
		return true;
}
/* TODOs
 * 		-implement SetDiskMode() what's with this method? what does it do? test!
 * 			does it has something to do with DrawPictureAsync( filename* .. )?
 *		-implement DrawAfterChildren()
 */
