#ifndef DATA_TRANSLATIONS_H
#define DATA_TRANSLATIONS_H

#include <Application.h>
#include <String.h>
#include <Alert.h>

#include <stdlib.h>

#include "DataTranslationsWindow.h"
#include "DataTranslationsView.h"
#include "DataTranslationsSettings.h"

class DataTranslationsApplication : public BApplication 
{
public:
	DataTranslationsApplication();
	virtual ~DataTranslationsApplication();
	
	// void MessageReceived(BMessage *message);
	virtual void RefsReceived(BMessage *message);
	BPoint WindowCorner() const {return fSettings->WindowCorner(); }
	void SetWindowCorner(BPoint corner);

	void AboutRequested(void);
	
private:
	void 	Install_Done();
		
	static const char kDataTranslationsApplicationSig[];

	DataTranslationsSettings		*fSettings;
	
	// Tell User our installation is done	
	void 	no_trans(char item_name[B_FILE_NAME_LENGTH]); 
	// Tell User he didn´t drop a translator
	bool 	overwrite, moveit;
	BString string;	
};

#endif