//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Deskbar.cpp
//	Author:			Jeremy Rand (jrand@magma.ca), Jérôme Duval
//	Description:	BDeskbar allows one to control the deskbar from an
//                  application.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Deskbar.h>
#include <Messenger.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <InterfaceDefs.h>
#include <Node.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
static const char *gDeskbarSignature = "application/x-vnd.Be-TSKB";
static const uint32 gAddItemViewWhat = 'icon';
static const uint32 gAddItemAddonWhat = 'adon';
static const uint32 gHasItemWhat = 'exst';
static const uint32 gGetItemInfoWhat = 'info';
static const uint32 gCountItemsWhat = 'cwnt';
static const uint32 gRemoveItemWhat = 'remv';
static const uint32 gLocationWhat = 'gloc';
static const uint32 gIsExpandedWhat = 'gexp';
static const uint32 gSetLocationWhat = 'sloc';
static const uint32 gExpandWhat = 'sexp';


status_t get_deskbar_frame(BRect *frame)
{
	BMessenger deskbarMessenger(gDeskbarSignature);
	BMessage requestMessage(B_GET_PROPERTY);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddSpecifier("Frame");
	if (result == B_OK) {
		result = requestMessage.AddSpecifier("Window", "Deskbar");
		if (result == B_OK) {
			result = deskbarMessenger.SendMessage(&requestMessage, &replyMessage);
			if ((result == B_OK) &&
	            (replyMessage.what == B_REPLY)) {
	    		result = replyMessage.FindRect("result", frame);
	    	}
	    }
	}
	return(result);
}


