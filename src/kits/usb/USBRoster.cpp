
#include <USBKit.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Application.h>
#include <Directory.h>
#include <List.h>
#include <MessageFilter.h>
#include <String.h>
#include <Path.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>

static int __directory = 0;
static void *DIRECTORY = (void *) &__directory;

struct WatchItem
{
	ino_t inode;
	
	char *name;
	void *data;
	
	WatchItem *children;
	WatchItem *next;
	WatchItem *parent;	
	
	~WatchItem(){
		free(name);
	}
	
	char *LongName(void){
		WatchItem *w;
		int sz;
		char *n;
		for(w=parent,sz=1+strlen(name);w;w=w->parent){
			sz += 1+strlen(w->name);
		}
		n = (char*) malloc(sz);
		sz--;
		n[sz] = 0;
		for(w=this;w;w=w->parent){
			sz -= strlen(w->name);
			memcpy(n + sz, w->name, strlen(w->name));
			if(sz){
				sz--;
				n[sz]='/';
			}
		}
		return n;		
	}
	
	WatchItem(ino_t inode, const char *name, void *data = NULL){
		parent = NULL;
		children = NULL;
		next = NULL;
		
		this->inode = inode;
		this->name = strdup(name);
		this->data = data;
	}

	WatchItem *Find(const char *name0){
		WatchItem *w;
		for(w=children;w;w=w->next){
			if(!strcmp(name0,w->name)) return w;
		}
		return NULL;
	}
		
	WatchItem *Find(ino_t inode0){
		WatchItem *w;
		
		if(inode == inode0) {
			return this;
		}
			
		for(w=children;w;w=w->next){
			WatchItem *r = w->Find(inode0);
			if(r) {
				return r;
			}
		}
		return NULL;
	}
	
	WatchItem *Remove(WatchItem *w){
		char *n = w->LongName();
		free(n);
		
		if(w->parent != this) return w;
		w->parent = NULL;
		
		if(children == w){
			children = w->next;
			w->next = NULL;
			return w;
		}
		WatchItem *w0;
		for(w0=children;w0;w0=w0->next){
			if(w0->next == w){
				w0->next = w->next;
				w->next = NULL;
				return w;
			}
		}
		return w;
	}
	
	void Add(WatchItem *w){
		w->parent = this;
		w->next = children;
		children = w;
		
		char *n = w->LongName();
		free(n);
	}
};

class USBRosterLooper : public BLooper
{
	friend USBRoster;
	
	void DestroyItem(WatchItem *item);
	void ItemAdded(WatchItem *item);
	void WatchDirectory(BEntry &dir, WatchItem *parent);
	BMessenger *messenger;
	WatchItem *root;
	USBRoster *roster;
	dev_t dev;
	
public:
	USBRosterLooper(USBRoster *roster);
	virtual void MessageReceived(BMessage *msg);
};


void
USBRosterLooper::WatchDirectory(BEntry &_entry, WatchItem *parent)
{
	node_ref nref; 
	BEntry entry;
	BDirectory dir(&_entry);
	if(dir.GetNodeRef(&nref)) return;  // Not a directory
	BPath path(&_entry);
	WatchItem *item;
	
//	fprintf(stderr,"Watching %s\n",path.Path());
	
	item = parent ? parent->Find(nref.node) : NULL;
	if(!item) {
		item = new WatchItem(nref.node,path.Leaf(), DIRECTORY);
		watch_node(&nref, B_WATCH_DIRECTORY, *messenger);
	}
	
	if(!parent){
//		fprintf(stderr,"create root\n");
		parent = root = item;
	} else {
		parent->Add(item);
	}
	
			
	while(dir.GetNextEntry(&entry) == B_NO_ERROR){
		BPath path(&entry);
		if(entry.IsDirectory()){
			WatchDirectory(entry, item);
		} else {
			if(!strcmp(path.Leaf(),"unload")) continue;
//			fprintf(stderr,"File %s\n",path.Path());
			if(!item->Find(path.Leaf())){
				struct stat s;
				if(!entry.GetStat(&s)){
					USBDevice *dev = new USBDevice(path.Path());
					if(dev->InitCheck() == B_OK){
						WatchItem *wi = new WatchItem(s.st_ino,path.Leaf(),dev);
						item->Add(wi);
						ItemAdded(wi);
					} else {
						delete dev;
						item->Add(new WatchItem(s.st_ino,path.Leaf()));
					}
				}
			}
		}
	}
}
      
