// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        mail.cpp
//  Author:      Santiago (Jacques) Lema
//  Description: sends an e-mail from the command-line
//  Created :    May 23, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <OS.h>
#include <String.h>
#include <Application.h>

#include "mailApp.h"

int main( int argc, char* argv[] )
{

   // No arguments, show usage
	if (argc==1) {
		fprintf(stdout,"[OBOS-mail] Sorry: This program can only send mail, not read it.\n");
		fprintf(stdout,"usage: /bin/mail [-v] [-s subject] [-c cc-addr] [-b bcc-addr] to-addr ...\n");
		fflush(stdout);		
		return B_OK;
	}
	
	char *subject ="No title";
	char *cc = ""; 
	char *bcc = "";
	BString to = "";
	BString body="";

	bool verbose =false; 
		//parse arguments		
		for (int i=1; i<argc; i++) 
		{
	
			if (strcmp (argv[i], "-v") == 0)
				verbose = true;
			else 
				if (strcmp (argv[i], "-s") == 0)
				{
	 				subject = argv[i+1]; 
	 				i++;
				}
			else 
				if (strcmp (argv[i], "-c") == 0)
				{
	 				cc = argv[i+1]; 
	 				i++;
	 			}
			else 
				if (strcmp (argv[i], "-b") == 0)
				{
	 				bcc = argv[i+1]; 
	 				i++;
				}
			else 
			{
  					to.Append( argv[i] );
 					if (i<argc-1)
 						to.Append(" ");
   			}		

				
		}

//		fprintf(stdout,"[ OBOS command-line mail sender ]\n");
		
//		if (verbose)
//			fprintf(stdout,"Verbose on\n");
//		 else
//			fprintf(stdout,"Verbose off\n");
		
		if (verbose)
		{
			fprintf(stdout,"\n");
			fprintf(stdout,"To:\t<%s> \n",to.String());
			fprintf(stdout,"Cc:\t<%s> \n",cc);
			fprintf(stdout,"Bcc:\t<%s> \n",bcc);
			fprintf(stdout,"Subj:\t<%s> \n",subject);
			fprintf(stdout,"Body:\t<%s> \n",body.String());
	    	fprintf(stdout,"\n");
		}
		//read each line until we get a single dot "." on a line
		

		//check if valid recipients
	    if(
	    strcmp (to.String(), "") == 0
	    && strcmp (cc, "") == 0
	    && strcmp (bcc, "") == 0
         )
		{
			fprintf(stdout,"[Error]\nYou must specify at least one recipient in to,cc or bcc fields.\n");
			return B_ERROR;
		}
		
		char line[32768] =""; 

		printf("Now type your message.\nType '.' alone on a line to send it.\n"); 
		do
		{ 
		    gets(line);
		    
		    if(strcmp (line, ".") != 0)
		    {
		    	body.Append(line);
			    body.Append("\n");
			}    
		    //fprintf(stdout,"Line: %s \n",line);
		}while (strcmp (line, ".") != 0);

		
		if (verbose)
    	fprintf(stdout,"\nBody:\n%s\n",body.String());

		if (verbose)
    	fprintf(stdout,"\nSending E-mail...\n");
		
		 
 		 fflush(stdout); 
 		
 		 mailApp *be_app = new mailApp(); 
//    	 be_app->Run(); 
         be_app->sendMail(subject, body.String(),to.String(),cc,bcc); 
         delete be_app; 


}
