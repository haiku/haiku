/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#include "CannaInterface.h"
#include <UTF8.h>
#include <string.h>

#ifdef DEBUG
#include <Debug.h>
#endif

inline int32
UTF8CharLen(uchar c)
	{ return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); }

CannaInterface::CannaInterface( char *path )
{
	strcpy( basePath, path );
	InitializeCanna();
}

void
CannaInterface::InitializeCanna()
{
	char **warn;

	context_id = 0; //context id is now fixed to zero.
#ifdef DEBUG
	SERIAL_PRINT(( "CannaInterface:Setting basepath to %s.\n", basePath ));
#endif

	setBasePath( basePath );

	jrKanjiControl(context_id, KC_INITIALIZE, (char *)&warn);
#ifdef DEBUG
	SERIAL_PRINT(( "CannaInterface:Canna Initialize result = %x.\n", warn ));
#endif

    if (warn)
    {
		canna_enabled = false;
		return;
    }

	int32 max_width = KOUHO_WINDOW_MAXCHAR - 2;
	jrKanjiControl(context_id, KC_SETWIDTH, (char *) max_width);

	jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 0);

	jrKanjiControl(context_id, KC_SETHEXINPUTSTYLE, (char *)(int32) 1);

	jrKanjiControl( context_id, KC_SETUNDEFKEYFUNCTION, (char *)(int32) kc_through );

	jrKanjiStatusWithValue	ks;
	uchar buf[CONVERT_BUFFER_SIZE];

	ks.val = CANNA_MODE_HenkanMode;
	ks.buffer = buf;
	ks.bytes_buffer = CONVERT_BUFFER_SIZE;
	ks.ks = &kanji_status;
	jrKanjiControl( context_id, KC_CHANGEMODE, (char *)&ks);

	canna_enabled = true;
	convert_arrowkey = gSettings.convert_arrowkey;
	hadMikakuteiStr = false;
	hadGuideLine = false;
}

CannaInterface::~CannaInterface()
{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaInterface: Destructor called.\n" ));
#endif
	jrKanjiStatusWithValue	ks;
	ks.val = 0;
	ks.buffer = (unsigned char *)kakuteiStr;
	ks.bytes_buffer = CONVERT_BUFFER_SIZE;
	ks.ks = &kanji_status;
	jrKanjiControl(context_id, KC_FINALIZE, (char *)&ks);
}

status_t CannaInterface::InitCheck()
{
	if ( canna_enabled )
	{
		jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 2);
		char mode[5];
		jrKanjiControl( context_id, KC_QUERYMODE, mode );
		current_mode = mode[1] - '@';
		jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 0);
		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}

uint32 CannaInterface::KeyIn( char ch, uint32 mod, int32 key )
{
	int inkey;

	inkey = ConvertSpecial( ch, mod, key );
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: KeyIn() returned from ConvertSpecial. inkey = 0x%x\n", inkey ));
#endif
	if ( convert_arrowkey && kanji_status.gline.length != 0 )
		inkey = ConvertArrowKey( inkey );
/*
	if ( kanji_status.length == 0 )
	{
		hadMikakuteiStr = false;
		previousUTF[0] = '\0';
	}
	else
	{
		strcpy( previousUTF, mikakuteiUTF );
		hadMikakuteiStr = true;
	}
*/
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: Calling jrKanjiString()...\n" ));
#endif
	kakuteiLen = jrKanjiString(context_id, inkey, kakuteiStr, CONVERT_BUFFER_SIZE, &kanji_status);
#ifdef DEBUG
SERIAL_PRINT(( "kakL = %d, mikL = %d, glineL = %d, info = 0x%x\n", kakuteiLen, kanji_status.length, kanji_status.gline.length, kanji_status.info ));
#endif
	//return UpdateKanjiStatus();
	uint32 result = UpdateKanjiStatus();
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: KeyIn() returning 0x%x.\n", result ));
#endif
	return result;
}

