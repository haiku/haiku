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

using std::map;

namespace {

void GetSubFolders(BDirectory *of, BStringList *folders, const char *prepend = "");

class UpdateHandler : public BHandler {
	public:
		UpdateHandler(BRemoteMailStorageProtocol *prot, const char *dest)
			: fProtocol(prot), fDestination(dest)
		{
			node_ref ref;
			fDestination.GetNodeRef(&ref);
			fDestinationNode = ref.node;
		}

		virtual ~UpdateHandler()
		{
			stop_watching(this);
		}

		void MessageReceived(BMessage *msg)
		{
			switch (msg->what) {
				case 'INIT': {
					if (fProtocol->InitCheck() < B_OK)
						return;

					((BMailChainRunner *)(Looper()))->ReportProgress(0, 0,
						"Synchronizing Mailboxes");

					BStringList subdirs;
					GetSubFolders(&fDestination, &subdirs);
					BStringList to_delete;
					BStringList to_add;
					subdirs.NotThere(fProtocol->mailboxes, &to_add);
					if (subdirs.CountItems() != 0) {
						// If it's a virgin mailfolder, the user probably just configured
						// his machine and probably *doesn't* want all his mail folders deleted :)
						subdirs.NotHere(fProtocol->mailboxes, &to_delete);
					}
					for (int32 i = 0; i < to_add.CountItems(); i++) {
						if (fProtocol->CreateMailbox(to_add[i]) != B_OK)
							continue;

						fProtocol->mailboxes += to_add[i];
						fProtocol->SyncMailbox(to_add[i]);
					}
					
					fProtocol->CheckForDeletedMessages();
						// Refresh the manifest list, delete messages in locally deleted folders

					for (int32 i = 0; i < to_delete.CountItems(); i++) {
						if (to_delete[i][0] == 0)
							continue;
						if (fProtocol->DeleteMailbox(to_delete[i]) == B_OK)
							fProtocol->mailboxes -= to_delete[i];
					}

					entry_ref ref;
					BEntry entry;
					fDestination.GetEntry(&entry);
					entry.GetRef(&ref);
					BPath path(&ref), work_path(path);
					BNode node(&ref);
					node_ref watcher;
					node.GetNodeRef(&watcher);
					watch_node(&watcher,B_WATCH_DIRECTORY,this);

					for (int32 i = 0; i < fProtocol->mailboxes.CountItems(); i++) {
						work_path = path;
						work_path.Append(fProtocol->mailboxes[i]);
						node.SetTo(work_path.Path());
						node.GetNodeRef(&watcher);
						fNodes[watcher.node] = strdup(fProtocol->mailboxes[i]);
						if (fProtocol->mailboxes[i][0] == 0) {
							// We've covered this in the parent monitor
							continue;
						}
						watch_node(&watcher, B_WATCH_DIRECTORY, this);
						fProtocol->SyncMailbox(fProtocol->mailboxes[i]);
					}
					((BMailChainRunner *)(Looper()))->ResetProgress();
					break;
				}

				case B_NODE_MONITOR: {
					int32 opcode;
					if (msg->FindInt32("opcode", &opcode) < B_OK)
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
						msg->FindInt64("from directory", &from);
						msg->FindInt64("to directory", &to);
						const char *from_mb(fNodes[from]), *to_mb(fNodes[to]);
						if (to == fDestinationNode)
							to_mb = "";
						if (from == fDestinationNode)
							from_mb = "";
						if (from_mb == NULL) {
							msg->AddInt64("directory", to);
							opcode = B_ENTRY_CREATED;
						} else if (to_mb == NULL) {
							msg->AddInt64("directory", from);
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
										// Why in HELL can't you make a BNode from a node_ref?

									if (node.InitCheck() != B_OK) {
										// We're late, it's already gone elsewhere. Ignore for now.
										break;
									}

									BString id;
									node.ReadAttrString("MAIL:unique_id", &id);
									id.Truncate(id.FindLast('/'));
									if (id == to_mb) {
										// Already where it belongs, no need to do anything
										break;
									}
								}

								snooze(uint64(5e5));
								fProtocol->SyncMailbox(to_mb);

								//node.WriteAttrString("MAIL:unique_id",&id);

								fProtocol->CheckForDeletedMessages();
							} else {
								BString mb;
								if (to_mb[0] == 0)
									mb = msg->FindString("name");
								else {
									mb = to_mb;
									mb << '/' << msg->FindString("name");
								}
								if (strcmp(mb.String(), fNodes[node]) == 0)
									break;
								if (fProtocol->CreateMailbox(mb.String()) < B_OK)
									break;
								fProtocol->mailboxes += mb.String();
								fProtocol->SyncMailbox(mb.String());
								fProtocol->CheckForDeletedMessages();
								fProtocol->DeleteMailbox(fNodes[node]);
								fProtocol->mailboxes -= fNodes[node];
								free((void *)fNodes[node]);
								fNodes[node] = strdup(mb.String());
							}
							break;
						}
					}						

