/* BMailProtocol - the base class for protocol filters
**
** Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <String.h>
#include <Alert.h>
#include <Query.h>
#include <VolumeRoster.h>
#include <StringList.h>
#include <E-mail.h>
#include <NodeInfo.h>
#include <Directory.h>
#include <NodeMonitor.h>
#include <Node.h>

#include <stdio.h>
#include <fs_attr.h>
#include <malloc.h>
#include <assert.h>

#include <MDRLanguage.h>

class BMailProtocol;

#include <MailProtocol.h>
#include <ChainRunner.h>
#include <status.h>

namespace {

class ManifestAdder : public BMailChainCallback {
	public:
		ManifestAdder(BStringList *list,BStringList **list2, const char *id) : manifest(list), uids_on_disk(list2), uid(id) {}
		virtual void Callback(status_t result) {
			if (result == B_OK) {
				(*manifest) += uid;
				if (*uids_on_disk != NULL)
					(**uids_on_disk) += uid;
			}
		}
		
	private:
		BStringList *manifest,**uids_on_disk;
		const char *uid;
};

class MessageDeletion : public BMailChainCallback {
	public:
		MessageDeletion(BMailProtocol *home, const char *uid, BEntry *io_entry, bool delete_anyway);
		virtual void Callback(status_t result);
		
	private:
		BMailProtocol *us;
		bool always;
		const char *message_id;
		BEntry *entry;
};


inline void
BMailProtocol::error_alert(const char *process, status_t error)
{
	BString string;
	MDR_DIALECT_CHOICE (
		string << "Error while " << process << ": " << strerror(error);
		runner->ShowError(string.String());
	,
		string << process << "中にエラーが発生しました: " << strerror(error);
		runner->ShowError(string.String());
	)
}


class DeleteHandler : public BHandler {
	public:
		DeleteHandler(BMailProtocol *a)
			: us(a)
		{
		}

		void MessageReceived(BMessage *msg)
		{
			if ((msg->what == 'DELE') && (us->InitCheck() == B_OK)) {
				us->CheckForDeletedMessages();
				Looper()->RemoveHandler(this);
				delete this;
			}
		}

	private:
		BMailProtocol *us;
};

class TrashMonitor : public BHandler {
	public:
		TrashMonitor(BMailProtocol *a, int32 chain_id)
			: us(a), trash("/boot/home/Desktop/Trash"), messages_for_us(0), id(chain_id)
		{
		}

		void MessageReceived(BMessage *msg)
		{
			if (msg->what == 'INIT') {
				node_ref to_watch;
				trash.GetNodeRef(&to_watch);
				watch_node(&to_watch,B_WATCH_DIRECTORY,this);
				return;
			}
			if ((msg->what == B_NODE_MONITOR) && (us->InitCheck() == B_OK)) {
			 	int32 opcode;
				if (msg->FindInt32("opcode",&opcode) < B_OK)
					return;
					
				if (opcode == B_ENTRY_MOVED) {
					int64 node(msg->FindInt64("to directory"));
					dev_t device(msg->FindInt32("device"));
					node_ref item_ref;
					item_ref.node = node;
					item_ref.device = device;
					
					BDirectory moved_to(&item_ref);
					
					BNode trash_item(&moved_to,msg->FindString("name"));
					int32 chain;
					if (trash_item.ReadAttr("MAIL:chain",B_INT32_TYPE,0,&chain,sizeof(chain)) < B_OK)
						return;
						
					if (chain == id)
						messages_for_us += (moved_to == trash) ? 1 : -1;
				}
				
				if (messages_for_us < 0)
					messages_for_us = 0; // Guard against weirdness
				
			 	if (trash.CountEntries() == 0) {
			 		if (messages_for_us > 0)
						us->CheckForDeletedMessages();
						
					messages_for_us = 0;
				}
			}
		}

	private:
		BMailProtocol *us;
		BDirectory trash;
		int32 messages_for_us;
		int32 id;
};

BMailProtocol::BMailProtocol(BMessage *settings, BMailChainRunner *run)
	: BMailFilter(settings),
	runner(run), trash_monitor(NULL), uids_on_disk(NULL)
{
	unique_ids = new BStringList;
	BMailProtocol::settings = settings;

	manifest = new BStringList;
	
	{
		BString attr_name = "MAIL:";
		attr_name << runner->Chain()->ID() << ":manifest"; //--- In case someone puts multiple accounts in the same directory
		
		if (runner->Chain()->MetaData()->HasString("path")) {
			BNode node(runner->Chain()->MetaData()->FindString("path"));
			if (node.InitCheck() >= B_OK) {
				attr_info info;
				if (node.GetAttrInfo(attr_name.String(),&info) < B_OK) {
					if (runner->Chain()->MetaData()->FindFlat("manifest", manifest) == B_OK) {
						runner->Chain()->MetaData()->RemoveName("manifest");
						runner->Chain()->Save(); //--- Not having this code made an earlier version of MDR delete all my *(&(*& mail
					}
				} else {
					void *flatmanifest = malloc(info.size);
					node.ReadAttr(attr_name.String(),manifest->TypeCode(),0,flatmanifest,info.size);
					manifest->Unflatten(manifest->TypeCode(),flatmanifest,info.size);
					free(flatmanifest);
				}
			} else runner->ShowError("Error while reading account manifest: cannot use destination directory.");
		} else runner->ShowError("Error while reading account manifest: no destination directory exists.");
	}
	
	uids_on_disk = new BStringList;
	BVolumeRoster volumes;
	BVolume volume;
	while (volumes.GetNextVolume(&volume) == B_OK) {
		BQuery fido;
		entry_ref entry;
	
		fido.SetVolume(&volume);
		fido.PushAttr("MAIL:chain");
		fido.PushInt32(settings->FindInt32("chain"));
		fido.PushOp(B_EQ);
		if (!settings->FindBool("delete_remote_when_local")) {
			fido.PushAttr("BEOS:type");
			fido.PushString("text/x-partial-email");
			fido.PushOp(B_EQ);
			fido.PushOp(B_AND);
		}
		fido.Fetch();
	
		BString uid;
		while (fido.GetNextRef(&entry) == B_OK) {
			BNode(&entry).ReadAttrString("MAIL:unique_id",&uid);
			uids_on_disk->AddItem(uid.String());
		}
	}
		
	(*manifest) |= (*uids_on_disk);
	
	if (!settings->FindBool("login_and_do_nothing_else_of_any_importance")) {
		DeleteHandler *h = new DeleteHandler(this);
		runner->AddHandler(h);
		runner->PostMessage('DELE',h);
		
		trash_monitor = new TrashMonitor(this,runner->Chain()->ID());
		runner->AddHandler(trash_monitor);
		runner->PostMessage('INIT',trash_monitor);
	}
}


BMailProtocol::~BMailProtocol()
{
	if (manifest != NULL) {
		BMessage *meta_data = runner->Chain()->MetaData();
		meta_data->RemoveName("manifest");
		BString attr_name = "MAIL:";
		attr_name << runner->Chain()->ID() << ":manifest"; //--- In case someone puts multiple accounts in the same directory
		if (meta_data->HasString("path")) {
			BNode node(meta_data->FindString("path"));
			if (node.InitCheck() >= B_OK) {
				node.RemoveAttr(attr_name.String());
				ssize_t manifestsize = manifest->FlattenedSize();
				void *flatmanifest = malloc(manifestsize);
				manifest->Flatten(flatmanifest,manifestsize);
				if (status_t err = node.WriteAttr(attr_name.String(),manifest->TypeCode(),0,flatmanifest,manifestsize) < B_OK) {
					BString error = "Error while saving account manifest: ";
					error << strerror(err);
					runner->ShowError(error.String());
				}
				free(flatmanifest);
			} else runner->ShowError("Error while saving account manifest: cannot use destination directory.");
		} else runner->ShowError("Error while saving account manifest: no destination directory exists.");
	}
	delete unique_ids;
	delete manifest;
	delete trash_monitor;
	delete uids_on_disk;
}


#define dump_stringlist(a) printf("BStringList %s:\n",#a); \
							for (int32 i = 0; i < (a)->CountItems(); i++)\
								puts((a)->ItemAt(i)); \
							puts("Done\n");
							
status_t
BMailProtocol::ProcessMailMessage(BPositionIO **io_message, BEntry *io_entry,
	BMessage *io_headers, BPath *io_folder, const char *io_uid)
{
	status_t error;

	if (io_uid == NULL)
		return B_ERROR;
	
	error = GetMessage(io_uid, io_message, io_headers, io_folder);
	if (error < B_OK) {
		if (error != B_MAIL_END_FETCH) {
			MDR_DIALECT_CHOICE (
				error_alert("getting a message",error);,
				error_alert("新しいメッセージヲ取得中にエラーが発生しました",error);
			);
		}
		return B_MAIL_END_FETCH;
	}

	runner->RegisterMessageCallback(new ManifestAdder(manifest, &uids_on_disk, io_uid));
	runner->RegisterMessageCallback(new MessageDeletion(this, io_uid, io_entry, !settings->FindBool("leave_mail_on_server")));

	return B_OK;
}

void BMailProtocol::CheckForDeletedMessages() {
	{
		//---Delete things from the manifest no longer on the server
		BStringList temp;
		manifest->NotThere(*unique_ids, &temp);
		(*manifest) -= temp;
	}

	if (((settings->FindBool("delete_remote_when_local")) || !(settings->FindBool("leave_mail_on_server"))) && (manifest->CountItems() > 0)) {
		BStringList to_delete;
		
		if (uids_on_disk == NULL) {
			BStringList query_contents;
			BVolumeRoster volumes;
			BVolume volume;
			
			while (volumes.GetNextVolume(&volume) == B_OK) {
				BQuery fido;
				entry_ref entry;
		
				fido.SetVolume(&volume);
				fido.PushAttr("MAIL:chain");
				fido.PushInt32(settings->FindInt32("chain"));
				fido.PushOp(B_EQ);
				fido.Fetch();
		
				BString uid;
				while (fido.GetNextRef(&entry) == B_OK) {
					BNode(&entry).ReadAttrString("MAIL:unique_id",&uid);
					query_contents.AddItem(uid.String());
				}
			}
			
			query_contents.NotHere(*manifest,&to_delete);
		} else {
			uids_on_disk->NotHere(*manifest,&to_delete);
			delete uids_on_disk;
			uids_on_disk = NULL;
		}			

		for (int32 i = 0; i < to_delete.CountItems(); i++)
			DeleteMessage(to_delete[i]);
		
		//*(unique_ids) -= to_delete; --- This line causes bad things to
		// happen (POP3 client uses the wrong indices to retrieve
		// messages).  Without it, bad things don't happen.
		*(manifest) -= to_delete;
	}
}

void BMailProtocol::_ReservedProtocol1() {}
void BMailProtocol::_ReservedProtocol2() {}
void BMailProtocol::_ReservedProtocol3() {}
void BMailProtocol::_ReservedProtocol4() {}
void BMailProtocol::_ReservedProtocol5() {}


//	#pragma mark -


MessageDeletion::MessageDeletion(BMailProtocol *home, const char *uid,
	BEntry *io_entry, bool delete_anyway)
	:
	us(home),
	always(delete_anyway),
	message_id(uid), entry(io_entry)
{
}


void
MessageDeletion::Callback(status_t result)
{
	#if DEBUG
	 printf("Deleting %s\n", message_id);
	#endif
	BNode node(entry);
	BNodeInfo info(&node);
	char type[255];
	info.GetType(type);
	if ((always && strcmp(B_MAIL_TYPE,type) == 0) || result == B_MAIL_DISCARD)
		us->DeleteMessage(message_id);
}

}
