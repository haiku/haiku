#ifndef DATATRANSLATIONS_WINDOW_H
#define DATATRANSLATIONS_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

#include <Box.h>
#include <Button.h>
#include <SupportDefs.h>
#include <ListView.h>
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

#include "DataTranslationsSettings.h"
#include "DataTranslationsView.h"

class DataTranslationsWindow : public BWindow 
{
public:
	DataTranslationsWindow();
	~DataTranslationsWindow();
	
virtual	bool	QuitRequested();
virtual void    MessageReceived(BMessage* message);
virtual void    WindowActivated(bool state);
virtual void 	FrameMoved(BPoint origin);
		// void    WindowActivated(bool state);
		// void 	FrameMoved(BPoint origin);
		void	DrawIcon();
		bool	Install_Done; // True if an install just finished, but no List-Update done yet.

	
private:
		const char 		*translator_name, *translator_info;
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
				
		DataTranslationsView*	liste; // Improved list of Translators
		
		BStringView*	DTN; 		 // Display the DataTranslatorName
		BMessage 		archiv;		 // My Message
		BView*			Icon;		 // The Icon and Config - Views
		BBitmap*        Icon_bit;
		BView*			Konf;	
		
		BEntry     		entry; 
		BNode      		node; 
		BNodeInfo  		info;  
		
		void 			Trans_by_ID(int32 id);
		int				WriteTrans();	
		void			BuildView();
	
};

#endif // DATATRANSLATIONS_WINDOW_H
