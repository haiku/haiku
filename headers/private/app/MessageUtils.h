//------------------------------------------------------------------------------
//	MessageUtils.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <ByteOrder.h>
#include <DataIO.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

uint32 _checksum_(const uchar *buf, int32 size);

namespace BPrivate {	// Only putting these here because Be did

status_t entry_ref_flatten(char* buffer, size_t* size, const entry_ref* ref);
status_t entry_ref_unflatten(entry_ref* ref, const char* buffer, size_t size);
status_t entry_ref_swap(char* buffer, size_t size);

size_t calc_padding(size_t size, size_t boundary);

}	// namespace BPrivate

//------------------------------------------------------------------------------
// _set_message_target_
/*!	\brief Sets the target of a message.

	\param message The message.
	\param token The target handler token.
	\param preferred Indicates whether to use the looper's preferred handler.
*/
inline
void
_set_message_target_(BMessage *message, int32 token, bool preferred)
{
	message->fTarget = token;
	message->fPreferred = preferred;
}
//------------------------------------------------------------------------------
// _set_message_reply_
/*!	\brief Sets a message's reply target.

	\param message The message.
	\param messenger The reply messenger.
*/
inline
void
_set_message_reply_(BMessage *message, BMessenger messenger)
{
	message->fReplyTo.port = messenger.fPort;
	message->fReplyTo.target = messenger.fHandlerToken;
	message->fReplyTo.team = messenger.fTeam;
	message->fReplyTo.preferred = messenger.fPreferredTarget;
}
//------------------------------------------------------------------------------
inline int32 _get_message_target_(BMessage *msg)
{
	return msg->fTarget;
}
//------------------------------------------------------------------------------
inline bool _use_preferred_target_(BMessage *msg)
{
	return msg->fPreferred;
}
//------------------------------------------------------------------------------
inline status_t normalize_err(status_t err)
{
	return err >= 0 ? B_OK : err;
}
//------------------------------------------------------------------------------
template<class T>
inline void byte_swap(T& /*data*/)
{
	// Specialize for data types which actually swap
}
//------------------------------------------------------------------------------
inline void write_helper(BDataIO* stream, const void* data, size_t size,
						 status_t& err)
{
	err = normalize_err(stream->Write(data, size));
}
//------------------------------------------------------------------------------
// Will the template madness never cease??
template<class T>
inline status_t write_helper(BDataIO* stream, T& data, status_t& err)
{
	return normalize_err(stream->Write((const void*)data, sizeof (T)));
}
//------------------------------------------------------------------------------
class TReadHelper
{
	public:
		TReadHelper(BDataIO* stream)
			:	fStream(stream), err(B_OK), fSwap(false)
		{
			;
		}

		TReadHelper(BDataIO* stream, bool swap)
			:	fStream(stream), err(B_OK), fSwap(swap)
		{
			;
		}

		template<class T> inline void operator()(T& data)
		{
			err = normalize_err(fStream->Read((void*)&data, sizeof (T)));
			if (err)
				throw err;

			if (IsSwapping())
			{
				byte_swap(data);
			}
		}

		template<class T> inline void operator()(T data, size_t len)
		{
			err = normalize_err(fStream->Read((void*)data, len));
			if (err)
				throw err;
		}

		status_t	Status() { return err; }
		void		SetSwap(bool yesNo) { fSwap = yesNo; }
		bool		IsSwapping() { return fSwap; }

	private:
		BDataIO*	fStream;
		status_t	err;
		bool		fSwap;
};
//------------------------------------------------------------------------------
class TChecksumHelper
{
	public:
		TChecksumHelper(uchar* buffer) : fBuffer(buffer), fBufPtr(buffer) {;}

		template<class T> inline void Cache(T& data)
		{
			*((T*)fBufPtr) = data;
			fBufPtr += sizeof (T);
		}

		int32    CheckSum();

	private:
		uchar*		fBuffer;
		uchar*		fBufPtr;
};
//------------------------------------------------------------------------------
template<class T> inline status_t read_helper(BDataIO* stream, T& data)
{
	return normalize_err(stream->Read((void*)&data, sizeof (T)));
}
//------------------------------------------------------------------------------
template<> inline void byte_swap(double& data)
{
	data = __swap_double(data);
}
template<> inline void byte_swap(float& data)
{
	data = __swap_float(data);
}
template<> inline void byte_swap(int64& data)
{
	data = __swap_int64(data);
}
template<> inline void byte_swap(int32& data)
{
	data = __swap_int32(data);
}
template<> inline void byte_swap(int16& data)
{
	data = __swap_int16(data);
}
template<> inline void byte_swap(entry_ref& data)
{
	byte_swap(data.device);
	byte_swap(data.directory);
}
//------------------------------------------------------------------------------

#endif	// MESSAGEUTILS_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

