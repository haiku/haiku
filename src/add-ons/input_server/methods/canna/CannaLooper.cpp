//
//	CannaLooper.cpp
//	CannaIM looper object

//	This is a part of...
//	CannaIM
//	version 1.0
//	(c) 1999 M.Kawamura
//

#include <string.h>

#include <Input.h>
#include <Screen.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Alert.h>
#include <Messenger.h>

#ifdef DEBUG
#include <Debug.h>
#endif

#include "CannaCommon.h"
#include "CannaLooper.h"
#include "CannaMethod.h"
#include "KouhoWindow.h"
#include "PaletteWindow.h"

CannaLooper::CannaLooper( CannaMethod *method )
	:BLooper( "Canna Looper" )
{
	owner = method;
	canna = 0;
	theKouhoWindow = 0;
	thePalette = 0;
	self = BMessenger( NULL, this );

	theMenu = new BMenu( B_EMPTY_STRING );
	theMenu->SetFont( be_plain_font );
	theMenu->AddItem( new BMenuItem( "About CannaIM…", new BMessage( B_ABOUT_REQUESTED )));
	theMenu->AddSeparatorItem();
	theMenu->AddItem( new BMenuItem( "Convert arrow keys", new BMessage( ARROW_KEYS_FLIPPED )));
	theMenu->AddItem( new BMenuItem( "Reload Init file", new BMessage( RELOAD_INIT_FILE )));

	if ( gSettings.convert_arrowkey )
	{
		BMenuItem *item = theMenu->FindItem( ARROW_KEYS_FLIPPED );
		item->SetMarked( true );
	}
	
	Run();
}

status_t
CannaLooper::InitCheck()
{
	status_t err;
	char basepath[ B_PATH_NAME_LENGTH + 1 ];
	err = ReadSettings( basepath );
	if ( err != B_NO_ERROR )
		return err;

	canna = new CannaInterface( basepath );
	err = canna->InitCheck();
	if ( err != B_NO_ERROR )
		return err;
	
//	theKouhoWindow = new KouhoWindow( &kouhoFont );
		
	return B_NO_ERROR;
}

void
CannaLooper::Quit()
{
	// delete palette here
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: destructor called.\n" ));
#endif
	if ( canna )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: Deleting canna...\n" ));
#endif
		delete canna;
	}
	
	if ( theKouhoWindow )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: Sending QUIT to kouho window...\n" ));
#endif
		theKouhoWindow->Lock();
		theKouhoWindow->Quit();
	}
	
	if ( thePalette )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: Sending QUIT to palette...\n" ));
#endif
		thePalette->Lock();
		thePalette->Quit();
	}
	
	owner->SetMenu( NULL, BMessenger() );
	delete theMenu;
	BLooper::Quit();
}

status_t
CannaLooper::ReadSettings( char *basepath )
{
#ifndef __HAIKU__ 
	strcpy( basepath, "/boot/home/config/KanBe/" );
#else
	strcpy( basepath, "/etc/KanBe/" );
#endif
	font_family family;
	font_style style;
	strcpy( family, "Haru" );
	strcpy( style, "Regular" );
	kouhoFont.SetFamilyAndStyle( family, style );
	kouhoFont.SetSize( 12 );
	return B_NO_ERROR;
}

void
CannaLooper::EnqueueMessage( BMessage *msg )
{
	owner->EnqueueMessage( msg );
}

