#include <RemoteStorageProtocol.h>
#include <E-mail.h>
#include <netdb.h>
#include <errno.h>
#include <Message.h>
#include <ChainRunner.h>
#include <MDRLanguage.h>
#include <Debug.h>
#include <StringList.h>
#include <Entry.h>
#include <Directory.h>
#include <File.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <Path.h>
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#ifdef BONE
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
	#include <sys/select.h>
#else
	#include <socket.h>
#endif

#ifdef USESSL
	#include <openssl/ssl.h>
	#include <openssl/rand.h>
#endif

#include "NestedString.h"

#define CRLF "\r\n"
#define xEOF    236
const bigtime_t kIMAP4ClientTimeout = 1000000*60; // 60 sec

enum { OK,BAD,NO,CONTINUE, NOT_COMMAND_RESPONSE };

struct mailbox_info {
	int32 exists;
	int32 next_uid;
	BString server_mb_name;
};

class IMAP4Client : public BMailRemoteStorageProtocol {
	public:
		IMAP4Client(BMessage *settings, BMailChainRunner *run);
		virtual ~IMAP4Client();
		
		virtual status_t GetMessage(const char *mailbox, const char *message, BPositionIO **, BMessage *headers);
		virtual status_t AddMessage(const char *mailbox, BPositionIO *data, BString *id);
		
		virtual status_t DeleteMessage(const char *mailbox, const char *message);
		virtual status_t CopyMessage(const char *mailbox, const char *to_mailbox, BString *message);
		
		virtual status_t CreateMailbox(const char *mailbox);
		virtual status_t DeleteMailbox(const char *mailbox);
		
		void GetUniqueIDs();
		
		status_t ReceiveLine(BString &out);
		status_t SendCommand(const char *command);
		
		status_t Select(const char *mb, bool force_reselect = false, bool queue_new_messages = true, bool noop = true, bool no_command = false, bool ignore_forced_reselect = false);
		status_t Close();
		
		virtual status_t InitCheck(BString *) { if (net < 0 && err == B_OK) return net; return err; }
		
		int GetResponse(BString &tag, NestedString *parsed_response, bool report_literals = false, bool recursion_flag = false);
		bool WasCommandOkay(BString &response);
		
		void InitializeMailboxes();
	
	private:
		friend class NoopWorker;
		friend class IMAP4PartialReader;
		
		NoopWorker *noop;
		BMessageRunner *nooprunner;
		
		int32 commandCount;
		int net;
		BString selected_mb, inbox_name, hierarchy_delimiter, mb_root;
		BList box_info;
		status_t err;
		
		#ifdef USESSL
			SSL_CTX *ctx;
			SSL *ssl;
			BIO *sbio;
			
			bool use_ssl;
		#endif
		
		bool force_reselect;
};

class NoopWorker : public BHandler {
	public:
		NoopWorker(IMAP4Client *a) : us(a), last_run(0) {}
		void MessageReceived(BMessage *msg) {
			if (msg->what != 'impn' /* IMaP Noop */)
				return;
				
			if ((time(NULL) - last_run) < 9)
				return;
							
			us->Select(us->inbox_name.String());
			last_run = time(NULL);
		}
	private:
		IMAP4Client *us;
		time_t last_run;
};

