/*
	
	transwin.h
	
*/

#ifndef TRANS_WIN_H
#define TRANS_WIN_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

#ifndef LVIEW_H
#include "LView.h"
#endif

#define BUTTON_MSG 'PRES'
const uint32 SEL_CHANGE	= 'SEch';


#include <Box.h>
#include <Button.h>
#include <SupportDefs.h>
#include <ListView.h>
#include <string.h>
#include <TranslatorRoster.h>
#include <TranslationDefs.h>
#include <ScrollView.h>
#include <Alert.h>
#include <String.h>
#include <StringView.h>
#include <Bitmap.h>
#include <NodeInfo.h> 
#include <storage/Path.h>
#include <storage/Directory.h>
#include <storage/Entry.h>


class DATWindow : public BWindow 
{
public:
				DATWindow(BRect frame); 
virtual	bool	QuitRequested();
virtual void    MessageReceived(BMessage* message);
virtual void    WindowActivated(bool state);
virtual void 	FrameMoved(BPoint origin);

		void	DrawIcon();

private:
		const char *translator_name, *translator_info;
		int32 			translator_version;
		const char* 	pfad;
		BString 		tex;
		bool			showInfo;
		bool			newIcon;
				
		BButton*		dButton;     // Default-Button    
		BBox*           fBox;        // Full-Window Box 
		BScrollView*    tListe;      // To get that fancy scrollbar
		BBox*			lBox;		 // Box around list
		BBox*			rBox;		 // Box hosting Config View
		BBox*			gBox;		 // Box around Config View, funny border 
		
		// BListView*      list;        // My lovely List of Translators no longer needed
		
		LView*			liste;		 // Improved list of Translators
		
		BStringView*	DTN; 		 // Display the DataTranslatorName
		BMessage 		archiv;		 // My Message
		BView*			Icon;		  // The Icon and Config - Views
		BBitmap*        Icon_bit;
		BView*			Konf;
		
		
		BEntry     		entry; 
		BNode      		node; 
		BNodeInfo  		info;  		
		
		void 			GetDrop(BMessage *message);
		void 			Trans_by_ID(int32 id);
		int				WriteTrans();					

};

#endif //TRANS_WIN_H