/******************************************************************************
/
/	File:			RadeonAddOn.h
/
/	Description:	ATI Radeon Video Capture Media AddOn for BeOS.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __RADEON_ADDON_H__
#define __RADEON_ADDON_H__

#include <media/MediaAddOn.h>
#include <Path.h>
#include "benaphore.h"

#define TOUCH(x) ((void)(x))

extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id you);

class CRadeonAddOn;

// descriptor of a possible Radeon node
class CRadeonPlug {
public:
	CRadeonPlug( CRadeonAddOn *addon, const BPath &dev_path, int id );
	~CRadeonPlug() {}
	
	const char *getName() { return fFlavorInfo.name; }
	const char *getDeviceName() { return dev_path.Path(); }
	const char *getDeviceShortName() { return dev_path.Leaf(); }
	flavor_info	*getFlavorInfo() { return &fFlavorInfo; }
	BMediaNode	*getNode() { return node; }
	void		setNode( BMediaNode *anode ) { node = anode; }
	BMessage	*getSettings() { return &settings; }
	
	void		writeSettings( BMessage *new_settings );

private:
	CRadeonAddOn	*addon;		// (global) addon (only needed for GetConfiguration())
	BPath 			dev_path;		// path of device driver
	int				id;				// id of plug
	flavor_info		fFlavorInfo;	// flavor of plug (contains unique id)	
	BMediaNode		*node;			// active node (or NULL)
	BMessage		settings;		// settings for this node
	media_format	fMediaFormat[4];
	
	void		readSettings();
	BPath 		getSettingsPath();
};

class CRadeonAddOn : public BMediaAddOn
{
public:
						CRadeonAddOn(image_id imid);
	virtual 			~CRadeonAddOn();
	
	virtual	status_t	InitCheck(const char **out_failure_text);

	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode	*InstantiateNodeFor(
							const flavor_info * info,
							BMessage * config,
							status_t * out_error);

	virtual	status_t	SaveConfigInfo(BMediaNode *node, BMessage *message)
								{ TOUCH(node); TOUCH(message); return B_OK; }

	virtual	bool		WantsAutoStart() { return false; }
	virtual	status_t	AutoStart(int in_count, BMediaNode **out_node,
								int32 *out_internal_id, bool *out_has_more)
								{	TOUCH(in_count); TOUCH(out_node);
									TOUCH(out_internal_id); TOUCH(out_has_more);
									return B_ERROR; }
									
	virtual status_t	GetConfigurationFor(
							BMediaNode *your_node,
							BMessage *into_message );
		
// private methods
	void				UnregisterNode( BMediaNode *node, BMessage *settings );

private:
	status_t			fInitStatus;
	BList				fDevices;		// list of BPath of found devices
	thread_id			settings_thread;
	sem_id				settings_thread_sem;
	benaphore			plug_lock;
	BMessage			settings;
	
	status_t			RecursiveScan( const char* rootPath, BEntry *rootEntry = NULL );
	static int32 		settings_writer( void *param );
	void				settingsWriter();
	void 				writeSettings();
};

#endif