IMAP4Client::IMAP4Client(BMessage *settings, BMailChainRunner *run) : BMailRemoteStorageProtocol(settings,run), noop(NULL), commandCount(0), net(-1), selected_mb(""), force_reselect(false) {
	err = B_OK;
	
	mb_root = settings->FindString("root");
	#ifdef USESSL
		use_ssl = (settings->FindInt32("flavor") == 1);
		ssl = NULL;
		ctx = NULL;
	#endif
	
	int port = settings->FindInt32("port");
	
	if (port <= 0)
		#ifdef USESSL
			port = use_ssl ? 993 : 143;
		#else
			port = 143;
		#endif

//-----Open TCP link	
	runner->ReportProgress(0,0,MDR_DIALECT_CHOICE ("Opening connection...","接続中..."));
	
	uint32 hostIP = inet_addr(settings->FindString("server"));  // first see if we can parse it as a numeric address
	if ((hostIP == 0)||(hostIP == (uint32)-1)) {
		struct hostent * he = gethostbyname(settings->FindString("server"));
		hostIP = he ? *((uint32*)he->h_addr) : 0;
	}
   
	if (hostIP == 0) {
		BString error;
		error << "Could not connect to IMAP server " << settings->FindString("server");
		if ((port != 143) && (port != 993))
			error << ":" << port;
		error << ": Host not found.";
		runner->ShowError(error.String());
		net = -1;
		runner->Stop();
		return;
	}
	
	net = socket(AF_INET, SOCK_STREAM, 0);
	if (net >= 0) {
		struct sockaddr_in saAddr;
		memset(&saAddr, 0, sizeof(saAddr));
		saAddr.sin_family      = AF_INET;
		saAddr.sin_port        = htons(port);
		saAddr.sin_addr.s_addr = hostIP;
		int result = connect(net, (struct sockaddr *) &saAddr, sizeof(saAddr));
		if (result < 0) {
#ifdef BONE
			close(net);
#else
			closesocket(net);
#endif
			net = -1;
			BString error;
			error << "Could not connect to IMAP server " << settings->FindString("server");
			if ((port != 143) && (port != 993))
				error << ":" << port;
			error << '.';
			runner->ShowError(error.String());
			runner->Stop();
			return;
		}
	} else {
		BString error;
		error << "Could not connect to IMAP server " << settings->FindString("server");
		if ((port != 143) && (port != 993))
			error << ":" << port;
		error << ". (" << strerror(errno) << ')';
		runner->ShowError(error.String());
		net = -1;
		runner->Stop();
		return;
	}
	
#ifdef USESSL
	if (use_ssl) {
		SSL_library_init();
    	SSL_load_error_strings();
    	RAND_seed(this,sizeof(IMAP4Client));
    	/*--- Because we're an add-on loaded at an unpredictable time, all
    	      the memory addresses and things contained in ourself are
    	      esssentially random. */
    	
    	ctx = SSL_CTX_new(SSLv23_method());
    	ssl = SSL_new(ctx);
    	sbio=BIO_new_socket(net,BIO_NOCLOSE);
    	SSL_set_bio(ssl,sbio,sbio);
    	
    	if (SSL_connect(ssl) <= 0) {
    		BString error;
			error << "Could not connect to IMAP server " << settings->FindString("server");
			if (port != 993)
				error << ":" << port;
			error << ". (SSL Connection Error)";
			runner->ShowError(error.String());
			SSL_CTX_free(ctx);
			#ifdef BONE
				close(net);
			#else
				closesocket(net);
			#endif
			runner->Stop();
			return;
		}
	}
	
	#endif
	
//-----Wait for welcome message
	BString response;
	ReceiveLine(response);

//-----Log in
	runner->ReportProgress(0,0,MDR_DIALECT_CHOICE ("Authenticating...","認証中..."));
	
	const char *password = settings->FindString("password");
	{
		char *passwd = get_passwd(settings,"cpasswd");
		if (passwd)
			password = passwd;
	}

	BString command = "LOGIN ";
	command << "\"" << settings->FindString("username") << "\" ";
	command << "\"" << password << "\"";
	SendCommand(command.String());
	if (!WasCommandOkay(response)) {
		response.Prepend("Login failed. Please check your username and password.\n(");
		response << ')';
		runner->ShowError(response.String());
		err = B_ERROR;
		runner->Stop();
		return;
	}
	
	runner->ReportProgress(0,0,"Logged in");
	
	InitializeMailboxes();
	GetUniqueIDs();
	
	BStringList to_dl;
	unique_ids->NotThere(*manifest,&to_dl);
	
	noop = new NoopWorker(this);
	runner->AddHandler(noop);
	nooprunner = new BMessageRunner(BMessenger(noop,runner),new BMessage('impn'),10e6);
	
	if (to_dl.CountItems() > 0)
		runner->GetMessages(&to_dl,-1);
}

IMAP4Client::~IMAP4Client() {	
	if (net > 0) { 
		if (selected_mb != "")
			SendCommand("CLOSE");
		SendCommand("LOGOUT");
	}
	
	for (int32 i = 0; i < box_info.CountItems(); i++)
		delete (struct mailbox_info *)(box_info.ItemAt(i));
	
	delete noop;
	
#ifdef USESSL
	if (use_ssl) {
		if (ssl)
			SSL_shutdown(ssl);
		if (ctx)
			SSL_CTX_free(ctx);
	}
#endif
	
#ifdef BONE
	close(net);
#else
	closesocket(net);
#endif
}

