/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#ifndef _CANNA_INTERFACE_H
#define _CANNA_INTERFACE_H

#include <stdio.h>
#include <Message.h>
#include <View.h>
#include <StorageDefs.h>

#include "canna/jrkanji.h"
#include "CannaCommon.h"
//#include "InlineTextViewDefs.h"
//#include "InlineKouhoWindow.h"

extern Preferences gSettings;

class CannaInterface
{
private:
	jrKanjiStatus		kanji_status;
//	InlineKouhoWindow*	kouhoWindow;
//	BMessenger*			paletteApp;
	bool				canna_enabled;
	bool				hadMikakuteiStr;
	bool				hadGuideLine;
	bool				convert_arrowkey;
	int					context_id;
//	BFont				kouhoFont;
//	rgb_color			underline_color;
//	rgb_color			highlight_color;
	rgb_color			selection_color;
	int32				kakuteiLen;
	int32				kakuteiUTFLen;
	int32				mikakuteiUTFLen;
	int32				kouhoUTFLen;
	int32				revBegin;
	int32				revEnd;
	int32				kouhoRevLine;
	int32				current_mode;
	char				basePath[ B_PATH_NAME_LENGTH + 1 ];
	char				kakuteiStr[ CONVERT_BUFFER_SIZE ];
	char				kakuteiUTF[ CONVERT_BUFFER_SIZE * 2 ];
	char				mikakuteiUTF[ CONVERT_BUFFER_SIZE * 2 ];
//	char				previousUTF[ CONVERT_BUFFER_SIZE * 2];
	char				kouhoUTF[ KOUHO_WINDOW_MAXCHAR * 2 ];
	char				infoUTF[ NUMBER_DISPLAY_MAXCHAR ];
	int					ConvertSpecial( char ch, uint32 mod, int32 key );
	int					ConvertArrowKey( int key );
//	void				ClearPrevious();
//	bool 				ReadSetting(char *path, BFont *aFont);
	uint32				UpdateKanjiStatus();
	void				InitializeCanna();


public:
						CannaInterface( char *basepath );
						~CannaInterface();
	//void				SetTargetITView( BView *itView );
	status_t			InitCheck();
	uint32				KeyIn( char ch, uint32 mod, int32 key );
	char*				GetKakuteiStr()
						{ return kakuteiUTF; };
	int32				KakuteiLength()
						{ return kakuteiUTFLen; };
	char*				GetMikakuteiStr()
						{ return mikakuteiUTF; };
	int32				MikakuteiLength()
						{ return mikakuteiUTFLen; };
//	void				GetModified( int32* from, int32* to, char** string );
//	int32				ForceKakutei();
	bool				HasRev();
	void				GetRevPosition( int32* begin, int32 *end )
						{ *begin = revBegin; *end = revEnd; };
	int32				GetRevStartPositionInChar();
	void				SetConvertArrowKey( bool convert )
						{ convert_arrowkey = convert; };
	uint32				ChangeMode( int32 mode );
	uint32				CurrentMode()
						{ return current_mode; };
	uint32				Kakutei();
	BMessage*			GenerateKouhoString();
	int32				GetMode();
	void				Reset();
	//void				HidePalette();
	//void				ShowPalette();
	//void				UpdateModePalette();
};

#endif