void
CannaLooper::MessageReceived( BMessage *msg )
{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: Entering MessageReceived() what=%x\n", msg->what ));
#endif
	switch ( msg->what )
	{
		case B_KEY_DOWN:
		case NUM_SELECTED_FROM_KOUHO_WIN:
			HandleKeyDown( msg );
			break;
		
		case B_INPUT_METHOD_EVENT:
			uint32 opcode, result;
			opcode = 0;
			msg->FindInt32( "be:opcode", (int32 *)&opcode );
			switch ( opcode )
			{
				case B_INPUT_METHOD_LOCATION_REQUEST:
					HandleLocationReply( msg );
					break;
					
				case B_INPUT_METHOD_STOPPED:
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_STOPPED received\n" ));
#endif
					ForceKakutei();
					break;
			}
			break;
			
		case CANNA_METHOD_ACTIVATED:
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: CANNA_METHOD_ACTIVATED received\n" ));
#endif
			HandleMethodActivated( msg->HasBool( "active" ) );
			break;
		
		case MODE_CHANGED_FROM_PALETTE:
			ForceKakutei();
			int32 mode;
			msg->FindInt32( "mode", &mode );
			result = canna->ChangeMode( mode );
			ProcessResult( result );
			break;
			
		case B_ABOUT_REQUESTED:
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_ABOUT_REQUESTED received\n" ));
#endif
			BAlert *panel;
			panel = new BAlert( "", "CannaIM Version 1.02\n"
								"  by Masao Kawamura 1999\n\n"
								"Canna\n  Copyright 1992 NEC Corporation, Tokyo, Japan\n"
								"  Special thanks to T.Murai for porting\n"
								"  BeOS Version 1.02",
								"OK" );
			panel->Go();
			break;
			
		case RELOAD_INIT_FILE:
			ForceKakutei();
			canna->Reset();
			break;

		case ARROW_KEYS_FLIPPED:
			{
				BMenuItem *item = theMenu->FindItem( ARROW_KEYS_FLIPPED );
				gSettings.convert_arrowkey = !gSettings.convert_arrowkey;
				item->SetMarked( gSettings.convert_arrowkey );
				canna->SetConvertArrowKey( gSettings.convert_arrowkey );
			
			}
			break;
			
							
		default:
			BLooper::MessageReceived( msg );
			break;
	}
}

void CannaLooper::HandleKeyDown( BMessage *msg )
{
	uint32 result;
	uint32 modifier;
//	int32 rawchar;
	int32 key;
	char theChar;

	msg->FindInt32( "modifiers", (int32 *)&modifier );
	msg->FindInt32( "key", &key );
//	msg->FindInt32( "raw_char", &rawchar );
	
	if ( ( modifier & B_COMMAND_KEY ) != 0 )
	{
		EnqueueMessage( DetachCurrentMessage() );
		return;
	}
	
	msg->FindInt8( "byte", (int8 *)&theChar );

// The if clause below is to avoid processing key input which char code is 0x80 or more.
// if mikakutei string exists, dispose current message.
// Otherwise, send it to application as usual B_KEY_DOWN message.
// This is a bug fix for version 1.0 beta 2

	if ( theChar & 0x80 )
	{
		if ( canna->MikakuteiLength() != 0 )
		{
			BMessage *blackhole = DetachCurrentMessage();
			delete blackhole;
		}
		else
			EnqueueMessage( DetachCurrentMessage() );

		return;
	}
	
// end of bug fix

#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: HandleKeyDown() calling CannaInterface::KeyIn()...\n", result ));
#endif
	
	result = canna->KeyIn( theChar, modifier, key );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: HandleKeyDown() received result = %d from CannaInterface.\n", result ));
#endif
	ProcessResult( result );
}

void
CannaLooper::ProcessResult( uint32 result )
{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing result = %d\n", result ));
#endif
	if ( result & GUIDELINE_APPEARED )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing GUIDELINE_APPEARED\n" ));
#endif
		if ( canna->MikakuteiLength() != 0 ) // usual guideline i.e. kouho
		{
			BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
			msg->AddInt32( "be:opcode", B_INPUT_METHOD_LOCATION_REQUEST );
			owner->EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_LOCATION_REQUEST has been sent\n" ));
#endif
		}
		else // guideline exists, but no mikakutei string - means extend mode and such.
		{
#ifdef DEBUG
		SERIAL_PRINT(( "  GUIDELINE_APPEARED: calling GenerateKouho()...\n" ));
#endif
			theKouhoWindow->PostMessage( canna->GenerateKouhoString() );
			BMessage m( KOUHO_WINDOW_SHOW_ALONE );
#ifdef DEBUG
		SERIAL_PRINT(( "  GUIDELINE_APPEARED: posting KouhoMsg to KouhoWindow %x...\n", theKouhoWindow ));
#endif
			theKouhoWindow->PostMessage( &m );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: KOUHO_WINDOW_SHOW_ALONE has been sent\n" ));
#endif
		}
		
	}

