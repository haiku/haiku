/*
 * DataTranslations.cpp
 * DataTranslations mccall@digitalparadise.co.uk
 *
 */

#include <Alert.h>
#include <Screen.h>

#include "DataTranslations.h"
#include "DataTranslationsWindow.h"
#include "DataTranslationsSettings.h"
#include "DataTranslationsMessages.h"

const char DataTranslationsApplication::kDataTranslationsApplicationSig[] = "application/x-vnd.OpenBeOS-prefs-translations";

int main(int, char**)
{
	DataTranslationsApplication	myApplication;

	myApplication.Run();

	return(0);
}

DataTranslationsApplication::DataTranslationsApplication()
					:BApplication(kDataTranslationsApplicationSig)
{

	DataTranslationsWindow		*window;
	
	fSettings = new DataTranslationsSettings();
		
	window = new DataTranslationsWindow();

}


/* Unused.
void
DataTranslationsApplication::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case ERROR_DETECTED:
			{
				BAlert *errorAlert = new BAlert("Error", "Unknown Error occured","OK",NULL,NULL,B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
				errorAlert->Go();
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;			
		default:
			BApplication::MessageReceived(message);
			break;
	}
}
*/
void
DataTranslationsApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}

void
DataTranslationsApplication::AboutRequested(void)
{
	(new BAlert("", "... written by Oliver Siebenmarck", "Cool!"))->Go();
	return;
}

DataTranslationsApplication::~DataTranslationsApplication()
{
	delete fSettings;
}

void DataTranslationsApplication::Install_Done()
{
	(new BAlert("", "You have to quit and restart running applications\nfor the installed Translators to be available in them.", "OK"))->Go();
	// aWindow->Install_Done  = true;
	return;
}

void DataTranslationsApplication::RefsReceived(BMessage *message)
{

	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;
	BString newfile;
	
	char e_name[B_FILE_NAME_LENGTH];
	char mbuf[256];
	
	
	message->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 
	
    
    if ( message->FindRef("refs", &ref) == B_OK )
    {
    	BEntry entry(&ref, true);
    	entry.GetName(e_name);
    	
    	if ( entry.IsFile() && entry.GetPath(&path)==B_OK)
    	{
    		BNode      node (&entry); 
	        BNodeInfo  info (&node); 
	        info.GetType (mbuf);
	        string.SetTo(mbuf);
		
			if (string.FindFirst("application/x-vnd.Be-elfexecutable") != B_ERROR )
			{
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
    			
    			if (moveit) // Just a quick move 
    			{
    				if (dirt.Contains(newfile.String()))
    				{
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
    				Install_Done(); 	 // Installation done
    				return;
    			}
    			
    			if (dirt.Contains(newfile.String()))
    			{
    				// File exists, What are we supposed to do now (Overwrite/_Stopp_?)
    				string.SetTo("");
    				string << "An item named '" << e_name <<"' already is in the \nTranslators folder";
    				BAlert *myAlert = new BAlert("title",  string.String() , "Overwrite", "Stop");
    				myAlert->SetShortcut(1, B_ESCAPE); 
					
					if (myAlert->Go() != 0) 
						return;
				}
    			
    			string.SetTo(path.Path()); // Fullpath+Filename 
    			
    			BString 
				command	=	"copyattr -d -r ";
				command << string.String() << " " << newfile.String(); // Prepare Copy 
				
				system(command.String()); // Execute copy command
    			
    			string.SetTo("");
    			string << "Filename: " << e_name << "\nPath: " << path.Path() ;
    			
    			// The new Translator has been installed by now.
    			// And done we are
    			Install_Done();
    			return;
    		}
    		else
    		{
    			// Not a Translator
    			no_trans(e_name);
    		}
    	}
    	else
    	{
    		// Not even a file...
    		no_trans(e_name);
    	}	
    }
}

void DataTranslationsApplication::no_trans(char item_name[B_FILE_NAME_LENGTH])
{
	BString text;
	text.SetTo("The item '");
	text << item_name << "' does not appear to be a Translator and will not be installed";
	(new BAlert("", text.String(), "Stop"))->Go();
	return;
}