void IMAP4Client::InitializeMailboxes() {
	BString command;
	command << "LSUB \"" << mb_root << "\" \"*\"";
	
	SendCommand(command.String());
	
	BString tag;
	char expected[255];
	::sprintf(expected,"a%.7ld",commandCount);
	create_directory(runner->Chain()->MetaData()->FindString("path"),0777);
	const char *path = runner->Chain()->MetaData()->FindString("path");
	int val;
	do {
		NestedString response;
		val = GetResponse(tag,&response);
		if (val != NOT_COMMAND_RESPONSE)
			break;
				
		if (tag == expected)
			break;
		
		if (response[3]()[0] != '.') {
			struct mailbox_info *info = new struct mailbox_info;
			info->exists = -1;
			info->next_uid = -1;
			info->server_mb_name = response[3]();
			box_info.AddItem(info);
			BString parsed_name = response[3]();
			if ((mb_root != "") &&  (strncmp(mb_root.String(),parsed_name.String(),mb_root.Length()) == 0))
				parsed_name.Remove(0,mb_root.Length());
			
			if (strcasecmp(response[2](),"NIL")) {
				hierarchy_delimiter = response[2]();
				if (strcmp(response[2](),"/")) {
					if (strcmp(response[2](),"\\"))
						parsed_name.ReplaceAll('/','\\');
					else
						parsed_name.ReplaceAll('/','-');
						
					parsed_name.ReplaceAll(response[2](),"/");
				}
			}
			if (parsed_name.ByteAt(0) == '/')
				parsed_name.Remove(0,1);
				
			mailboxes += parsed_name.String();
			if (strcasecmp(parsed_name.String(),"INBOX") == 0)
				inbox_name = parsed_name;
				
			BPath blorp(path);
			blorp.Append(parsed_name.String());
			create_directory(blorp.Path(),0777);
		}
	} while (1);
	
	
	if (hierarchy_delimiter == "" || hierarchy_delimiter == "NIL") {
		SendCommand("LIST \"\" \"\"");
		NestedString dem;
		GetResponse(tag,&dem);
		hierarchy_delimiter = dem[2]();
		if (hierarchy_delimiter == "" || hierarchy_delimiter == "NIL")
			hierarchy_delimiter = "/";
	}
	
	if (mb_root.ByteAt(mb_root.Length() - 1) != hierarchy_delimiter.ByteAt(0)) {	
		command = "SELECT ";
		command << mb_root;
		SendCommand(command.String());
		if (WasCommandOkay(command)) {
			struct mailbox_info *info = new struct mailbox_info;
			info->exists = -1;
			info->next_uid = -1;
			info->server_mb_name = mb_root;
			
			mailboxes += "";
			box_info.AddItem(info);
			
			if (strcasecmp(mb_root.String(),"INBOX") == 0)
				inbox_name = "";
			SendCommand("CLOSE");
		}
	}
}

#define dump_stringlist(a) printf("BStringList %s:\n",#a); \
							for (int32 i = 0; i < a->CountItems(); i++)\
								puts(a->ItemAt(i)); \
							puts("Done\n");

status_t IMAP4Client::AddMessage(const char *mailbox, BPositionIO *data, BString *id) {
	Select(mailbox); //---Update info
	
	const int32 box_index = mailboxes.IndexOf(mailbox);
	char expected[255];
	BString tag;
	
	BString command = "APPEND \"";
	off_t size;
	data->Seek(0,SEEK_END);
	size = data->Position();
	
	BString attributes = "\\Seen";
	
	{
		BNode *node = dynamic_cast<BNode *>(data);
		
		if (node != NULL) {
			BString status;
			node->ReadAttrString(B_MAIL_ATTR_STATUS,&status);
			/*if (status == "Sent")
				attributes += " \\Sent";
			if (status == "Pending")
				attributes += " \\Sent";*/
			if (status == "Replied")
				attributes += " \\Answered";
		}
	}
		
	
	command << ((struct mailbox_info *)(box_info.ItemAt(box_index)))->server_mb_name << "\" (" << attributes << ") {" << size << '}';
	SendCommand(command.String());
	status_t err = ReceiveLine(command);
	if (err < B_OK)
		return err;
		
	char *buffer = new char[size];
	data->ReadAt(0,buffer,size);
#ifdef USESSL
	if (use_ssl) {
		SSL_write(ssl,buffer,size);
		SSL_write(ssl,"\r\n",2);
	} else 
#endif
	{
		send(net,buffer,size,0);
		send(net,"\r\n",2,0);
	}
	Select(mailbox,false,false,false,true);
	
	if (((struct mailbox_info *)(box_info.ItemAt(box_index)))->next_uid <= 0) {
		command = "FETCH ";
		command << ((struct mailbox_info *)(box_info.ItemAt(box_index)))->exists << " UID";
		
		SendCommand(command.String());
		::sprintf(expected,"a%.7ld",commandCount);
		*id = "";
		while (1) {
			NestedString response;
			GetResponse(tag,&response);
						
			if (tag == expected)
				break;
			
			*id = response[2][1]();
		}
	} else {
		*id = "";
		*id << (((struct mailbox_info *)(box_info.ItemAt(box_index)))->next_uid - 1);
	}
	
	return B_OK;
}