//	if ( result == MIKAKUTEI_NO_CHANGE )
//		return;
				
	if ( result & GUIDELINE_DISAPPEARED )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing GUIDELINE_DISAPPEARED\n" ));
#endif
		theKouhoWindow->PostMessage( KOUHO_WINDOW_HIDE );
	}
		
	if ( result & MODE_CHANGED )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing MODE_CHANGED\n" ));
#endif
		int32 mode;
		mode = canna->GetMode();
		BMessage m( PALETTE_WINDOW_BUTTON_UPDATE );
		m.AddInt32( "mode", mode );
		thePalette->PostMessage( &m );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: PALETTE_BUTTON_UPDATE has been sent. mode = %d\n", mode ));
#endif
	}

	if ( result & GUIDELINE_CHANGED )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing GUIDELINE_CHANGED\n" ));
#endif
		theKouhoWindow->PostMessage( canna->GenerateKouhoString() );
	}

	if ( result & THROUGH_INPUT )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing THROUGH_INPUT\n" ));
#endif
		EnqueueMessage( DetachCurrentMessage() );
	}

	if ( result & NEW_INPUT_STARTED )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing NEW_INPUT_STARTED\n" ));
#endif
		SendInputStarted();
	}

	if ( result & KAKUTEI_EXISTS )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing KAKUTEI_EXISTS\n" ));
#endif
		BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
		msg->AddInt32( "be:opcode", B_INPUT_METHOD_CHANGED );
	
		msg->AddString("be:string", canna->GetKakuteiStr() );
		msg->AddInt32("be:clause_start", 0);
		msg->AddInt32("be:clause_end", canna->KakuteiLength() );
		msg->AddInt32("be:selection", canna->KakuteiLength() );
		msg->AddInt32("be:selection", canna->KakuteiLength() );
		msg->AddBool( "be:confirmed", true );
		owner->EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_CHANGED (confired) has been sent\n" ));
#endif
		if ( !(result & MIKAKUTEI_EXISTS) ) //if both kakutei and mikakutei exist, do not send B_INPUT_STOPPED
			SendInputStopped();
		
	}
	
	if ( result & MIKAKUTEI_EXISTS )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing MIKAKUTEI_EXISTS\n" ));
#endif
		int32 start, finish;
		BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
		msg->AddInt32( "be:opcode", B_INPUT_METHOD_CHANGED );
	
		msg->AddString("be:string", canna->GetMikakuteiStr() );
		if ( canna->HasRev() )
		{
			canna->GetRevPosition( &start, &finish );
			msg->AddInt32("be:clause_start", 0);
			msg->AddInt32("be:clause_end", start );
			msg->AddInt32("be:clause_start", start );
			msg->AddInt32("be:clause_end", finish );
			msg->AddInt32("be:clause_start", finish );
			msg->AddInt32("be:clause_end", canna->MikakuteiLength() );
		}
		else
		{
			start = finish = canna->MikakuteiLength();
			msg->AddInt32("be:clause_start", 0);
			msg->AddInt32("be:clause_end", canna->MikakuteiLength() );
		}
		
		msg->AddInt32("be:selection", start );
		msg->AddInt32("be:selection", finish );
		//msg->AddBool( "be:confirmed", false );
		owner->EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_CHANGED (non-confirmed) has been sent\n" ));
