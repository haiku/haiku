//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Partition.h>
#include <OS.h>
#include <Session.h>

#include <Messenger.h>		// needed only until we can link against
#include <RegistrarDefs.h>	// libopenbeos
#include <Roster.h>			//
#include <RosterPrivate.h>	//

// Hack to make BDiskDeviceRoster communicate with our registrar.

BRoster _be_roster;
const BRoster *be_roster = &_be_roster;

// init_roster
void
init_roster()
{
	bool initialized = false;
	// find the registrar port
	port_id rosterPort = find_port(kRosterPortName);
	port_info info;
	if (rosterPort >= 0 && get_port_info(rosterPort, &info) == B_OK) {
		// construct the roster messenger
		struct {
			port_id	fPort;
			int32	fHandlerToken;
			team_id	fTeam;
			int32	extra0;
			int32	extra1;
			bool	fPreferredTarget;
			bool	extra2;
			bool	extra3;
			bool	extra4;
		} fakeMessenger;
		fakeMessenger.fPort = rosterPort;
		fakeMessenger.fHandlerToken = 0;
		fakeMessenger.fTeam = info.team;
		fakeMessenger.fPreferredTarget = true;
		BMessenger mainMessenger = *(BMessenger*)&fakeMessenger;
		// ask for the MIME messenger
		BMessage reply;
		status_t error = mainMessenger.SendMessage(B_REG_GET_MIME_MESSENGER,
												   &reply);
		if (error == B_OK && reply.what == B_REG_SUCCESS) {
			BMessenger mimeMessenger;
			reply.FindMessenger("messenger", &mimeMessenger);
			BRoster::Private(_be_roster).SetTo(mainMessenger, mimeMessenger);
			initialized = true;
		} else {
		}
	}
	if (!initialized) {
		printf("initializing be_roster failed!\n");
		exit(1);
	}
}


class DumpVisitor : public BDiskDeviceVisitor {
public:
	virtual bool Visit(BDiskDevice *device)
	{
		printf("device `%s'\n", device->Path());
		printf("  size:          %lld\n", device->Size());
		printf("  block size:    %ld\n", device->BlockSize());
		printf("  read-only:     %d\n", device->IsReadOnly());
		printf("  removable:     %d\n", device->IsRemovable());
		printf("  has media:     %d\n", device->HasMedia());
		printf("  type:          0x%x\n", device->Type());
		return false;
	}
	
	virtual bool Visit(BSession *session)
	{
		printf("  session %ld:\n", session->Index());
		printf("    offset:        %lld\n", session->Offset());
		printf("    size:          %lld\n", session->Size());
		printf("    block size:    %ld\n", session->BlockSize());
		printf("    flags:         %lx\n", session->Flags());
		printf("    partitioning : `%s'\n", session->PartitioningSystem());
		return false;
	}
	
	virtual bool Visit(BPartition *partition)
	{
		printf("    partition %ld:\n", partition->Index());
		printf("      offset:         %lld\n", partition->Offset());
		printf("      size:           %lld\n", partition->Size());
//		printf("      device:         `%s'\n", partition->Size());
		printf("      flags:          %lx\n", partition->Flags());
		printf("      partition name: `%s'\n", partition->Name());
		printf("      partition type: `%s'\n", partition->Type());
		printf("      FS short name:  `%s'\n",
			   partition->FileSystemShortName());
		printf("      FS long name:   `%s'\n",
			   partition->FileSystemLongName());
		printf("      volume name:    `%s'\n", partition->VolumeName());
		printf("      FS flags:       0x%lx\n", partition->FileSystemFlags());
		return false;
	}
};

// main
int
main()
{
	init_roster();
	BDiskDeviceRoster roster;
	DumpVisitor visitor;
	roster.Traverse(&visitor);
}

