#include <NodeMonitor.h>
#include <Handler.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <File.h>

#include <map>

class _EXPORT BRemoteMailStorageProtocol;

#include <RemoteStorageProtocol.h>
#include <ChainRunner.h>
#include <E-mail.h>

namespace {

void GetSubFolders(BDirectory *of, BStringList *folders, const char *prepend = "");

class UpdateHandler : public BHandler {
	public:
		UpdateHandler(BRemoteMailStorageProtocol *prot, const char *dest) : _prot(prot), _dest(dest) {
			node_ref ref;
			_dest.GetNodeRef(&ref);
			dest_node = ref.node;
		}
		virtual ~UpdateHandler() {
			stop_watching(this);
		}
		void MessageReceived(BMessage *msg) {
			switch (msg->what) {
				case 'INIT': {
					if (_prot->InitCheck() < B_OK)
						return;
					
					((BMailChainRunner *)(Looper()))->ReportProgress(0,0,"Synchronizing Mailboxes");
					
					BStringList subdirs;
					GetSubFolders(&_dest,&subdirs);
					BStringList to_delete;
					BStringList to_add;
					subdirs.NotThere(_prot->mailboxes,&to_add);
					if (subdirs.CountItems() != 0) // --- If it's a virgin mailfolder, the user probably just configured his machineand probably *doesn't* want all his mail folders deleted :)
						subdirs.NotHere(_prot->mailboxes,&to_delete);
					for (int32 i = 0; i < to_add.CountItems(); i++) {
						if (_prot->CreateMailbox(to_add[i]) != B_OK)
							continue;
						_prot->mailboxes += to_add[i];
						_prot->SyncMailbox(to_add[i]);
					}
					
					_prot->CheckForDeletedMessages(); //--- Refresh the manifest list, delete messages in locally deleted folders
					
					for (int32 i = 0; i < to_delete.CountItems(); i++) {
						if (to_delete[i][0] == 0)
							continue;
						if (_prot->DeleteMailbox(to_delete[i]) == B_OK)
							_prot->mailboxes -= to_delete[i];
					}
					
					entry_ref ref;
					BEntry entry;
					_dest.GetEntry(&entry);
					entry.GetRef(&ref);
					BPath path(&ref), work_path(path);
					BNode node(&ref);
					node_ref watcher;
					node.GetNodeRef(&watcher);
					watch_node(&watcher,B_WATCH_DIRECTORY,this);
					
					for (int32 i = 0; i < _prot->mailboxes.CountItems(); i++) {
						
						work_path = path;
						work_path.Append(_prot->mailboxes[i]);
						node.SetTo(work_path.Path());
						node.GetNodeRef(&watcher);
						nodes[watcher.node] = strdup(_prot->mailboxes[i]);
						if (_prot->mailboxes[i][0] == 0)
							continue; //--- We've covered this in the parent monitor
						watch_node(&watcher,B_WATCH_DIRECTORY,this);
						_prot->SyncMailbox(_prot->mailboxes[i]);
					}
					((BMailChainRunner *)(Looper()))->ResetProgress();
				} break;
			
			case B_NODE_MONITOR: {
				int32 opcode;
				if (msg->FindInt32("opcode",&opcode) < B_OK)
					break;
				
				int64 directory, node(msg->FindInt64("node"));
				dev_t device(msg->FindInt32("device"));
				
			
				bool is_dir;
				{
					node_ref item_ref;
					item_ref.node = node;
					item_ref.device = device;
					BDirectory dir(&item_ref);
					is_dir = (dir.InitCheck() == B_OK);
				}
				
				if (opcode == B_ENTRY_MOVED) {
					ino_t from, to;
					msg->FindInt64("from directory",&from);
					msg->FindInt64("to directory",&to);
					const char *from_mb(nodes[from]), *to_mb(nodes[to]);
					if (to == dest_node)
						to_mb = "";
					if (from == dest_node)
						from_mb = "";
					if (from_mb == NULL) {
						msg->AddInt64("directory",to);
						opcode = B_ENTRY_CREATED;
					} else if (to_mb == NULL) {
						msg->AddInt64("directory",from);
						if (is_dir)
							opcode = B_ENTRY_REMOVED;
					} else {
						if (!is_dir) {
							{
								node_ref item_ref;
								item_ref.node = to;
								item_ref.device = device;
								BDirectory dir(&item_ref);
								BNode node(&dir,msg->FindString("name"));
								//--- Why in HELL can't you make a BNode from a node_ref?
								
								if (node.InitCheck() != B_OK)
									break; //-- We're late, it's already gone elsewhere. Ignore for now.
								
								BString id;
								node.ReadAttrString("MAIL:unique_id",&id);
								id.Truncate(id.FindLast('/'));
								if (id == to_mb)
									break; //-- Already where it belongs, no need to do anything
							}
								
							snooze(uint64(5e5));
							_prot->SyncMailbox(to_mb);
								
							//node.WriteAttrString("MAIL:unique_id",&id);
							
							_prot->CheckForDeletedMessages();
						} else {
							BString mb;
							if (to_mb[0] == 0)
								mb = msg->FindString("name");
							else {
								mb = to_mb;
								mb << '/' << msg->FindString("name");
							}
							if (strcmp(mb.String(),nodes[node]) == 0)
								break;
							if (_prot->CreateMailbox(mb.String()) < B_OK)
								break;
							_prot->mailboxes += mb.String();
							_prot->SyncMailbox(mb.String());
							_prot->CheckForDeletedMessages();
							_prot->DeleteMailbox(nodes[node]);
							_prot->mailboxes -= nodes[node];
							free((void *)nodes[node]);
							nodes[node] = strdup(mb.String());
						}
						break;
					}
				}						
				
				msg->FindInt64("directory",&directory);
				switch (opcode) {
					case B_ENTRY_CREATED:
						if (!is_dir) {
							const char *dir = nodes[directory];
							snooze(500000);
							
							if (dir == NULL)
								dir = "";
								
							if (!_prot->mailboxes.HasItem(dir))
								break;
							
							{
								node_ref item_ref;
								item_ref.node = directory;
								item_ref.device = device;
								BDirectory dir(&item_ref);
								BNode node(&dir,msg->FindString("name"));
								//--- Why in HELL can't you make a BNode from a node_ref?
								
								if (node.InitCheck() != B_OK)
									break; //-- We're late, it's already gone elsewhere. Ignore for now.
							}
							
							_prot->SyncMailbox(nodes[directory]);
						} else {
							BString mb;
							if (directory == dest_node)
								mb = msg->FindString("name");
							else {
								mb = nodes[directory];
								mb << '/' << msg->FindString("name");
							}
							if (_prot->CreateMailbox(mb.String()) < B_OK)
								break;
							nodes[node] = strdup(mb.String());
							_prot->mailboxes += mb.String();
							_prot->SyncMailbox(mb.String());
							node_ref ref;
							ref.device = device;
							ref.node = node;
							watch_node(&ref,B_WATCH_DIRECTORY,this);
						}
						break;
					case B_ENTRY_REMOVED:
						_prot->CheckForDeletedMessages();
						if ((is_dir) && (nodes[node] != NULL)) {
							_prot->DeleteMailbox(nodes[node]);
							_prot->mailboxes -= nodes[node];
							free((void *)nodes[node]);
							nodes[node] = NULL;
							node_ref ref;
							ref.device = device;
							ref.node = node;
							watch_node(&ref,B_STOP_WATCHING,this);
						}
						break;
				}
				
				} break;
			}
		}
						
						

