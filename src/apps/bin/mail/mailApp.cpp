#include <storage/AppFileInfo.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/FindDirectory.h>

#include "mailApp.h"
#include <String.h>
#include <E-mail.h>
#include <stdio.h>

 
 
#define APP_SIG				"application/x-vnd.OBos-cmd-mail"


mailApp :: mailApp() : BApplication(APP_SIG)
{
}

mailApp :: ~mailApp()
{
   // empty
}
 
void mailApp :: sendMail(const char *subject, const char*body, const char *to, const char *cc, const char *bcc)
{
 		BMailMessage *mail;
 		mail = new BMailMessage();
 		mail->AddHeaderField(B_MAIL_TO, to);
 		mail->AddHeaderField(B_MAIL_CC, cc);
 		mail->AddHeaderField(B_MAIL_BCC, bcc);
 		mail->AddHeaderField(B_MAIL_SUBJECT, subject); 
 		mail->AddContent(body,strlen(body));
 		status_t result =  mail->Send(); 
 		
 		if (result==B_OK)
 			fprintf(stdout, "\nMessage was sent successfully.");
}

  