uint32 CannaInterface::UpdateKanjiStatus()
{
	uint32 result = 0;
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: Entering UpdateKanjiStatus()...\n" ));
#endif

	if ( hadGuideLine && kanji_status.gline.length == 0 )
	{
		result |= GUIDELINE_DISAPPEARED;
		hadGuideLine = false;
	}

	if ( kanji_status.length == -1 )
	{
		result |= MIKAKUTEI_NO_CHANGE;
		return result;
	}

	if ( kanji_status.info & KanjiThroughInfo )
	{
		result |= THROUGH_INPUT;
		return result;
	}

	if ( kanji_status.info & KanjiModeInfo )
	{
		jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 2);
		char mode[5];
		jrKanjiControl( context_id, KC_QUERYMODE, mode );
		current_mode = mode[1] - '@';
		jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 0);
		result |= MODE_CHANGED;
	}

	if ( !hadMikakuteiStr && (kanji_status.length != 0 || kakuteiLen != 0 ))
	{
		//ClearPrevious();
		result |= NEW_INPUT_STARTED;
	}

	kakuteiUTF[0] = mikakuteiUTF[0] = '\0';
	kakuteiUTFLen = mikakuteiUTFLen = 0;
	int32 length;
	int32 state;
	//Convert KakuteiString to UTF-8
	if ( kakuteiLen > 0 )
	{
		length = kakuteiLen; //make copy
		kakuteiUTFLen = CONVERT_BUFFER_SIZE * 2;
		convert_to_utf8( B_EUC_CONVERSION, (const char *)kakuteiStr, &length,
						 kakuteiUTF, &kakuteiUTFLen, &state );
		kakuteiUTF[kakuteiUTFLen] = '\0'; //terminating NULL

		result |= KAKUTEI_EXISTS;
	}

		//Convert MikakuteiString to UTF-8
	if ( kanji_status.length > 0 )
	{
		length = kanji_status.length; //make copy
		mikakuteiUTFLen = CONVERT_BUFFER_SIZE * 2;
		convert_to_utf8( B_EUC_CONVERSION,
						 (const char *)kanji_status.echoStr, &length,
						 mikakuteiUTF, &mikakuteiUTFLen, &state );
		mikakuteiUTF[mikakuteiUTFLen] = '\0'; //terminating NULL

		result |= MIKAKUTEI_EXISTS;
	}

	//when mikakutei string is deleted and become empty
	if ( hadMikakuteiStr && kanji_status.length == 0 && kakuteiLen == 0 )
		result |= MIKAKUTEI_BECOME_EMPTY;

	if ( kanji_status.length == 0 ) // set hadMikakuteiStr flag for the next transaction
		hadMikakuteiStr = false;
	else
		hadMikakuteiStr = true;

	if ( hadGuideLine && (kanji_status.info & KanjiGLineInfo) )
		result |= GUIDELINE_CHANGED;

	if ( !hadGuideLine && kanji_status.gline.length != 0 )
	{
		result |= GUIDELINE_APPEARED;
		hadGuideLine = true;
	}

	// calculate revpos, revlen
	if ( kanji_status.revLen == 0 )
	{
		revBegin = 0;
		revEnd = 0;
	}
	else
	{
		length = kanji_status.revPos;
		revBegin = CONVERT_BUFFER_SIZE * 2;
		convert_to_utf8( B_EUC_CONVERSION, (const char*)kanji_status.echoStr, &length,
						mikakuteiUTF, &revBegin, &state );
		revBegin += kakuteiUTFLen;

		length = kanji_status.revPos + kanji_status.revLen;
		revEnd = CONVERT_BUFFER_SIZE * 2;
		convert_to_utf8( B_EUC_CONVERSION, (const char*)kanji_status.echoStr, &length,
						mikakuteiUTF, &revEnd, &state );
		revEnd += kakuteiUTFLen;
	}
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: UpdateKanjiStatus() returning 0x%x.\n", result ));
#endif
	return result;
}

int32
CannaInterface::GetMode()
{
	jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 2);
	char mode[5];
	jrKanjiControl( context_id, KC_QUERYMODE, mode );
	current_mode = mode[1] - '@';
//printf ("New mode = %x\n", current_mode );
	jrKanjiControl( context_id, KC_SETMODEINFOSTYLE, (char *)(int32) 0);
	return current_mode;
}