status_t IMAP4Client::DeleteMessage(const char *mailbox, const char *message) {
	BString command = "UID STORE ";
	command << message << " +FLAGS.SILENT (\\Deleted)";
	
	if (Select(mailbox,false,true,true,false,true) < B_OK)
		return B_ERROR;
	
	SendCommand(command.String());
	if (!WasCommandOkay(command)) {
		command.Prepend("Error while deleting message: ");
		runner->ShowError(command.String());
		return B_ERROR;
	}
	
	force_reselect = true;
	
	return B_OK;
}

status_t IMAP4Client::CopyMessage(const char *mailbox, const char *to_mailbox, BString *message) {
	struct mailbox_info *to_mb = (struct mailbox_info *)(box_info.ItemAt(mailboxes.IndexOf(to_mailbox)));
	char expected[255];
	BString tag;
	
	Select(mailbox);
	
	BString command = "UID COPY ";
	command << *message << " \"" << to_mb->server_mb_name << '\"';
	SendCommand(command.String());
	if (!WasCommandOkay(command))
		return B_ERROR;
	
	Select(to_mailbox,false,false,true); //---Update mailbox info
	
	if (to_mb->next_uid <= 0) {
		command = "FETCH ";
		command << to_mb->exists << " UID";
		
		SendCommand(command.String());
		::sprintf(expected,"a%.7ld",commandCount);
		*message = "";
		while (1) {
			NestedString response;
			GetResponse(tag,&response);
						
			if (tag == expected)
				break;
			
			*message = response[2][1]();
		}
	} else {
		*message = "";
		*message << (to_mb->next_uid - 1);
	}
	
	return B_OK;
}

status_t IMAP4Client::CreateMailbox(const char *mailbox) {
	Close();
	
	struct mailbox_info *info = new struct mailbox_info;
	info->exists = -1;
	info->next_uid = -1;
	info->server_mb_name = mailbox;
	info->server_mb_name.ReplaceAll("/",hierarchy_delimiter.String());
	if ((mb_root.ByteAt(mb_root.Length() - 1) != hierarchy_delimiter.ByteAt(0)) && (mb_root.Length() > 0))
		info->server_mb_name.Prepend(hierarchy_delimiter);
		
	info->server_mb_name.Prepend(mb_root.String());
	
	BString command;
	command << "CREATE \"" << info->server_mb_name << '\"';
	SendCommand(command.String());
	BString response;
	WasCommandOkay(response);
	//--- Deliberately ignore errors in the case of extant, but unsubscribed, mailboxes
	
	command = "SUBSCRIBE \"";
	command << info->server_mb_name << '\"';
	SendCommand(command.String());
	
	if (!WasCommandOkay(response)) {
		command = "Error creating mailbox ";
		command << mailbox << ". The server said: \n" << response << "\nThis may mean you can't create a new mailbox in this location.";
		runner->ShowError(command.String());
		delete info;
		return B_ERROR;
	}
	
	box_info.AddItem(info);
	
	return B_OK;
}

