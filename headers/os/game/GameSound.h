/*******************************************************************************
/
/	File:			GameGameSound.h
/
/   Description:    BGameSound is an abstract base class for all sounds being
/					played using the gamesound kit. Use one of the concrete
/					subclasses for actually playing sound.
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined(_GAME_SOUND_H)
#define _GAME_SOUND_H


#include <GameSoundDefs.h>
#include <new>

struct gs_audio_format;
class BGameSoundDevice;

namespace BPrivate {
	class PrivGameSound;
}

class BGameSound
{
public:
							BGameSound(
									BGameSoundDevice * device = NULL);
virtual						~BGameSound();

virtual	BGameSound *		Clone() const = 0;
		status_t			InitCheck() const;

		BGameSoundDevice *	Device() const;
		gs_id				ID() const;
		const gs_audio_format &
							Format() const;		//	only valid after Init()

virtual	status_t			StartPlaying();
virtual	bool				IsPlaying();
virtual	status_t			StopPlaying();

		status_t			SetGain(
									float gain,
									bigtime_t duration = 0);	//	ramp duration in seconds
		status_t			SetPan(
									float pan,
									bigtime_t duration = 0);	//	ramp duration in seconds
		float				Gain();
		float				Pan();

virtual	status_t			SetAttributes(
									gs_attribute * inAttributes,
									size_t inAttributeCount);
virtual	status_t			GetAttributes(
									gs_attribute * outAttributes,
									size_t inAttributeCount);

virtual	status_t Perform(int32 selector, void * data);

		void * 				operator new(
									size_t size);
		void *				operator new(
									size_t size,
									const nothrow_t &) throw();
		void				operator delete(
									void * ptr);
#if !__MWERKS__
		//	there's a bug in MWCC under R4.1 and earlier
		void				operator delete(
									void * ptr, 
									const nothrow_t &) throw();
#endif

static	status_t			SetMemoryPoolSize(		//	implemented in PrivGameSound.cpp
									size_t in_poolSize);
static	status_t			LockMemoryPool(			//	implemented in PrivGameSound.cpp
									bool in_lockInCore);
static	int32				SetMaxSoundCount(		//	implemented in PrivGameSound.cpp
									int32 in_maxCount);

protected:

		status_t			SetInitError(
									status_t in_initError);
		status_t			Init(
									gs_id handle);

							BGameSound(
									const BGameSound & other);
		BGameSound &		operator=(
									const BGameSound & other);

private:

	friend class BPrivate::PrivGameSound;

		gs_id	_m_handle;
		BGameSoundDevice *	_m_device;
		gs_audio_format
							_m_format;
		status_t			_m_initError;

	/* leave these declarations private unless you plan on actually implementing and using them. */
	BGameSound();

	/* fbc data and virtuals */

	uint32 _reserved_BGameSound_[16];

virtual	status_t _Reserved_BGameSound_0(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_23(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_24(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_25(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_26(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_27(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_28(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_29(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_30(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_31(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_32(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_33(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_34(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_35(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_36(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_37(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_38(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_39(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_40(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_41(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_42(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_43(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_44(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_45(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_46(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_47(int32 arg, ...);

};


#endif	//	_GAME_SOUND_H