	private:
		BRemoteMailStorageProtocol *_prot;
		BDirectory _dest;
		
		map<int64, const char *> nodes;
		ino_t dest_node;
};

void GetSubFolders(BDirectory *of, BStringList *folders, const char *prepend) {
	of->Rewind();
	BEntry ent;
	BString crud;
	BDirectory sub;
	char buf[255];
	while (of->GetNextEntry(&ent) == B_OK) {
		if (ent.IsDirectory()) {
			sub.SetTo(&ent);
			ent.GetName(buf);
			crud = prepend;
			crud << buf << '/';
			GetSubFolders(&sub,folders,crud.String());
			crud = prepend;
			crud << buf;
			(*folders) += crud.String();
		}
	}
}

BRemoteMailStorageProtocol::BRemoteMailStorageProtocol(BMessage *settings, BMailChainRunner *runner) : BMailProtocol(settings,runner) {
	handler = new UpdateHandler(this,runner->Chain()->MetaData()->FindString("path"));
	runner->AddHandler(handler);
	runner->PostMessage('INIT',handler);
}

BRemoteMailStorageProtocol::~BRemoteMailStorageProtocol() {
	delete handler;
}
}

//----BMailProtocol stuff
status_t BRemoteMailStorageProtocol::GetMessage(
	const char* uid,
	BPositionIO** out_file, BMessage* out_headers,
	BPath* out_folder_location) {
		BString folder(uid), id;
		{
			BString raw(uid);
			folder.Truncate(raw.FindLast('/'));
			raw.CopyInto(id,raw.FindLast('/') + 1,raw.Length());
		}
		
		*out_folder_location = folder.String();
		return GetMessage(folder.String(),id.String(),out_file,out_headers);
	}
	
