// ClipboardHandler.cpp

#include <Message.h>
#include <RegistrarDefs.h>

#include "ClipboardHandler.h"

// bonefish: TODO: Cleanup/reimplement! The ClipboardTree doesn't need to be a
// tree at all. Strip it off fLeftChild, fRightChild, and fCount and we have a
// server-side clipboard representation. Simply use a name->clipboard map to
// manage the clipboards. The clipboard count (which doesn't work at the
// moment) will be the size of the map.


/*!
	\class ClipboardHandler
	\brief Handles all clipboard related requests.
*/

// constructor
/*!	\brief Creates and initializes a ClipboardHandler.
*/
ClipboardHandler::ClipboardHandler()
				: BHandler()
{
}

// destructor
/*!	\brief Frees all resources associate with this object.
*/
ClipboardHandler::~ClipboardHandler()
{
}

// MessageReceived
/*!	\brief Overrides the super class version to handle the clipboard specific
		   messages.
	\param message The message to be handled
*/
void
ClipboardHandler::MessageReceived(BMessage *message)
{
	BString name;
	BMessage reply;
	switch (message->what) {
		case B_REG_ADD_CLIPBOARD:
		{
	  		if ( message->FindString("name",&name) != B_OK )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				fClipboardTree.AddNode(name);
				reply.AddInt32("result",B_OK);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
		}
	  	break;
		case B_REG_GET_CLIPBOARD_COUNT:
		{
	  		if ( message->FindString("name",&name) != B_OK )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				ClipboardTree *node = fClipboardTree.GetNode(name);
				if ( node )
				{
					reply.AddInt32("count",(uint32)(node->GetCount()));
					reply.AddInt32("result",B_OK);
				}
				else
					reply.AddInt32("result",B_BAD_VALUE);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
		}
	  	break;
		case B_REG_CLIPBOARD_START_WATCHING:
		{
			BMessenger target;
	  		if ( (message->FindString("name",&name) != B_OK) ||
			     (message->FindMessenger("target",&target) != B_OK) )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				ClipboardTree *node = fClipboardTree.GetNode(name);
				if ( node && node->AddWatcher(&target) )
					reply.AddInt32("result",B_OK);
				else
					reply.AddInt32("result",B_BAD_VALUE);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
		}
	  	break;
		case B_REG_CLIPBOARD_STOP_WATCHING:
		{
			BMessenger target;
	  		if ( (message->FindString("name",&name) != B_OK) ||
			     (message->FindMessenger("target",&target) != B_OK) )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				ClipboardTree *node = fClipboardTree.GetNode(name);
				if ( node && node->RemoveWatcher(&target) )
					reply.AddInt32("result",B_OK);
				else
					reply.AddInt32("result",B_BAD_VALUE);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
		}
	  	break;
		case B_REG_DOWNLOAD_CLIPBOARD:
		{
	  		if ( message->FindString("name",&name) != B_OK )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				ClipboardTree *node = fClipboardTree.GetNode(name);
				if ( node )
				{
					reply.AddMessage("data",node->GetData());
					reply.AddMessenger("data source",*node->GetDataSource());
					reply.AddInt32("count",(uint32)(node->GetCount()));
					reply.AddInt32("result",B_OK);
				}
				else
					reply.AddInt32("result",B_BAD_VALUE);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
		}
	  	break;
		case B_REG_UPLOAD_CLIPBOARD:
		{
			BMessage data;
			BMessenger dataSource;
			ClipboardTree *node = NULL;
	  		if ( (message->FindString("name",&name) != B_OK) ||
			     (message->FindMessage("data",&data) != B_OK) ||
			     (message->FindMessenger("data source",&dataSource) != B_OK) )
			{
				reply.AddInt32("result",B_BAD_VALUE);
			}
			else
			{
				node = fClipboardTree.GetNode(name);
				if ( node )
				{
					node->SetData(&data);
					node->SetDataSource(&dataSource);
					reply.AddInt32("count",(uint32)(node->IncrementCount()));
					reply.AddInt32("result",B_OK);
				}
				else
					reply.AddInt32("result",B_BAD_VALUE);
			}
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
			if ( node )
			  node->NotifyWatchers();
		}
	  	break;
		default:
			BHandler::MessageReceived(message);
			break;
	}
}



