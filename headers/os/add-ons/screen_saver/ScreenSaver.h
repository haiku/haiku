#if ! defined _SCREENSAVER_H
#define _SCREENSAVER_H

#include <BeBuild.h>
#include <DirectWindow.h>	// for direct_buffer_info
#include <image.h>
#include <Message.h>

class BView;

class BScreenSaver
{
public:
						BScreenSaver(BMessage *archive, image_id);
	virtual				~BScreenSaver();

	// Return an error if something goes wrong or if you don't
	// like the running environment (lack of 3D acceleration,
	// lack of sound, ...).
	virtual status_t	InitCheck();

	// Animation start and stop. The view is not yet visible when
	// StartSaver is called.
	virtual status_t	StartSaver(BView *view, bool preview);
	virtual void		StopSaver();

	// This hook is called periodically, you should
	// override it to do the drawing. Notice that you
	// should clear the screen when frame == 0 if you
	// don't want to paint over the desktop.
	virtual void		Draw(BView *view, int32 frame);

	// These hooks are for direct screen access, the first is
	// called every time the settings change, the second is
	// the equivalent of Draw to be called to access the screen
	// directly. Draw and DirectDraw can be used at the same time.
	virtual void		DirectConnected(direct_buffer_info *info);
	virtual void		DirectDraw(int32 frame);

	// configuration dialog methods
	virtual void		StartConfig(BView *configView);
	virtual void		StopConfig();

	// Module should fill this in with metadata
	// example: randomizable = true
	virtual void		SupplyInfo(BMessage *info) const;

	// Send all the metadata info to the module
	virtual void		ModulesChanged(const BMessage *info);

	// BArchivable like parameter saver method
	virtual	status_t	SaveState(BMessage *into) const;

	// These methods can be used to control drawing frequency.
	void				SetTickSize(bigtime_t ts);
	bigtime_t			TickSize() const;

	// These methods can be used to control animation loop cycles
	void				SetLoop(int32 on_count, int32 off_count);
	int32				LoopOnCount() const;
	int32				LoopOffCount() const;

private:
	virtual	void _ReservedScreenSaver1();
	virtual	void _ReservedScreenSaver2();
	virtual	void _ReservedScreenSaver3();
	virtual	void _ReservedScreenSaver4();
	virtual	void _ReservedScreenSaver5();
	virtual	void _ReservedScreenSaver6();
	virtual	void _ReservedScreenSaver7();
	virtual	void _ReservedScreenSaver8();

	bigtime_t	ticksize;
	int32		looponcount;
	int32		loopoffcount;

	uint32		_reservedScreenSaver[6];
};

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id id);

//_EXPORT int32 saverVersionID;

#endif