status_t IMAP4Client::DeleteMailbox(const char *mailbox) {
	Close();
	
	if (!mailboxes.HasItem(mailbox))
		return B_ERROR;
	
	BString command;
	
	command = "UNSUBSCRIBE \"";
	command << ((struct mailbox_info *)(box_info.ItemAt(mailboxes.IndexOf(mailbox))))->server_mb_name << '\"';
	SendCommand(command.String());
	WasCommandOkay(command);
	//---If this fails, that's fine.
	
	command = "DELETE \"";
	command << ((struct mailbox_info *)(box_info.ItemAt(mailboxes.IndexOf(mailbox))))->server_mb_name << '\"';
	
	SendCommand(command.String());
	if (!WasCommandOkay(command)) {
		command = "Error deleting mailbox ";
		command << mailbox << '.';
		runner->ShowError(command.String());
		
		return B_ERROR;
	}
	
	delete ((struct mailbox_info *)(box_info.RemoveItem(mailboxes.IndexOf(mailbox))));
	
	return B_OK;
}

void IMAP4Client::GetUniqueIDs() {
	BString command;
	char expected[255];
	BString tag;
	BString uid;
	struct mailbox_info *info;
	
	runner->ReportProgress(0,0,"Getting Unique IDs");
	
	for (int32 i = 0; i < mailboxes.CountItems(); i++) {
		Select(mailboxes[i],true,false /* We queue them as a group */);
		
		info = (struct mailbox_info *)(box_info.ItemAt(i));
		if (info->exists <= 0)
			continue;
		
		command = "FETCH 1:";
		command << info->exists << " UID";
		SendCommand(command.String()); 
		
		::sprintf(expected,"a%.7ld",commandCount);
		while(1) {
			NestedString response;
			GetResponse(tag,&response);
						
			if (tag == expected)
				break;
			
			uid = mailboxes[i];
			uid << '/' << response[2][1]();
			unique_ids->AddItem(uid.String());
		}
	}
}

status_t IMAP4Client::Close() {
	if (selected_mb != "") {
		BString worthless;
		SendCommand("CLOSE");
		if (!WasCommandOkay(worthless))
			return B_ERROR;
			
		selected_mb = "";
	}
	
	return B_OK;
}

status_t IMAP4Client::Select(const char *mb, bool reselect, bool queue_new_messages, bool noop, bool no_command, bool ignore_forced_reselect) {
	if (force_reselect && !ignore_forced_reselect) {
		reselect = true;
		force_reselect = false;
	}
	
	if (reselect)
		Close();
	
	struct mailbox_info *info = (struct mailbox_info *)(box_info.ItemAt(mailboxes.IndexOf(mb)));
	if (info == NULL)
		return B_NAME_NOT_FOUND;

	const char *real_mb = info->server_mb_name.String();
	
	if ((selected_mb != real_mb) || (noop) || (no_command)) {
		if ((selected_mb != "")  && (selected_mb != real_mb)){
			BString trash;
			if (SendCommand("CLOSE") < B_OK)
				return B_ERROR;
			selected_mb = "";
			WasCommandOkay(trash);
		}
		BString cmd;
		if (selected_mb == real_mb)
			cmd = "NOOP";
		else
			cmd << "SELECT \"" << real_mb << '\"';
			
		if (!no_command)
			if (SendCommand(cmd.String()) < B_OK)
				return B_ERROR;
		
		char expected[255];
		BString tag;
		::sprintf(expected,"a%.7ld",commandCount);
		
		int32 new_exists(-1), new_next_uid(-1), recent(-1);
		
		while(1) {
			NestedString response;
			if (GetResponse(tag,&response) < B_OK)
				return B_ERROR;
			
			if (tag == expected)
				break;
			
			if ((response.CountItems() > 1) && (strcasecmp(response[1](),"EXISTS") == 0))
				new_exists = atoi(response[0]());
			
			if (response[0].CountItems() == 2 && strcasecmp(response[0][0](),"UIDNEXT") == 0)
				new_next_uid = atol(response[0][1]());
				
			if ((response.CountItems() > 1) && (strcasecmp(response[1](),"RECENT") == 0))
				recent = atoi(response[0]());
		}
		
		if ((queue_new_messages) && (recent > 0)) {
			BString command = "FETCH ";
			command << new_exists - recent + 1 << ':' << new_exists << " UID";
			SendCommand(command.String());
			::sprintf(expected,"a%.7ld",commandCount);
			BStringList list;
			BString uid;
			while(1) {
				NestedString response;
				if (GetResponse(tag,&response) < 0)
					return B_ERROR;
							
				if (tag == expected)
					break;
				
				if (strcmp(response[2][0](),"UID") != 0)
					continue; //--- Courier IMAP blows. Hard.
				
				uid = mb;
				uid << '/' << response[2][1]();
				if (!unique_ids->HasItem(uid.String()))
					list.AddItem(uid.String());
			}
			
			if (list.CountItems() > 0) {
				(*unique_ids) += list;
				runner->GetMessages(&list,-1);
			}
		}
		
		info->exists = new_exists;
		info->next_uid = new_next_uid;
		
		selected_mb = real_mb;
	}
		
	return B_OK;
}

