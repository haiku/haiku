/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 */
#ifndef SERVER_PICTURE_H
#define SERVER_PICTURE_H


#include <DataIO.h>

#include <ObjectList.h>
#include <PictureDataWriter.h>
#include <Referenceable.h>


class ServerApp;
class View;
class BFile;

namespace BPrivate {
	class LinkReceiver;
	class PortLink;
}
class BList;


class ServerPicture : public BReferenceable, public PictureDataWriter {
public:
								ServerPicture();
								ServerPicture(const ServerPicture& other);
								ServerPicture(const char* fileName,
									int32 offset);
								~ServerPicture();

			int32				Token() { return fToken; }
			bool				SetOwner(ServerApp* owner);
			ServerApp*			Owner() const { return fOwner; }

			bool				ReleaseClientReference();

			void				EnterStateChange();
			void				ExitStateChange();

			void				SyncState(View* view);
			void				SetFontFromLink(BPrivate::LinkReceiver& link);

			void				Play(View* view);

			void 				PushPicture(ServerPicture* picture);
			ServerPicture*		PopPicture();

			void				AppendPicture(ServerPicture* picture);
			bool				NestPicture(ServerPicture* picture);

			off_t				DataLength() const;

			status_t			ImportData(BPrivate::LinkReceiver& link);
			status_t			ExportData(BPrivate::PortLink& link);

private:
			typedef BObjectList<ServerPicture> PictureList;

			int32				fToken;
			BFile*				fFile;
			BPositionIO*		fData;
			PictureList*		fPictures;
			ServerPicture*		fPushed;
			ServerApp*			fOwner;
};


#endif	// SERVER_PICTURE_H