int
CannaInterface::ConvertSpecial(char ch, uint32 mod, int32 key)
{
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: ConvertSpecial ch = 0x%x, mod = 0x%x, key = 0x%x\n", ch, mod, key ));
#endif
	if (mod & B_CONTROL_KEY) {
		// if control key is held down, do not convert special key
		switch (ch) {
			case B_UP_ARROW:
				return CANNA_KEY_Cntrl_Up;
			case B_DOWN_ARROW:
				return CANNA_KEY_Cntrl_Down;
			case B_RIGHT_ARROW:
				return CANNA_KEY_Cntrl_Right;
			case B_LEFT_ARROW:
				return CANNA_KEY_Cntrl_Left;
			default:
				return ch;
		}
	}

	switch (ch) {
		case B_FUNCTION_KEY:
			switch (key) {
				case B_F1_KEY:
				case B_F2_KEY:
				case B_F3_KEY:
				case B_F4_KEY:
				case B_F5_KEY:
				case B_F6_KEY:
				case B_F7_KEY:
				case B_F8_KEY:
				case B_F9_KEY:
				case B_F10_KEY:
				case B_F11_KEY:
				case B_F12_KEY:
					return (int)CANNA_KEY_F1 + (int)key - (int)B_F1_KEY;
			}
			break;

		case B_INSERT:
			return CANNA_KEY_Insert;
		case B_PAGE_UP:
			return CANNA_KEY_Rollup;
		case B_PAGE_DOWN:
			return CANNA_KEY_Rolldown;
		case B_UP_ARROW:
			if (mod & B_SHIFT_KEY /* shifted */ )
				return CANNA_KEY_Shift_Up;
			return CANNA_KEY_Up;

		case B_DOWN_ARROW:
			if (mod & B_SHIFT_KEY /* shifted */ )
				return CANNA_KEY_Shift_Down;
			return CANNA_KEY_Down;

		case B_RIGHT_ARROW:
			if (mod & B_SHIFT_KEY /* shifted */ )
				return CANNA_KEY_Shift_Right;
			return CANNA_KEY_Right;

		case B_LEFT_ARROW:
			if (mod & B_SHIFT_KEY /* shifted */ )
				return CANNA_KEY_Shift_Left;
			return CANNA_KEY_Left;

		case B_END:
			return CANNA_KEY_Help;
		case B_HOME:
			return CANNA_KEY_Home;
	}

	return ch;
}


int
CannaInterface::ConvertArrowKey(int key)
{
	switch (key) {
		case CANNA_KEY_Up:
			return CANNA_KEY_Left;
		case CANNA_KEY_Down:
			return CANNA_KEY_Right;
		case CANNA_KEY_Shift_Up:
			return CANNA_KEY_Up;
		case CANNA_KEY_Shift_Down:
			return CANNA_KEY_Down;
		case CANNA_KEY_Right:
			return (uint8)B_RETURN;
		default:
			return key;
	}
}

/*
void CannaInterface::ClearPrevious()
{
	for ( long i = 0 ; i < CONVERT_BUFFER_SIZE * 2 ; i++ )
		previousUTF[i] = '\0';
}

void CannaInterface::GetModified( int32* from, int32* to, char** string )
{
	int32 i, previousLen;
	previousLen = strlen( previousUTF );

	for( i = 0 ;
		mikakuteiUTF[i] == previousUTF[i]
		&& mikakuteiUTF[i] != '\0'
		&& previousUTF[i] != '\0' ;
		i++ ) {}

	*from = i;

	if ( mikakuteiUTFLen > previousLen )
		*to = mikakuteiUTFLen;
	else
		*to = previousLen;

	*string = &mikakuteiUTF[ i ];
}

int32 CannaInterface::ForceKakutei()
{
	if ( !canna_enabled )
		return 0;

	jrKanjiStatusWithValue	ks;
	ks.val = 0;
	ks.buffer = (unsigned char *)kakuteiStr;
	ks.bytes_buffer = CONVERT_BUFFER_SIZE;
	ks.ks = &kanji_status;
	jrKanjiControl(context_id, KC_KAKUTEI, (char *)&ks);
	return ks.val;
}

bool CannaInterface::ReadSetting(char *path, BFont *aFont)
{
	BMessage pref;
	BFile preffile( INLINE_SETTING_FILE, B_READ_ONLY );
	if ( preffile.InitCheck() != B_NO_ERROR )
		return false;

	if ( pref.Unflatten( &preffile ) != B_OK )
		return false;

	font_family	fontfamily;
	float size;
	char *string;

	underline_color = FindColorData( &pref, "underline" );
	highlight_color = FindColorData( &pref, "highlight" );
	selection_color = FindColorData( &pref, "selection" );
	pref.FindString( "font", &string );
	strcpy( fontfamily, string );
	pref.FindString( "dictpath", &string );
	strcpy( path, string );
	pref.FindFloat( "size", &size );
	pref.FindBool( "arrow", &convert_arrowkey );

	aFont->SetFamilyAndStyle( fontfamily, NULL );
	aFont->SetSize( size );
	return true;
}

rgb_color CannaInterface::FindColorData( BMessage *msg, char *name )
{
	rgb_color result;
	msg->FindInt8( name, 0, (int8 *)&result.red );
	msg->FindInt8( name, 1, (int8 *)&result.green );
	msg->FindInt8( name, 2, (int8 *)&result.blue );
	msg->FindInt8( name, 3, (int8 *)&result.alpha );
	return result;
}
*/

