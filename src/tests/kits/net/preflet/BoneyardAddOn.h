//******************************************************************************
//
//      File:           bone_byaddon.h
//
//      Description:    Addon API for the BONE configuration preference.
//
//      Copyright 2000, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef H_BONE_BYADDON
#define H_BONE_BYADDON

#include <List.h>
#include <View.h>

class BYDataEntry {
public:
	BYDataEntry(const char *name,const char *value,const char *ccomment = NULL);
	virtual ~BYDataEntry();

	const char *Name();
	const char *Value();
	const char *Comment();
	void SetValue(char *value);
	void SetComment(char *comment);

private:
    virtual void _ReservedDataEntry1();
    virtual void _ReservedDataEntry2();

	char *m_name,*m_value,*m_comment;
	
	uint32 _reserved[2];
};

// Common importance values used by BYAddon
#define BY_IMP_IDENTITY		200		// priority of the "Identity" tab
#define BY_IMP_INTERFACES	190		// priority of the "Interfaces" tab
#define BY_IMP_HIGH			150		// put the tab up front
#define BY_IMP_PPP			140		// priority of the "Dial-Up" (PPP) tab
#define BY_IMP_NORMAL		100		// anywhere is fine
#define BY_IMP_SERVICES		 90		// priority of the "Services" tab
#define BY_IMP_PROFILES		 10		// priority of the "Profiles" tab
#define BY_IMP_LOW			  0		// put near the back

class BYAddon {
public:
	BYAddon();
	virtual ~BYAddon();
	
	// Perform actions
	virtual BView *CreateView(BRect *bounds);	// return config view
	virtual void Revert();						// Revert to settings on disk
	virtual void Save();						// Save changes to disk

	// status/info
	virtual const char *Name();					// name of addon (for tab)
	virtual const char *Section();				// section name (for network.conf)
	virtual const char *Description();			// description of section
	virtual const BList *FileList();			// list of files used for configuration
	virtual int Importance();					// sort order in tab list.
	
	// For data collection/management
	const BList *ObtainSectionData();			// get all BYDataEntrys for my section.
	const char *GetValue(const char *name);		// get a value from your section.
	void SetValue(const char *name,const char *data);		// set a value in your section.
	void ClearValue(const char *name);			// remove an entry from your section.
	void ClearSectionData();					// remove all entries from your section.
	bool IsDirty();								// query dirty status
	void SetDirty(bool _dirty = true);			// set dirty status
	const BList *GetAddonList();				// get all add-ons in the cradle.

private:
	virtual void _ReservedAddon1();
	virtual void _ReservedAddon2();
	virtual void _ReservedAddon3();
	virtual void _ReservedAddon4();
	virtual void _ReservedAddon5();
	virtual void _ReservedAddon6();
	virtual void _ReservedAddon7();
	virtual void _ReservedAddon8();

	bool is_dirty;
	int32 _reserved[16];
};

extern "C" {

typedef BYAddon *(*by_instantiate_func)(void);

#if defined (__POWERPC__)
#define BONE_EXPORT __declspec(dllexport)
#else
#define BONE_EXPORT
#endif

BONE_EXPORT BYAddon *instantiate(void);

extern void boneyard_do_save();
extern void boneyard_do_revert();

}

#endif
