/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _LINK_SENDER_H
#define _LINK_SENDER_H


#include <OS.h>


namespace BPrivate {
	
class LinkSender {
	public:
		LinkSender(port_id sendport);
		virtual ~LinkSender(void);

		void SetPort(port_id port);
		port_id	Port() const { return fPort; }

		team_id TargetTeam() const;
		void SetTargetTeam(team_id team);

		status_t StartMessage(int32 code, size_t minSize = 0);
		void CancelMessage(void);
		status_t EndMessage(bool needsReply = false);

		status_t Flush(bigtime_t timeout = B_INFINITE_TIMEOUT, bool needsReply = false);

		status_t Attach(const void *data, size_t size);
		status_t AttachString(const char *string, int32 maxLength = -1);
		template <class Type> status_t Attach(const Type& data)
		{
			return Attach(&data, sizeof(Type));
		}

	protected:
		size_t SpaceLeft() const { return fBufferSize - fCurrentEnd; }
		size_t CurrentMessageSize() const { return fCurrentEnd - fCurrentStart; }

		status_t AdjustBuffer(size_t newBufferSize, char **_oldBuffer = NULL);
		status_t FlushCompleted(size_t newBufferSize);

		port_id	fPort;
		team_id fTargetTeam;

		char	*fBuffer;
		size_t	fBufferSize;

		uint32	fCurrentEnd;		// current append position
		uint32	fCurrentStart;		// start of current message

		status_t fCurrentStatus;
};


inline team_id
LinkSender::TargetTeam() const
{
	return fTargetTeam;
}


inline void
LinkSender::SetTargetTeam(team_id team)
{
	fTargetTeam = team;
}

}	// namespace BPrivate

#endif	/* _LINK_SENDER_H */
