/*

Media by Sikosis

(C)2003

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <StorageKit.h>
#include <String.h>
#include <stdio.h>
#include "Media.h"

// Media -- constructor 
Media::Media() 
: BApplication (APP_SIGNATURE)
{
	BRect rect(32,64,637,442);
	
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_ONLY);
		if(file.InitCheck()==B_OK) {
			char buffer[255];
			ssize_t size = 0;
			while((size = file.Read(buffer, 255))>0) {
				int32 i=0;
				while(buffer[i]=='#') {
					while(i<size&&buffer[i]!='\n')
						i++;
					i++;
				}
				int32 a,b,c,d;
				if(sscanf(&buffer[i], " rect = %i,%i,%i,%i", &a, &b, &c, &d)>0){
					if(c-a>=int(rect.Width())) {
						rect.left = a;
						rect.right = c;
					}
					if(d-b>=int(rect.Height())) {
						rect.top = b;
						rect.bottom = d;
					}
				}
			}		
		}
	}

	mWindow = new MediaWindow(rect);
	mWindow->SetSizeLimits(605.0, 10000.0, 378.0, 10000.0);
}
// ---------------------------------------------------------------------------------------------------------- //

// Media::MessageReceived -- handles incoming messages
/*void Media::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
	    default:
    	    BApplication::MessageReceived(message); // pass it along ... 
        	break;
    }
}*/
// ---------------------------------------------------------------------------------------------------------- //

// Media Main
int main(void)
{
   Media theApp;
   theApp.Run();
   return 0;
}
// end ------------------------------------------------------------------------------------------------------ //