class IMAP4PartialReader : public BPositionIO {
	public:
		IMAP4PartialReader(IMAP4Client *client,BPositionIO *_slave,const char *id) : us(client), slave(_slave), done(false) {
			strcpy(unique,id);
		}
		~IMAP4PartialReader() {
			delete slave;
			us->runner->ReportProgress(0,1);
		}
		off_t Seek(off_t position, uint32 seek_mode) {			
			if (seek_mode == SEEK_END) {
				if (!done) {
					slave->Seek(0,SEEK_END);
					FetchMessage("RFC822.TEXT");
				}
				done = true;
			}
			return slave->Seek(position,seek_mode);
		}
		off_t Position() const {
			return slave->Position();
		}
		ssize_t	WriteAt(off_t pos, const void *buffer, size_t amountToWrite) {
			return slave->WriteAt(pos,buffer,amountToWrite);
		}
		ssize_t	ReadAt(off_t pos, void *buffer, size_t amountToWrite) {
			ssize_t bytes;
			while ((bytes = slave->ReadAt(pos,buffer,amountToWrite)) < amountToWrite && !done) {
					slave->Seek(0,SEEK_END);
					FetchMessage("RFC822.TEXT");
					done = true;
			}
			return bytes;
		}
	private:
		void FetchMessage(const char *part) {
			BString command = "UID FETCH ";
			command << unique << " (" << part << ')';
			us->SendCommand(command.String());
			static char cmd[255];
			::sprintf(cmd,"a%.7ld"CRLF,us->commandCount);
			NestedString response;
			if (us->GetResponse(command,&response) != NOT_COMMAND_RESPONSE && command == cmd)
				return;
			
			//response.PrintToStream();
			
			us->WasCommandOkay(command);
			for (int32 i = 0; (i+1) < response[2].CountItems(); i++) {
				if (strcmp(response[2][i](),part) == 0) {
					slave->Write(response[2][i+1](),strlen(response[2][i+1]()));
					break;
				}
			}
			
		}
			
		IMAP4Client *us;
		char unique[25];
		BPositionIO *slave;
		bool done;
};

status_t IMAP4Client::GetMessage(const char *mailbox, const char *message, BPositionIO **data, BMessage *headers) {	
	{
		//--- Error reporting for non-existant messages often simply doesn't exist. So we have to check first...
		BString uid = mailbox;
		uid << '/' << message;
		if (!unique_ids->HasItem(uid.String())) {
			uid.Prepend("This message (");
			uid.Append(") does not exist on the server. Possibly it was deleted by another client.");
			runner->ShowError(uid.String());
			return B_NAME_NOT_FOUND;
		}
	}
	
	Select(mailbox);
	
	if (headers->FindBool("ENTIRE_MESSAGE")) {				
		BString command = "UID FETCH ";
		command << message << " (FLAGS RFC822)";
		
		SendCommand(command.String());
		static char cmd[255];
		::sprintf(cmd,"a%.7ld"CRLF,commandCount);
		NestedString response;
				
		if (GetResponse(command,&response,true) != NOT_COMMAND_RESPONSE && command == cmd)
			return B_ERROR;
				
		for (int32 i = 0; i < response[2][1].CountItems(); i++) {
			if (strcmp(response[2][1][i](),"\\Seen") == 0) {
				headers->AddString("STATUS","Read");
			}
			if (strcmp(response[2][1][i](),"\\Sent") == 0) {
				if (headers->HasString("STATUS"))
					headers->ReplaceString("STATUS","Sent");
				else
					headers->AddString("STATUS","Sent");
			}
			if (strcmp(response[2][1][i](),"\\Answered") == 0) {
				if (headers->HasString("STATUS"))
					headers->ReplaceString("STATUS","Replied");
				else
					headers->AddString("STATUS","Replied");
			}
		}
		
		WasCommandOkay(command);
		(*data)->WriteAt(0,response[2][5](),strlen(response[2][5]()));
		runner->ReportProgress(0,1);
		return B_OK;
	} else {
		BString command = "UID FETCH ";
		command << message << " (RFC822.SIZE FLAGS RFC822.HEADER)";
		SendCommand(command.String());
		static char cmd[255];
		::sprintf(cmd,"a%.7ld"CRLF,commandCount);
		NestedString response;
		if (GetResponse(command,&response) != NOT_COMMAND_RESPONSE && command == cmd)
			return B_ERROR;
				
		WasCommandOkay(command);
		
		for (int32 i = 0; i < response[2].CountItems(); i++) {
			if (strcmp(response[2][i](),"RFC822.SIZE") == 0) {
				i++;
				headers->AddInt32("SIZE",atoi(response[2][i]()));
			} else if (strcmp(response[2][i](),"FLAGS") == 0) {
				i++;
				for (int32 j = 0; j < response[2][i].CountItems(); j++) {
					if (strcmp(response[2][i][j](),"\\Seen") == 0)
						headers->AddString("STATUS","Read");
					else if (strcmp(response[2][i][j](),"\\Answered") == 0) {
						if (headers->ReplaceString("STATUS","Replied") != B_OK)
							headers->AddString("STATUS","Replied");
					}
				}
			} else if (strcmp(response[2][i](),"RFC822.HEADER") == 0) {
				i++;
				(*data)->Write(response[2][i](),strlen(response[2][i]()));
			}
		}
		
		*data = new IMAP4PartialReader(this,*data,message);
		return B_OK;
	}
}