bool CannaInterface::HasRev()
{
	if ( kanji_status.revLen == 0 )
		return false;
	else
		return true;
}

BMessage *
CannaInterface::GenerateKouhoString()
{
	int32 length = kanji_status.gline.length;
	int32 revposition = kanji_status.gline.revPos;
	int32 revend;
	int32 revposUTF;
	int32 revendUTF;
	int32 state;
	bool noindex, sizelimit, partialhighlight;
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: GenerateKouhoStr() revPos = %d, revLen = %d, mode = %d\n", revposition, kanji_status.gline.revLen, current_mode ));
#endif

	noindex = sizelimit = partialhighlight = false;

	kouhoUTFLen = KOUHO_WINDOW_MAXCHAR * 2;
	convert_to_utf8( B_EUC_CONVERSION, (const char*)kanji_status.gline.line, &length,
					kouhoUTF, &kouhoUTFLen, &state );
	kouhoUTF[ kouhoUTFLen ] = '\0';


	//find gline revpos by converting to UTF8
	if ( kanji_status.gline.revLen == 0 )
		kouhoRevLine = -1;
	else
	{
		revposUTF = KOUHO_WINDOW_MAXCHAR * 2;
		convert_to_utf8( B_EUC_CONVERSION, (const char*)kanji_status.gline.line, &revposition,
					kouhoUTF, &revposUTF, &state );
			//then, count full-spaces before revpos
		kouhoRevLine = 0;

		if ( current_mode == CANNA_MODE_TourokuMode
			|| ( kanji_status.gline.length != 0 && current_mode != CANNA_MODE_KigoMode
			&& 		current_mode != CANNA_MODE_IchiranMode
			&&		current_mode != CANNA_MODE_YesNoMode
			&& 		current_mode != CANNA_MODE_OnOffMode
			&&		current_mode != CANNA_MODE_BushuMode
			&& 		current_mode != CANNA_MODE_ExtendMode
			&& 		current_mode != CANNA_MODE_RussianMode
			&& 		current_mode != CANNA_MODE_GreekMode
			&& 		current_mode != CANNA_MODE_LineMode
			&& 		current_mode != CANNA_MODE_ChangingServerMode
//			&&		current_mode != CANNA_MODE_HenkanMethodMode
			&& 		current_mode != CANNA_MODE_DeleteDicMode
			&& 		current_mode != CANNA_MODE_TourokuHinshiMode
			&& 		current_mode != CANNA_MODE_TourokuDicMode
			&& 		current_mode != CANNA_MODE_MountDicMode )) // in Touroku mode, delimiter is '['and ']'
		{
			revendUTF = KOUHO_WINDOW_MAXCHAR * 2;
			revend = kanji_status.gline.revPos + kanji_status.gline.revLen;
			convert_to_utf8( B_EUC_CONVERSION, (const char*)kanji_status.gline.line, &revend,
						kouhoUTF, &revendUTF, &state );
			partialhighlight = true;
		}
		else
		{
			for ( long i = 0; i < revposUTF ; i++ )
			{
				if ( (uint8)kouhoUTF[ i ] == 0xe3
					&& (uint8)kouhoUTF[ i + 1 ] == 0x80
					&& (uint8)kouhoUTF[ i + 2 ] == 0x80 )
						kouhoRevLine++;
			}
		}

	}
//printf("KouhoRevLine = %d\n", kouhoRevLine );

	// setup title string
	switch ( current_mode )
	{
			case CANNA_MODE_IchiranMode:
				infoUTF[0] = '\0';
				break;
			case CANNA_MODE_HexMode:
				strcpy( infoUTF, "16進" );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_BushuMode:
				strcpy( infoUTF, "部首 " );
				sizelimit = true;
				break;
			case CANNA_MODE_ExtendMode:
				strcpy( infoUTF, "拡張 " );
				break;
			case CANNA_MODE_RussianMode:
				strcpy( infoUTF, "ロシア " );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_GreekMode:
				strcpy( infoUTF, "ギリシャ " );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_LineMode:
				strcpy( infoUTF, "罫線 " );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_TourokuMode:
				strcpy( infoUTF, "単語登録" );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_TourokuHinshiMode:
				strcpy( infoUTF, "品詞選択" );
				break;
			case CANNA_MODE_TourokuDicMode:
				strcpy( infoUTF, "辞書選択" );
				sizelimit = true;
				break;
			case CANNA_MODE_DeleteDicMode:
				strcpy( infoUTF, "単語削除" );
				noindex = true;
				sizelimit = true;
				break;
			case CANNA_MODE_MountDicMode:
				strcpy( infoUTF, "辞書" );
				noindex = true;
				break;
			case CANNA_MODE_KigoMode:
				strcpy( infoUTF, "記号 " );
				noindex = true;
				sizelimit = true;
				break;
			default:
				strcpy( infoUTF, "情報" );
				noindex = true;
		}

		//setup info string according to current mode
		char* index;
		int32 len;

		if (current_mode == CANNA_MODE_IchiranMode
			|| current_mode == CANNA_MODE_ExtendMode
			|| (current_mode == CANNA_MODE_TourokuHinshiMode
			    && kouhoRevLine != -1)
			|| current_mode == CANNA_MODE_TourokuDicMode
			|| current_mode == CANNA_MODE_BushuMode) {
			// remove first index
			memmove(kouhoUTF, kouhoUTF + 2, kouhoUTFLen - 1);

			// convert full-space to LF
			while ((index = strstr(kouhoUTF, "\xe3\x80\x80")) != NULL) {
				*index = '\x0a';
				len = strlen(index);
				memmove(index + 1, index + 5, len - 4);
			}
			kouhoUTFLen = strlen(kouhoUTF);
		}

		if (current_mode == CANNA_MODE_KigoMode) {
			char num[5];
			for (long i = 0; i < 4; i++) {
				num[i] = kouhoUTF[i + 3];
			}
			num[4] = '\0';
			strcat(infoUTF, num);
			kouhoUTFLen -= 9;
			memmove(kouhoUTF, kouhoUTF + 10, kouhoUTFLen);
		}

		if (current_mode == CANNA_MODE_KigoMode
			|| current_mode == CANNA_MODE_MountDicMode
			|| current_mode == CANNA_MODE_RussianMode
			|| current_mode == CANNA_MODE_LineMode
			|| current_mode == CANNA_MODE_GreekMode) {
			//convert full-space to LF
			while ((index = strstr(kouhoUTF, "\xe3\x80\x80")) != NULL) {
				*index = '\x0a';
				len = strlen(index);
				memmove(index + 1, index + 3, len - 2);
			}
			kouhoUTFLen = strlen(kouhoUTF);
		}
/*
		if ( current_mode == CANNA_MODE_TourokuMode
			|| ( kanji_status.gline.length != 0 && ( current_mode == CANNA_MODE_TankouhoMode
			|| 		current_mode == CANNA_MODE_TankouhoMode
			||		current_mode == CANNA_MODE_AdjustBunsetsuMode )))
		{
			//convert '[' and ']' to LF
			for ( index = kouhoUTF ; index < kouhoUTF + kouhoUTFLen ; index++ )
			{
				if ( *index == '[' || *index == ']' )
					*index = '\x0a';
			}
		}
*/
		if ( current_mode == CANNA_MODE_IchiranMode
			|| current_mode == CANNA_MODE_RussianMode
			|| current_mode == CANNA_MODE_LineMode
			|| current_mode == CANNA_MODE_GreekMode
			|| current_mode == CANNA_MODE_ExtendMode
			|| (current_mode == CANNA_MODE_TourokuHinshiMode
			    && kouhoRevLine != -1 )
			|| current_mode == CANNA_MODE_BushuMode
			|| current_mode == CANNA_MODE_TourokuDicMode
			|| current_mode == CANNA_MODE_MountDicMode )
		{

			//extract number display
			index = kouhoUTF + kouhoUTFLen - 1;
			while ( ( *index  >= '0' && *index <= '9' ) || *index == '/' )
				index--;
			strcat( infoUTF, index );

			//remove excess spaces before number display
			while ( *index == ' ' )
				*index-- = '\0';
		}
		BMessage* msg = new BMessage( KOUHO_WINDOW_SETTEXT );
		msg->AddString( "text", kouhoUTF );
		msg->AddString( "info", infoUTF );
		msg->AddBool( "index", noindex );
		msg->AddBool( "limit", sizelimit );
		if ( partialhighlight )
		{
			msg->AddBool( "partial", true );
			msg->AddInt32( "revbegin", revposUTF );
			msg->AddInt32( "revend", revendUTF );
		}
		else
			msg->AddInt32( "kouhorev", kouhoRevLine );
		return msg;
}

