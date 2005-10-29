/*******************************************************************************
/
/	File:			NetPositive.h
/
/   Description:    Defines all public APIs for communicating with NetPositive
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_NETPOSITIVE_H
#define	_NETPOSITIVE_H

/*----------------------------------------------------------------*/
/*-----  message command constants -------------------------------*/

enum {
	/* Can be sent to the NetPositive application, a window, or a replicant */
	/* view.  Put the URL in a String field named be:url                    */
	B_NETPOSITIVE_OPEN_URL		= 'NPOP',	

	/* Can be sent to a window or replicant view */
	B_NETPOSITIVE_BACK			= 'NPBK',
	B_NETPOSITIVE_FORWARD		= 'NPFW',
	B_NETPOSITIVE_HOME			= 'NPHM',
	B_NETPOSITIVE_RELOAD		= 'NPRL',
	B_NETPOSITIVE_STOP		 	= 'NPST',
	B_NETPOSITIVE_DOWN			= 'NPDN',
	B_NETPOSITIVE_UP			= 'NPUP'
};
	
/*----------------------------------------------------------------*/
/*-----  NetPositive-related MIME types --------------------------*/

	/* The MIME types for the NetPositive application and its bookmark files */
#define B_NETPOSITIVE_APP_SIGNATURE  		"application/x-vnd.Be-NPOS"
#define B_NETPOSITIVE_BOOKMARK_SIGNATURE 	"application/x-vnd.Be-bookmark"

	/* To set up your application to receive notification when the user		 */
	/* clicks on a specific type of URL (telnet URL's, for example), see the */
	/* details in TypeConstants.h.  NetPositive will use external handlers	 */
	/* for all URL types except for http, https, file, netpositive, and		 */
	/* javascript, which it always handles internally.  To maintain			 */
	/* compatibility with its previous behavior, if NetPositive does not	 */
	/* find a handler for mailto URL's,	it will instead launch the handler	 */
	/* for "text/x-email".				 									 */

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

#endif /* _NETPOSITIVE_H */
