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
//	File Name:		RosterPrivate.cpp
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Access class for BRoster members
//------------------------------------------------------------------------------

#include <RosterPrivate.h>
#include <Roster.h>

/*!	\class BRoster::Private
	\brief Class used to access private BRoster members.

	This way, the only friend BRoster needs is this class.
*/

// SetTo
/*!	\brief Initializes the roster.

	\param mainMessenger A BMessenger targeting the registrar application.
	\param mimeMessenger A BMessenger targeting the MIME manager.
*/
void
BRoster::Private::SetTo(BMessenger mainMessenger, BMessenger mimeMessenger)
{
	if (fRoster) {
		fRoster->fMess = mainMessenger;
		fRoster->fMimeMess = mimeMessenger;
	}
}

// SendTo
/*!	\brief Sends a message to the registrar.

	\a mime specifies whether to send the message to the roster or to the
	MIME data base service.
	If \a reply is not \c NULL, the function waits for a reply.

	\param message The message to be sent.
	\param reply A pointer to a pre-allocated BMessage into which the reply
		   message will be copied.
	\param mime \c true, if the message should be sent to the MIME data base
		   service, \c false for the roster.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a message.
	- \c B_NO_INIT: the roster is \c NULL.
	- another error code
*/
status_t
BRoster::Private::SendTo(BMessage *message, BMessage *reply, bool mime)
{
	status_t error = (message ? B_OK : B_BAD_VALUE);
	if (error == B_OK && !fRoster)
		error = B_NO_INIT;
	if (error == B_OK) {
		if (mime)
			error = fRoster->fMimeMess.SendMessage(message, reply);
		else
			error = fRoster->fMess.SendMessage(message, reply);
	}
	return error;
}

// IsMessengerValid
/*!	\brief Returns whether the roster's messengers are valid.

	\a mime specifies whether to check the roster messenger or the one of
	the MIME data base service.

	\param mime \c true, if the MIME data base service messenger should be
		   checked, \c false for the roster messenger.
	\return \true, if the selected messenger is valid, \c false otherwise.
*/
bool
BRoster::Private::IsMessengerValid(bool mime) const
{
	return (fRoster && (mime ? fRoster->fMimeMess.IsValid()
							 : fRoster->fMess.IsValid()));
}


// _init_roster_
/*!	\brief Initializes the global be_roster variable.

	Called before the global constructors are invoked.

	\return Unknown!

	\todo Investigate what the return value means.
*/
int
_init_roster_()
{
	be_roster = new BRoster;
	return 0;
}

// _delete_roster_
/*!	\brief Deletes the global be_roster.

	Called after the global destructors are invoked.

	\return Unknown!

	\todo Investigate what the return value means.
*/
int
_delete_roster_()
{
	delete be_roster;
	return 0;
}

