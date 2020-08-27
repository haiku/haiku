/*
 * Copyright 2001-2019, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SERVER_PICTURE_H
#define SERVER_PICTURE_H


#include <DataIO.h>

#include <AutoDeleter.h>
#include <ObjectList.h>
#include <PictureDataWriter.h>
#include <Referenceable.h>


class BFile;
class Canvas;
class ServerApp;
class ServerFont;
class View;

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
	virtual						~ServerPicture();

			int32				Token() { return fToken; }
			bool				SetOwner(ServerApp* owner);
			ServerApp*			Owner() const { return fOwner; }

			bool				ReleaseClientReference();

			void				EnterStateChange();
			void				ExitStateChange();

			void				SyncState(Canvas* canvas);
			void				WriteFontState(const ServerFont& font,
									uint16 mask);

			void				Play(Canvas* target);

			void 				PushPicture(ServerPicture* picture);
			ServerPicture*		PopPicture();

			void				AppendPicture(ServerPicture* picture);
			bool				NestPicture(ServerPicture* picture);

			off_t				DataLength() const;

			status_t			ImportData(BPrivate::LinkReceiver& link);
			status_t			ExportData(BPrivate::PortLink& link);

private:
	friend class PictureBoundingBoxPlayer;

			typedef BObjectList<ServerPicture> PictureList;

			int32				fToken;
			ObjectDeleter<BFile>
								fFile;
			ObjectDeleter<BPositionIO>
								fData;
			ObjectDeleter<PictureList>
								fPictures;
			BReference<ServerPicture>
								fPushed;
			ServerApp*			fOwner;
};


#endif	// SERVER_PICTURE_H
