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
		BBox *fConfigBox;
			// Box hosting Config View
				
		DataTranslationsView *fTranListView;
			// List of Translators (left pane of window)
		
		BStringView *fTranNameView;
			// Display the DataTranslatorName

		IconView *fIconView;
		
		BView*			Konf;
		
	void GetTranInfo(int32 id, const char *&tranName, const char *&tranInfo, int32 &tranVersion, BPath &tranPath);
		int				WriteTrans();	
		void			BuildView();
	
};

#endif // DATATRANSLATIONS_WINDOW_H
