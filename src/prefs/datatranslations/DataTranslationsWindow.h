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
#include "IconView.h"

class DataTranslationsWindow : public BWindow 
{
public:
	DataTranslationsWindow();
	~DataTranslationsWindow();
	
virtual	bool	QuitRequested();
virtual void    MessageReceived(BMessage* message);
	
private:
		const char 		*translator_name, *translator_info;
		int32 			translator_version;
		const char* 	pfad;
		BString 		tex;
		bool fTranSelected;
			// Indicates whether or not a translator is selected
			// in the fTranListView
				
		BButton*		dButton;     // Default-Button    
		BBox*           fBox;        // Full-Window Box 
		BBox*			rBox;		 // Box hosting Config View
				
		DataTranslationsView *fTranListView;
			// List of Translators (left pane of window)
		
		BStringView*	DTN; 		 // Display the DataTranslatorName
		BMessage fRosterArchive;
			// Archive of the current BTranslatorRoster

		IconView *fIconView;
		
		BView*			Konf;	
		
		BEntry     		entry; 
		BNode      		node; 
		BNodeInfo  		info;  
		
		void 			Trans_by_ID(int32 id);
		int				WriteTrans();	
		void			BuildView();
	
};

#endif // DATATRANSLATIONS_WINDOW_H
