//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new.h>

#include <DiskDevice.h>
#include <DiskScannerAddOn.h>
#include <Partition.h>
#include <Session.h>
#include <View.h>

// IntelParameterEditor

class IntelParameterEditor : public BView, public BDiskScannerParameterEditor {
public:
	IntelParameterEditor(const BSession *session, const char *parameters);
	virtual ~IntelParameterEditor();

	virtual BView *View();
	virtual bool EditingDone();
	virtual status_t GetParameters(BString *parameters);

private:
	const BSession	*fSession;
	BString			fParameters;
};

// constructor
IntelParameterEditor::IntelParameterEditor(const BSession *session,
										   const char *parameters)
	: BView(BRect(0, 0, 100, 100), "intel parameters", B_FOLLOW_ALL, 0),
	  BDiskScannerParameterEditor(),
	  fSession(session),
	  fParameters(parameters)
{
	// for testing
	rgb_color blue = { 0, 0, 255 };
	SetViewColor(blue);
}

// destructor
IntelParameterEditor::~IntelParameterEditor()
{
}

// View
BView *
IntelParameterEditor::View()
{
	return this;
}

// EditingDone
bool
IntelParameterEditor::EditingDone()
{
	return true;
}

// GetParameters
status_t
IntelParameterEditor::GetParameters(BString *parameters)
{
	status_t error = (parameters ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		*parameters = fParameters;
	return error;
}


// IntelPartitionAddOn

class IntelPartitionAddOn : public BDiskScannerPartitionAddOn {
public:
	IntelPartitionAddOn();
	virtual ~IntelPartitionAddOn();

	virtual const char *ShortName();
	virtual const char *LongName();

	virtual BDiskScannerParameterEditor *CreateEditor(const BSession *session,
		const char *parameters);
};

// constructor
IntelPartitionAddOn::IntelPartitionAddOn()
	: BDiskScannerPartitionAddOn()
{
}

// destructor
IntelPartitionAddOn::~IntelPartitionAddOn()
{
}

// ShortName
const char *
IntelPartitionAddOn::ShortName()
{
	return "intel";
}

// LongName
const char *
IntelPartitionAddOn::LongName()
{
	return "Intel Style Partitioning";
}

// CreateEditor
BDiskScannerParameterEditor *
IntelPartitionAddOn::CreateEditor(const BSession *session,
								  const char *parameters)
{
	return new(nothrow) IntelParameterEditor(session, parameters);
}

// create_ds_partition_add_on
BDiskScannerPartitionAddOn *
create_ds_partition_add_on()
{
	return new IntelPartitionAddOn;
}