uint32 CannaInterface::ChangeMode( int32 mode )
{
	jrKanjiStatusWithValue ksv;
	ksv.ks = &kanji_status;
	ksv.buffer = (unsigned char *)kakuteiStr;
	ksv.bytes_buffer = CONVERT_BUFFER_SIZE;
	ksv.val = mode;

	jrKanjiControl( context_id, KC_CHANGEMODE, (char *)&ksv );
	kakuteiLen = ksv.val;
#ifdef DEBUG
		SERIAL_PRINT(( "CannaInterface: ChangeMode returned kakuteiLen = %d\n", kakuteiLen ));
		SERIAL_PRINT(( "CannaInterface: ChangeMode mikakuteiLen = %d\n", kanji_status.length ));
#endif

	return UpdateKanjiStatus();
}

uint32 CannaInterface::Kakutei()
{
#ifdef DEBUG
		SERIAL_PRINT(( "CannaInterface: Kakutei() called. mikL = %d\n", mikakuteiUTFLen ));
#endif
	if ( mikakuteiUTFLen == 0 )
		return NOTHING_TO_KAKUTEI;
	jrKanjiStatusWithValue ksv;
	ksv.ks = &kanji_status;
	ksv.buffer = (unsigned char *)kakuteiStr;
	ksv.bytes_buffer = CONVERT_BUFFER_SIZE;

	jrKanjiControl( context_id, KC_KAKUTEI, (char *)&ksv );
	kakuteiLen = ksv.val;
#ifdef DEBUG
		SERIAL_PRINT(( "CannaInterface: Kakutei() processed. kakL = %d, mikL = %d, info = 0x%x\n", kakuteiLen, kanji_status.length, kanji_status.info ));
#endif
	return UpdateKanjiStatus();
}

