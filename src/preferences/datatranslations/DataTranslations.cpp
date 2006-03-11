/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */


#include "DataTranslations.h"
#include "DataTranslationsWindow.h"
#include "DataTranslationsSettings.h"

#include <Alert.h>
#include <Directory.h>
#include <TranslatorRoster.h>
#include <TextView.h>


const char* kApplicationSignature = "application/x-vnd.haiku-translations";


DataTranslationsApplication::DataTranslationsApplication()
	: BApplication(kApplicationSignature)
{
	fSettings = new DataTranslationsSettings();
	new DataTranslationsWindow();
}


DataTranslationsApplication::~DataTranslationsApplication()
{
	delete fSettings;
}


void
DataTranslationsApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}


void
DataTranslationsApplication::AboutRequested()
{
	BAlert *alert = new BAlert("about", "DataTranslations\n"
		"\twritten by Oliver Siebenmarck and others\n"
		"\tCopyright 2002-2006, Haiku.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 16, &font);

	alert->Go();
}


void
DataTranslationsApplication::InstallDone()
{
	(new BAlert("",
		"You have to quit and restart running applications\n"
		"for the installed Translators to be available for them.",
		"OK"))->Go();
}


void
DataTranslationsApplication::RefsReceived(BMessage *message)
{
	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;
	BString newfile;
	
	char e_name[B_FILE_NAME_LENGTH];
		
	message->GetInfo("refs", &type, &count); 
	if (type != B_REF_TYPE)
		return; 
	
    if (message->FindRef("refs", &ref) == B_OK) {
    	BEntry entry(&ref, true);
    	entry.GetName(e_name);
    	
    	if (entry.IsFile() && entry.GetPath(&path) == B_OK) {
    		BTranslatorRoster *roster = BTranslatorRoster::Default();
			if (roster->AddTranslators(path.Path()) == B_OK) {
    			BDirectory dirt("/boot/home/config/add-ons/Translators/");
    			newfile.SetTo("/boot/home/config/add-ons/Translators/");
    			newfile << e_name; // Newfile with full path.
    			
    			// Find out wether we need to copy it
    			string.SetTo("");
    			string << "The item '" << e_name << "' is now copied into the right place.\nWhat do you want to do with the original?";
    			BAlert *keep = new BAlert("keep",  string.String() , "Remove", "Keep");
    			keep->SetShortcut(1, B_ESCAPE);
    			if (keep->Go() == 0)
    				moveit = true;
    			
    			if (moveit) {
    				// Just a quick move 
    				if (dirt.Contains(newfile.String())) {
    					string.SetTo("");
    					string << "An item named '" << e_name <<"' already is in the \nTranslators folder";
    					BAlert *myAlert = new BAlert("title",  string.String() , "Overwrite", "Stop");
    					myAlert->SetShortcut(1, B_ESCAPE); 

						if (myAlert->Go() != 0) 
							return;
						// File exists, we are still here. Kill it.
						BEntry dele;
						dirt.FindEntry(e_name, &dele);
						dele.Remove();	
					}
					entry.MoveTo(&dirt); // Move it.
    				InstallDone(); 	 // Installation done
    				return;
    			}

    			if (dirt.Contains(newfile.String())) {
    				// File exists, What are we supposed to do now (Overwrite/_Stopp_?)
    				string.SetTo("");
    				string << "An item named '" << e_name <<"' already is in the \nTranslators folder";
    				BAlert *myAlert = new BAlert("title",  string.String() , "Overwrite", "Stop");
    				myAlert->SetShortcut(1, B_ESCAPE); 

					if (myAlert->Go() != 0) 
						return;
				}

    			string.SetTo(path.Path()); // Fullpath+Filename 

    			BString command;
#ifdef __HAIKU__
				command	= "cp ";
#else
				command	= "copyattr -d -r ";
#endif
				command << string.String() << " " << newfile.String(); // Prepare Copy 

				system(command.String()); // Execute copy command

    			string.SetTo("");
    			string << "Filename: " << e_name << "\nPath: " << path.Path() ;

    			// The new Translator has been installed by now.
    			// And done we are
    			InstallDone();
    		} else {
    			// Not a Translator
    			NoTranslatorError(e_name);
    		}
    	} else {
    		// Not even a file...
    		NoTranslatorError(e_name);
    	}	
    }
}


void
DataTranslationsApplication::NoTranslatorError(const char* name)
{
	BString text;
	text.SetTo("The item '");
	text << name << "' does not appear to be a Translator and will not be installed.";
	(new BAlert("", text.String(), "Stop"))->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	DataTranslationsApplication	app;
	app.Run();

	return 0;
}