//------------------------------------------------------------------------------
BDeskbar::BDeskbar()
	:	fMessenger(new BMessenger(gDeskbarSignature))
{
}
//------------------------------------------------------------------------------
BDeskbar::~BDeskbar()
{
	delete fMessenger;
}
//------------------------------------------------------------------------------
BRect BDeskbar::Frame(void) const
{
	BMessage requestMessage(B_GET_PROPERTY);
	BMessage replyMessage;
	BRect result(0.0, 0.0, 0.0, 0.0);

	if ((requestMessage.AddSpecifier("Frame") == B_OK) &&
	    (requestMessage.AddSpecifier("Window", "Deskbar") == B_OK) &&
		(fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK) &&
	    (replyMessage.what == B_REPLY)) {
	    replyMessage.FindRect("result", &result);
	}
	return(result);
}
//------------------------------------------------------------------------------
deskbar_location BDeskbar::Location(bool *isExpanded) const
{
	BMessage requestMessage(gLocationWhat);
	BMessage replyMessage;
	int32 result = 0;
	
	if ((fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK) &&
	    (replyMessage.what == 'rply') &&
	    (replyMessage.FindInt32("location", &result) == B_OK) &&
	    (isExpanded != NULL)) {
	    replyMessage.FindBool("expanded", isExpanded);	
	}
	return(static_cast<deskbar_location>(result));
}
//------------------------------------------------------------------------------
status_t BDeskbar::SetLocation(deskbar_location location, bool expanded)
{
	BMessage requestMessage(gSetLocationWhat);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddInt32("location", static_cast<int32>(location));
	if (result == B_OK) {
		result = requestMessage.AddBool("expand", expanded);
		if (result == B_OK) {
			result = fMessenger->SendMessage(&requestMessage, &replyMessage);
		}
	}
	return(result);
}
//------------------------------------------------------------------------------
bool BDeskbar::IsExpanded(void) const
{
	BMessage requestMessage(gIsExpandedWhat);
	BMessage replyMessage;
	bool result = false;
	
	if ((fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK) &&
	    (replyMessage.what == 'rply')) {
    	replyMessage.FindBool("expanded", &result);
	}
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::Expand(bool expand)
{
	BMessage requestMessage(gExpandWhat);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddBool("expand", expand);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
	}
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::GetItemInfo(int32 id, const char **name) const
{
	/* NOTE: Be's implementation returned B_BAD_VALUE if *name was NULL,
	   not just if name was NULL.  This doesn't make much sense.  Be's
	   implementation means you cannot do the following:
	   		
	   		const char *buffer = NULL;
	   		myDeskbar.GetItemInfo(id, &buffer);
	   
	   Instead, you are forced to write code that looks like:
	   
	   		char tmpBuf[10];
	   		const char *buffer = tmpBuf;
	   		myDeskbar.GetItemInfo(id, &buffer);
	   		
	   There are a couple of issues with this:
	   	- Be's implementation does not use the space pointed to in buffer.
	   	  It cannot since it can't tell how big that space is and it won't
	   	  know whether the item's name will fit without overflowing the
	   	  buffer.
	   	- Worse, if the code looked like:
	   	
	   		const char *buffer = new char[5];
	   		myDeskbar.GetItemInfo(id, &buffer);
	      
	       The code will result in a memory leak.  The problem here is that
	       what buffer points to is changed by GetItemInfo().  If buffer
	       points to dynamically allocated memory, there is a good chance
	       the result is a memory leak.
	   
	   The OpenBeOS implementation will allow *name to point to NULL or
	   non-NULL.  If anything, we should consider forcing *name to point to
	   NULL for safety.
	 */
	if (name == NULL) {
		return(B_BAD_VALUE);
	}
	
	BMessage requestMessage(gGetItemInfoWhat);
	BMessage replyMessage;
	status_t result; 
	
	result = requestMessage.AddInt32("id", id);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
		if (result == B_OK) {
			const char *tmpName;
			result = replyMessage.FindString("name", &tmpName);
			if (result == B_OK) {
				*name = strdup(tmpName);
			}
		}
	}
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::GetItemInfo(const char *name, int32 *id) const
{
	if (name == NULL) {
		return(B_BAD_VALUE);
	}
	
	BMessage requestMessage(gGetItemInfoWhat);
	BMessage replyMessage;
	status_t result; 
	
	result = requestMessage.AddString("name", name);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
		if (result == B_OK) {
			result = replyMessage.FindInt32("id", id);
		}
	}
	return(result);
}
//------------------------------------------------------------------------------
bool BDeskbar::HasItem(int32 id) const
{
	BMessage requestMessage(gHasItemWhat);
	BMessage replyMessage;
	bool result = false;
	
	if ((requestMessage.AddInt32("id", id) == B_OK) &&
	    (fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK)) {
		replyMessage.FindBool("exists", &result);
	}
	return(result);
}
//------------------------------------------------------------------------------
bool BDeskbar::HasItem(const char *name) const
{
	BMessage requestMessage(gHasItemWhat);
	BMessage replyMessage;
	bool result = false;
	
	if ((requestMessage.AddString("name", name) == B_OK) &&
	    (fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK)) {
		replyMessage.FindBool("exists", &result);
	}
	return(result);
}
//------------------------------------------------------------------------------
uint32 BDeskbar::CountItems(void) const
{
	BMessage requestMessage(gCountItemsWhat);	
	BMessage replyMessage;
	int32 result = 0;
	
	if ((fMessenger->SendMessage(&requestMessage, &replyMessage) == B_OK) &&
	    (replyMessage.what == 'rply')) {
		replyMessage.FindInt32("count", &result);
	}
	return(static_cast<uint32>(result));
}
//------------------------------------------------------------------------------
status_t BDeskbar::AddItem(BView *archivableView, int32 *id)
{
	BMessage requestMessage(gAddItemViewWhat);
	BMessage viewMessage;
	BMessage replyMessage;
	status_t result;
	
	result = archivableView->Archive(&viewMessage);
	if (result == B_OK) {
		result = requestMessage.AddMessage("view", &viewMessage);
		if (result == B_OK) {
			result = fMessenger->SendMessage(&requestMessage, &replyMessage);
			if ((result == B_OK) &&
			    (id != NULL)) {
				result = replyMessage.FindInt32("id", id);
			}
		}
	}
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::AddItem(entry_ref *addon, int32 *id)
{
	BMessage requestMessage(gAddItemViewWhat);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddRef("addon", addon);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
		if ((result == B_OK) &&
		    (id != NULL)) {
			result = replyMessage.FindInt32("id", id);
			
		/* NOTE: I add this because the persistent state of the item
			is not set by the Deskbar itself and it needs to be done somewhere.
			In fact, telling the Deskbar about the addon ref is not enough and
			adding this attribute is mandatory.
			!! Deskbar seems to remove the attribute in RemoveItem though !!
		*/
			if (result == B_OK) {			
				BNode node(addon);
				node.WriteAttr("be:deskbar_item_status", 
					B_STRING_TYPE, 0, "enabled", strlen("enabled"));
			}
		}
	}
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::RemoveItem(int32 id)
{
	BMessage requestMessage(gRemoveItemWhat);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddInt32("id", id);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
	}
	/* here R5 returns B_OK always (probably the result of SendMessage())
	 * Deskbar itself also always return B_NO_REPLY, so ...
	 * here add some more checks for the future.
	 */
	if (result != B_OK)
		return(result);
	if (replyMessage.what == B_NO_REPLY)
		return(B_OK); /* we can only speculate */
	result = B_ERROR;
	replyMessage.FindInt32("error", &result);
	return(result);
}
//------------------------------------------------------------------------------
status_t BDeskbar::RemoveItem(const char *name)
{
	BMessage requestMessage(gRemoveItemWhat);
	BMessage replyMessage;
	status_t result;
	
	result = requestMessage.AddString("name", name);
	if (result == B_OK) {
		result = fMessenger->SendMessage(&requestMessage, &replyMessage);
	}
	/* same as above */
	if (result != B_OK)
		return(result);
	if (replyMessage.what == B_NO_REPLY)
		return(B_OK);
	result = B_ERROR;
	replyMessage.FindInt32("error", &result);
	return(result);
	
}
//------------------------------------------------------------------------------