#endif
	}
	
	if ( result & MIKAKUTEI_BECOME_EMPTY )
	{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ProcessResult() processing MIKAKUTEI_BECOME_EMPTY\n" ));
#endif
		BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
		msg->AddInt32( "be:opcode", B_INPUT_METHOD_CHANGED );
	
		msg->AddString("be:string", B_EMPTY_STRING );
		msg->AddInt32("be:clause_start", 0);
		msg->AddInt32("be:clause_end", 0 );
		msg->AddInt32("be:selection", 0 );
		msg->AddInt32("be:selection", 0 );
		msg->AddBool( "be:confirmed", true );
		owner->EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_CHANGED (NULL, confired) has been sent\n" ));
#endif
	
		SendInputStopped();
	}
}

void
CannaLooper::HandleLocationReply( BMessage *msg )
{
	BPoint where = B_ORIGIN;
	float height = 0.0;
	int32 start;

#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_LOCATION_REQUEST received\n" ));
#endif

	start = canna->GetRevStartPositionInChar();
#ifdef DEBUG
	type_code type;
	int32 count;
	msg->GetInfo( "be:location_reply", &type, &count );
	SERIAL_PRINT(( "CannaLooper: LOCATION_REPLY has %d locations.\n", count ));
	SERIAL_PRINT(( "CannaLooper: RevStartPosition is %d.\n", start ));
#endif
	msg->FindPoint( "be:location_reply", start, &where );
	msg->FindFloat( "be:height_reply", start, &height );
	BMessage m( KOUHO_WINDOW_SHOWAT );
	m.AddPoint( "position", where );
	m.AddFloat( "height", height );


	theKouhoWindow->PostMessage( canna->GenerateKouhoString() );
	theKouhoWindow->PostMessage( &m );
}

void
CannaLooper::HandleMethodActivated( bool active )
{
	if ( active )
	{
		if ( !thePalette ) //first time input method activated 
		{
			float x = gSettings.palette_loc.x;
			float y = gSettings.palette_loc.y;
			BRect frame( x, y, x + 114, y + 44 );
			thePalette = new PaletteWindow( frame, this );
			thePalette->Show();
			theKouhoWindow = new KouhoWindow( &kouhoFont, this );
			theKouhoWindow->Run();
		}
		thePalette->PostMessage( PALETTE_WINDOW_SHOW );
		owner->SetMenu( theMenu, self );
	}
	else
	{
		ForceKakutei();
		canna->ChangeMode( CANNA_MODE_HenkanMode );
		BMessage m( PALETTE_WINDOW_BUTTON_UPDATE );
		m.AddInt32( "mode", CANNA_MODE_HenkanMode );
		thePalette->PostMessage( &m );
		thePalette->PostMessage( PALETTE_WINDOW_HIDE );
		theKouhoWindow->PostMessage( KOUHO_WINDOW_HIDE );
		owner->SetMenu( NULL, self );
	}
}
			
void
CannaLooper::ForceKakutei()
{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: ForceKakutei() called\n" ));
#endif
	uint32 result = canna->Kakutei();
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: returned from Kakutei(). result = %d\n", result ));
#endif
	if ( result != NOTHING_TO_KAKUTEI )
		ProcessResult( result );
	else
		SendInputStopped();
	
	return;
}

void
CannaLooper::SendInputStopped()
{
	BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
	msg->AddInt32( "be:opcode", B_INPUT_METHOD_STOPPED );
	EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_STOPPED has been sent\n" ));
#endif
}

void
CannaLooper::SendInputStarted()
{
		BMessage *msg = new BMessage( B_INPUT_METHOD_EVENT );
		msg->AddInt32( "be:opcode", B_INPUT_METHOD_STARTED );
		msg->AddMessenger( "be:reply_to", BMessenger( NULL, this ) );
		EnqueueMessage( msg );
#ifdef DEBUG
		SERIAL_PRINT(( "CannaLooper: B_INPUT_METHOD_STARTED has been sent\n" ));
#endif
}