void 
USBRosterLooper::MessageReceived(BMessage *msg)
{
	int32 opcode;
	if (msg->FindInt32("opcode", &opcode) != B_NO_ERROR) return;
	
	switch(opcode){
	case B_ENTRY_CREATED: {	
		ino_t dir;
		dev_t device;
		const char *name;
		if (msg->FindInt32("device", &device) != B_NO_ERROR ||
			msg->FindInt64("directory", &dir) != B_NO_ERROR ||
			msg->FindString("name", &name) != B_NO_ERROR) break;
	
		if(!strcmp(name,"unload")) break;
		
//		fprintf(stderr,"created - %s (in %Ld)\n",name,dir);
		// Locate the directory by its inode
		WatchItem *item = root->Find(dir);
		if(item){
			// Item already exists?
			if(!item->Find(name)){
				entry_ref entryRef(device, dir, name);
				BEntry entry(&entryRef);
				if(entry.IsDirectory()){
					WatchDirectory(entry,item);
				} else {
					struct stat s;
					entry.GetStat(&s);
					BPath path(&entry);
					USBDevice *dev = new USBDevice(path.Path());
					if(dev->InitCheck() == B_OK){
						WatchItem *wi = new WatchItem(s.st_ino,name,dev);
						item->Add(wi);
						ItemAdded(wi);
					} else {
						delete dev;
						item->Add(new WatchItem(s.st_ino,name));
					}
				}	
			} else {
				fprintf(stderr,"no item\n");
			}
		}
		break;
	}

	case B_ENTRY_REMOVED: {		
		ino_t dir;
		dev_t device;
		ino_t node;
		if (msg->FindInt32("device", &device) != B_NO_ERROR ||
			msg->FindInt64("directory", &dir) != B_NO_ERROR ||
			msg->FindInt64("node", &node) != B_NO_ERROR) break;
//		fprintf(stderr,"removed - %Ld (in %Ld)\n",node,dir);
		
		// Locate the item by its inode
		WatchItem *item = root->Find(node);
		if(item && item->parent){
			DestroyItem(item->parent->Remove(item));
		} else {
			fprintf(stderr,"no item\n");
		}
	}
	}
}

void 
USBRosterLooper::DestroyItem(WatchItem *item)
{
	if(!item) return;
	if(item->data) {
		if(item->data == DIRECTORY){
			node_ref nref;
			nref.device = dev;
			nref.node = item->inode;
			watch_node(&nref, B_STOP_WATCHING, *messenger);
		} else {
			roster->DeviceRemoved((USBDevice *)item->data);
			delete ((USBDevice *)item->data);
		}
	}
	if(item->children){
		WatchItem *w = item->children;
		while(w) {
			WatchItem *next = w->next;
			DestroyItem(w);
			w = next;
		}
	}
	delete item;
}

void
USBRosterLooper::ItemAdded(WatchItem *item)
{
	if(!item) return;
	if(item->data && (item->data != DIRECTORY)){
		if(roster->DeviceAdded((USBDevice *)item->data) != B_OK){
			delete ((USBDevice *) item->data);
			item->data = NULL;
		}
	}
}

USBRosterLooper::USBRosterLooper(USBRoster *r)
{
	BEntry path("/dev/bus/usb");
	if(path.InitCheck()) fprintf(stderr,"BLAGHR!");
	
	roster = r;
	node_ref nref; 
	path.GetNodeRef(&nref);
	dev = nref.device;
	
	Run();
	messenger = new BMessenger(this);
	Lock();
	WatchDirectory(path,NULL);
	Unlock();
}


void 
USBRoster::Start(void)
{
	if(!rlooper){
		rlooper = new USBRosterLooper(this);
	}
}

void 
USBRoster::Stop(void)
{
	if(rlooper){
		rlooper->Lock();
		rlooper->DestroyItem(rlooper->root);
		rlooper->Quit();
		rlooper = NULL;
	}
}

USBRoster::USBRoster(void)
{
	rlooper = NULL;
}

USBRoster::~USBRoster()
{
	Stop();
}