/*
void CannaInterface::SetTargetITView( BView *itView )
{
	kouhoWindow->Lock();
	kouhoWindow->SetTargetITView( itView );
	kouhoWindow->Unlock();
}


void CannaInterface::ShowPalette()
{
	if ( paletteApp != NULL && paletteApp->IsValid() )
		paletteApp->SendMessage( CPW_SHOW );
}

void CannaInterface::HidePalette()
{
	if ( paletteApp != NULL && paletteApp->IsValid() )
		paletteApp->SendMessage( CPW_HIDE );
}
*/

// RevStartPositionInChar() returns rev postion in 'Nth char in UTFed MikakuteiStr'.
// This function returns zero when rev postion is not found.

int32
CannaInterface::GetRevStartPositionInChar()
{
	int32 charNum;
	charNum = 0;

	if ( mikakuteiUTFLen == 0 )
		return 0;
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: GetRevStartPos revBegin = %d\n", revBegin ));
#endif

	for ( int32 i = 0 ; i < mikakuteiUTFLen ; i += UTF8CharLen( mikakuteiUTF[i] ) )
	{

		if ( i == revBegin )
		{
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: GetRevStartPos found. charNum= %d\n", charNum ));
#endif
			return charNum;
		}
		else
			charNum++;
	}
	return 0;
}

void
CannaInterface::Reset()
{
#ifdef DEBUG
SERIAL_PRINT(( "CannaInterface: Resetting canna...\n" ));
#endif
	jrKanjiControl( context_id, KC_FINALIZE, 0 );
	InitializeCanna();
}
