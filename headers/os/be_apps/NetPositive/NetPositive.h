/* 
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_NETPOSITIVE_H
#define	_NETPOSITIVE_H


// Message command constants
// These are not supported by WebPositive at this time and only exists for
// compile time backwards compatibility.
enum {
	// This message could be sent to the NetPositive application, a window, or a
	// replicant view. The receiver expected the URL in a "be:url" string field.
	B_NETPOSITIVE_OPEN_URL		= 'NPOP',	

	// These commands could be sent to a window or replicant view.
	B_NETPOSITIVE_BACK			= 'NPBK',
	B_NETPOSITIVE_FORWARD		= 'NPFW',
	B_NETPOSITIVE_HOME			= 'NPHM',
	B_NETPOSITIVE_RELOAD		= 'NPRL',
	B_NETPOSITIVE_STOP			= 'NPST',
	B_NETPOSITIVE_DOWN			= 'NPDN',
	B_NETPOSITIVE_UP			= 'NPUP'
};


// NetPositive related MIME types
// The first one is useless on Haiku, unless NetPositive was manually installed,
// the second one is still used for URL files saved by WebPositive.
#define B_NETPOSITIVE_APP_SIGNATURE  		"application/x-vnd.Be-NPOS"
#define B_NETPOSITIVE_BOOKMARK_SIGNATURE 	"application/x-vnd.Be-bookmark"


#endif // _NETPOSITIVE_H
