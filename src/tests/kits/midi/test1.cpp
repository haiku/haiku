#include <MidiStore.h>
#include <MidiText.h>
#include <Entry.h>
#include <iostream.h>

int main(int argc, char * argv[]) {
	if(argc < 2) {
		cerr << "Must supply a filename (*.mid)!" << endl;
		return 1;
	}
	BMidiText * text = new BMidiText();
	BMidiStore * store = new BMidiStore();
	BEntry entry(argv[1],true);
	if(!entry.Exists()) {
		cerr << "File does not exist." << endl;
		return 2;
	}
	entry_ref e_ref;
	entry.GetRef(&e_ref);
	store->Import(&e_ref);
	store->Connect(text);
	uint32 start_time = B_NOW;
	store->Start();
	while(store->IsRunning()) {
		snooze(100000);
	}
	store->Stop();
	uint32 stop_time = B_NOW;
	cout << "Start Time: " << dec << start_time << "ms" << endl;
	cout << "Stop Time: " << dec << stop_time << "ms" << endl;
	cout << "Total time: " << dec << stop_time - start_time << "ms" << endl;
	
	store->Disconnect(text);
	delete store;
	delete text;
	return 0;
}