					msg->FindInt64("directory", &directory);
					switch (opcode) {
						case B_ENTRY_CREATED:
							if (!is_dir) {
								const char *dir = fNodes[directory];
								snooze(500000);
									// half a second

								if (dir == NULL)
									dir = "";

								if (!fProtocol->mailboxes.HasItem(dir))
									break;

								{
									node_ref nodeRef;
									nodeRef.node = directory;
									nodeRef.device = device;
									BDirectory dir(&nodeRef);
	
									BNode node(&dir, msg->FindString("name"));
									if (node.InitCheck() != B_OK)
										break; //-- We're late, it's already gone elsewhere. Ignore for now.
								}

								fProtocol->SyncMailbox(fNodes[directory]);
							} else {
								BString mb;
								if (directory == fDestinationNode)
									mb = msg->FindString("name");
								else {
									mb = fNodes[directory];
									mb << '/' << msg->FindString("name");
								}
								if (fProtocol->CreateMailbox(mb.String()) < B_OK)
									break;

								fNodes[node] = strdup(mb.String());
								fProtocol->mailboxes += mb.String();
								fProtocol->SyncMailbox(mb.String());
								node_ref ref;
								ref.device = device;
								ref.node = node;
								watch_node(&ref, B_WATCH_DIRECTORY, this);
							}
							break;
						case B_ENTRY_REMOVED:
							fProtocol->CheckForDeletedMessages();
							if ((is_dir) && (fNodes[node] != NULL)) {
								fProtocol->DeleteMailbox(fNodes[node]);
								fProtocol->mailboxes -= fNodes[node];
								free((void *)fNodes[node]);
								fNodes[node] = NULL;
								node_ref ref;
								ref.device = device;
								ref.node = node;
								watch_node(&ref, B_STOP_WATCHING, this);
							}
							break;
					}
				}
				break;
			}
		}

	private:
		BRemoteMailStorageProtocol *fProtocol;
		BDirectory fDestination;

		map<int64, const char *> fNodes;
		ino_t fDestinationNode;
};


void
GetSubFolders(BDirectory *of, BStringList *folders, const char *prepend)
{
	of->Rewind();

	BEntry entry;
	while (of->GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;

		BDirectory subDirectory(&entry);

		char buffer[B_FILE_NAME_LENGTH];
		entry.GetName(buffer);

		BString path = prepend;
		path << buffer << '/';
		GetSubFolders(&subDirectory, folders, path.String());
		path = prepend;
		path << buffer;
		*folders += path.String();
	}
}

} // unnamed namspace


BRemoteMailStorageProtocol::BRemoteMailStorageProtocol(BMessage *settings,
	BMailChainRunner *runner)
	: BMailProtocol(settings, runner)
{
	handler = new UpdateHandler(this, runner->Chain()->MetaData()->FindString("path"));
	runner->AddHandler(handler);
	runner->PostMessage('INIT', handler);
}


BRemoteMailStorageProtocol::~BRemoteMailStorageProtocol()
{
	delete handler;
}


//----BMailProtocol stuff