status_t
IMAP4Client::SendCommand(const char* command)
{
	if (net < 0)
		return B_ERROR;
		
	static char cmd[255];
	::sprintf(cmd,"a%.7ld %s"CRLF,++commandCount,command);
#ifdef USESSL
	if (use_ssl)
		SSL_write(ssl,cmd,strlen(cmd));
	else
#endif
		send(net,cmd,strlen(cmd),0);

	PRINT(("C: %s",cmd));
	
	return B_OK;
}

int32
IMAP4Client::ReceiveLine(BString &out)
{
	if (net < 0)
		return net;
		
	uint8 c = 0;
	int32 len = 0,r;
	out = "";
	
	struct timeval tv;
	struct fd_set fds; 

	tv.tv_sec = long(kIMAP4ClientTimeout / 1e6); 
	tv.tv_usec = long(kIMAP4ClientTimeout-(tv.tv_sec * 1e6)); 
	
	/* Initialize (clear) the socket mask. */ 
	FD_ZERO(&fds);
	
	/* Set the socket in the mask. */ 
	FD_SET(net, &fds);
	int result;
#ifdef USESSL
	if ((use_ssl) && (SSL_pending(ssl)))
		result = 1;
	else
#endif
		result = select(32, &fds, NULL, NULL, &tv);
	
	if (result < 0)
		return errno;
	
	if(result > 0)
	{
		while(c != '\n' && c != xEOF)
		{
		  #ifdef USESSL
			if (use_ssl)
				r = SSL_read(ssl,&c,1);
			else
		  #endif
				r = recv(net,&c,1,0);
			if(r <= 0) {
				BString error;
					error << "Connection to " << settings->FindString("server") << " lost.";
				net = -1;
				runner->Stop();
				runner->ShowError(error.String());
				return -1;
			}
	
			out += (char)c;
			len += r;
		}
	}else{
		// Log an error somewhere instead
		runner->Stop();
		runner->ShowError("IMAP Timeout.");
		return B_TIMED_OUT;
	}
	PRINT(("S:%s\n",out.String()));
	return len;
}