status_t BRemoteMailStorageProtocol::DeleteMessage(const char* uid) {
	BString folder(uid), id;
	{
		BString raw(uid);
		int32 j = raw.FindLast('/');
		folder.Truncate(j);
		raw.CopyInto(id,j + 1,raw.Length());
	}
	
	status_t err;
	if ((err = DeleteMessage(folder.String(),id.String())) < B_OK)
		return err;
		
	(*unique_ids) -= uid;
	return B_OK;
}

void BRemoteMailStorageProtocol::SyncMailbox(const char *mailbox) {
	BPath path(runner->Chain()->MetaData()->FindString("path"));
	path.Append(mailbox);
	
	BDirectory folder(path.Path());
	
	BEntry entry;
	BFile snoodle;
	BString string;
	uint32 chain;
	bool append;
	
	if (!mailboxes.HasItem(mailbox))
		return; // -- Basic Sanity Checking
	
	while (folder.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsFile())
			continue;
		while (snoodle.SetTo(&entry,B_READ_WRITE) == B_BUSY) snooze(100);
		append = false;
		
		while (snoodle.Lock() != B_OK) snooze(100);
		snoodle.Unlock();
		
		if (snoodle.ReadAttr("MAIL:chain",B_INT32_TYPE,0,&chain,sizeof(chain)) < B_OK)
			append = true;
		if (chain != runner->Chain()->ID()) {
			int32 pending_chain(-1), flags(0);
			snoodle.ReadAttr("MAIL:pending_chain",B_INT32_TYPE,0,&chain,sizeof(chain));
			snoodle.ReadAttr("MAIL:flags",B_INT32_TYPE,0,&flags,sizeof(flags));
		
			if ((pending_chain == runner->Chain()->ID()) && (BMailChain(chain).ChainDirection() == outbound) && (flags & B_MAIL_PENDING))
				continue; //--- Ignore this message, recode the chain attribute at the next SyncMailbox()
				
			if (pending_chain == runner->Chain()->ID()) {
				chain = runner->Chain()->ID();
				snoodle.WriteAttr("MAIL:chain",B_INT32_TYPE,0,&chain,sizeof(chain));
				append = false;
			} else
				append = true;
		}
		if (snoodle.ReadAttrString("MAIL:unique_id",&string) < B_OK)
			append = true;

		BString folder(string), id("");
		int32 j = string.FindLast('/');
		if ((!append) && (j >= 0)) {
			folder.Truncate(j);
			string.CopyInto(id,j + 1,string.Length());
			if (folder == mailbox)
				continue;
		} else {
			append = true;
		}
		
		if (append)
			AddMessage(mailbox,&snoodle,&id); //---We should check for partial messages here
		else
			CopyMessage(folder.String(),mailbox,&id);
			
		string = mailbox;
		string << '/' << id;
		/*snoodle.RemoveAttr("MAIL:unique_id");
		snoodle.RemoveAttr("MAIL:chain");*/
		chain = runner->Chain()->ID();
	
		int32 flags = 0;
		snoodle.ReadAttr("MAIL:flags",B_INT32_TYPE,0,&flags,sizeof(flags));
		
		if (flags & B_MAIL_PENDING)
			snoodle.WriteAttr("MAIL:pending_chain",B_INT32_TYPE,0,&chain,sizeof(chain));
		else
			snoodle.WriteAttr("MAIL:chain",B_INT32_TYPE,0,&chain,sizeof(chain));
		snoodle.WriteAttrString("MAIL:unique_id",&string);
		(*manifest) += string.String();
		(*unique_ids) += string.String();
		string = runner->Chain()->Name();
		snoodle.WriteAttrString("MAIL:account",&string);
	}
}

/*status_t BRemoteMailStorageProtocol::MoveMessage(const char *mailbox, const char *to_mailbox, BString *message) {
	BString new_id(*message);
	status_t err;
	if ((err = CopyMessage(mailbox,to_mailbox,&new_id)) < B_OK)
		return err;
	if ((err = DeleteMessage(mailbox,message->String())) < B_OK)
		return err;
	*message = new_id;
	return B_OK;
}*/