status_t
BRemoteMailStorageProtocol::GetMessage(const char *uid,
	BPositionIO **outFile, BMessage *outHeaders,
	BPath *outFolderLocation)
{
	BString folder(uid), id;
	{
		BString raw(uid);
		folder.Truncate(raw.FindLast('/'));
		raw.CopyInto(id, raw.FindLast('/') + 1, raw.Length());
	}

	*outFolderLocation = folder.String();
	return GetMessage(folder.String(), id.String(), outFile, outHeaders);
}
	

status_t
BRemoteMailStorageProtocol::DeleteMessage(const char *uid)
{
	BString folder(uid), id;
	{
		BString raw(uid);
		int32 j = raw.FindLast('/');
		folder.Truncate(j);
		raw.CopyInto(id, j + 1, raw.Length());
	}

	status_t err;
	if ((err = DeleteMessage(folder.String(), id.String())) < B_OK)
		return err;

	*unique_ids -= uid;
	return B_OK;
}


void
BRemoteMailStorageProtocol::SyncMailbox(const char *mailbox)
{
	BPath path(runner->Chain()->MetaData()->FindString("path"));
	path.Append(mailbox);

	BDirectory folder(path.Path());

	BEntry entry;
	BString string;
	uint32 chain;
	bool append;
	
	if (!mailboxes.HasItem(mailbox))
		return; // -- Basic Sanity Checking
	
	while (folder.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsFile())
			continue;

		BFile file;
		while (file.SetTo(&entry, B_READ_WRITE) == B_BUSY)
			snooze(100);
		append = false;

		while (file.Lock() != B_OK)
			snooze(100);
		file.Unlock();

		if (file.ReadAttr("MAIL:chain", B_INT32_TYPE, 0, &chain, sizeof(chain)) < B_OK)
			append = true;

		if (chain != runner->Chain()->ID()) {
			uint32 pendingChain(~0UL), flags(0);
			file.ReadAttr("MAIL:pending_chain", B_INT32_TYPE, 0, &pendingChain, sizeof(chain));
			file.ReadAttr("MAIL:flags", B_INT32_TYPE, 0, &flags, sizeof(flags));

			if (pendingChain == runner->Chain()->ID()
				&& BMailChain(chain).ChainDirection() == outbound
				&& (flags & B_MAIL_PENDING) != 0) {
				// Ignore this message, recode the chain attribute at the next SyncMailbox()
				continue;
			}

			if (pendingChain == runner->Chain()->ID()) {
				chain = runner->Chain()->ID();
				file.WriteAttr("MAIL:chain", B_INT32_TYPE, 0, &chain, sizeof(chain));
				append = false;
			} else
				append = true;
		}
		if (file.ReadAttrString("MAIL:unique_id", &string) < B_OK)
			append = true;

		BString folder(string), id("");
		int32 j = string.FindLast('/');
		if (!append && j >= 0) {
			folder.Truncate(j);
			string.CopyInto(id, j + 1, string.Length());
			if (folder == mailbox)
				continue;
		} else
			append = true;

		if (append) {
			// ToDo: We should check for partial messages here
			AddMessage(mailbox, &file, &id);
		} else
			CopyMessage(folder.String(), mailbox, &id);

		string = mailbox;
		string << '/' << id;
		/*file.RemoveAttr("MAIL:unique_id");
		file.RemoveAttr("MAIL:chain");*/
		chain = runner->Chain()->ID();

		int32 flags = 0;
		file.ReadAttr("MAIL:flags", B_INT32_TYPE, 0, &flags, sizeof(flags));

		if (flags & B_MAIL_PENDING)
			file.WriteAttr("MAIL:pending_chain", B_INT32_TYPE, 0, &chain, sizeof(chain));
		else
			file.WriteAttr("MAIL:chain", B_INT32_TYPE, 0, &chain, sizeof(chain));
		file.WriteAttrString("MAIL:unique_id", &string);
		*manifest += string.String();
		*unique_ids += string.String();
		string = runner->Chain()->Name();
		file.WriteAttrString("MAIL:account", &string);
	}
}

