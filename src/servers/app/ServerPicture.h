/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 */
#ifndef SERVER_PICTURE_H
#define SERVER_PICTURE_H

#include <DataIO.h>

#include <PictureDataWriter.h>


class ServerApp;
class View;
namespace BPrivate {
	class LinkReceiver;
	class PortLink;
}
class BList;

class ServerPicture : public PictureDataWriter {
public:	
		int32		Token() { return fToken; }
		
		void		EnterStateChange();
		void		ExitStateChange();
		
		void		SyncState(View *view);
		void		SetFontFromLink(BPrivate::LinkReceiver& link);	
		
		void		Play(View *view);
		
		void 		Usurp(ServerPicture *newPicture);
		ServerPicture*	StepDown();		
		
		bool		NestPicture(ServerPicture *picture);

		off_t		DataLength() const;
		
		status_t	ImportData(BPrivate::LinkReceiver &link);
		status_t	ExportData(BPrivate::PortLink &link);

private:
friend class	ServerApp;
	
		ServerPicture();
		ServerPicture(const ServerPicture &);
		ServerPicture(const char *fileName, const int32 &offset);
		~ServerPicture();

		int32		fToken;
		BPositionIO	*fData;
		// DrawState	*fState;
		BList		*fPictures;
		ServerPicture	*fUsurped;
};

#endif	// SERVER_PICTURE_H