int IMAP4Client::GetResponse(BString &tag, NestedString *parsed_response, bool report_literals, bool internal_flag) {
	if (net < 0)
		return net;
	
	uint8 c = 0;
	int32 r;
	int8 delimiters_passed = internal_flag ? 2 : 0;
	int answer = NOT_COMMAND_RESPONSE;
	BString out;
	bool in_quote = false;
	bool was_cr = false;
	int result;
	
	{
		struct timeval tv;
		struct fd_set fds; 
	
		tv.tv_sec = long(kIMAP4ClientTimeout / 1e6); 
		tv.tv_usec = long(kIMAP4ClientTimeout-(tv.tv_sec * 1e6)); 
		
		/* Initialize (clear) the socket mask. */ 
		FD_ZERO(&fds);
		
		/* Set the socket in the mask. */ 
		FD_SET(net, &fds);
#ifdef USESSL
		if ((use_ssl) && (SSL_pending(ssl)))
			result = 1;
		else
#endif
			result = select(32, &fds, NULL, NULL, &tv);
	}
	
	if (result < 0)
		return errno;
	
	if (!internal_flag)
		PRINT(("S: "));

	if(result > 0)
	{
		while(c != '\n' && c != xEOF)
		{
#ifdef USESSL
			if (use_ssl)
				r = SSL_read(ssl,&c,1);
			else
#endif
				r = recv(net,&c,1,0);
			if(r <= 0) {
				BString error;
				error << "Connection to " << settings->FindString("server") << " lost.";
				net = -1;
				runner->Stop();
				runner->ShowError(error.String());
				return -1;
			}
			
			#if DEBUG
				putchar(c);
			#endif
			
			if ((isspace(c) || (internal_flag && (c == ')' || c == ']'))) && !in_quote) {
				if (delimiters_passed == 0) {
					tag = out;
					out = "";
					delimiters_passed ++;
					continue;
				}
				if (delimiters_passed == 1) {
					
					if (out == "NO")
						answer = NO;
					else if (out == "BAD")
						answer = BAD;
					else if (out == "OK")
						answer = OK;
					else if (parsed_response != NULL && out != "")
						*parsed_response += out;
					
					out = "";
					delimiters_passed ++;
					continue;
				}
				
				if (c == '\r') {
					was_cr = true;
					continue;
				}

				if (c == '\n' && was_cr) {
					if (out.Length() == 0)
						return answer;
					if (out[0] == '{' && out[out.Length() - 1] == '}') {
						int octets_to_read;
						out.Truncate(out.Length() - 1);
						octets_to_read = atoi(out.String() + 1);
						out = "";
						char *buffer = new char[octets_to_read+1];
						buffer[octets_to_read] = 0;
						int read_octets = 0;
						int nibble_size;
						while (read_octets < octets_to_read) {
						  #ifdef USESSL
							if (use_ssl)
								nibble_size = SSL_read(ssl,buffer + read_octets,octets_to_read - read_octets);
							else
						  #endif
								nibble_size = recv(net,buffer + read_octets,octets_to_read - read_octets,0);
							read_octets += nibble_size;
							if (report_literals)
								runner->ReportProgress(nibble_size,0);
						}
						
						if (parsed_response != NULL)
							parsed_response->AdoptAndAdd(buffer);
						else
							delete [] buffer;
							
						c = ' ';
						continue;
					}
				}
				
				if (internal_flag && (c == ')' || c == ']')) {
					if (parsed_response != NULL && out != "")
						(*parsed_response) += out;
					break;
				}
				
				was_cr = false;
				if (parsed_response != NULL && out != "")
					(*parsed_response) += out;
				out = "";
				continue;
			}
			
			was_cr = false;
			
			if (c == '\"') {
				in_quote = !in_quote;
				continue;
			}
			if (c == '(' || c == '[') {
				if (parsed_response != NULL)
					(*parsed_response) += NULL;
					
				BString trash;
				GetResponse(trash,&((*parsed_response)[parsed_response->CountItems() - 1]),report_literals,true);
				continue;
			}
			
			out += (char)c;
		}
	}else{
		// Log an error somewhere instead
		runner->Stop();
		runner->ShowError("IMAP Timeout.");
		return B_TIMED_OUT;
	}
	return answer;
}

bool IMAP4Client::WasCommandOkay(BString &response) {
	do {
		response = "";
		if (ReceiveLine(response) < B_OK) {
			runner->ShowError("No response from server");
			return false;
		}
	} while (response[0] == '*');
		
	bool to_ret = false;
	static char cmd[255];
	::sprintf(cmd,"a%.7ld OK",commandCount);
	if (strncmp(response.String(),cmd,strlen(cmd)) == 0)
		to_ret = true;
	
	int32 i = response.FindFirst(' ');
	i = response.FindFirst(' ',i+1);
	response.Remove(0,i+1);
	response.ReplaceAll("\r\n","\n");
	for (int32 i = response.Length()-1; response.String()[i] == '\n'; i--)
		response.Truncate(i);
	
	return to_ret;
}

BMailFilter *instantiate_mailfilter(BMessage *settings, BMailChainRunner *runner)
{
	return new IMAP4Client(settings,runner);
}
