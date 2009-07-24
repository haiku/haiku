/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_INFO_H
#define IMAGE_INFO_H

#include <image.h>
#include <String.h>

#include "Types.h"


class ImageInfo {
public:
								ImageInfo();
								ImageInfo(const ImageInfo& other);
								ImageInfo(team_id team, image_id image,
									const BString& name, image_type type,
									target_addr_t textBase,
									target_size_t textSize,
									target_addr_t dataBase,
									target_size_t dataSize);

			void				SetTo(team_id team, image_id image,
									const BString& name, image_type type,
									target_addr_t textBase,
									target_size_t textSize,
									target_addr_t dataBase,
									target_size_t dataSize);

			team_id				TeamID() const	{ return fTeam; }
			image_id			ImageID() const	{ return fImage; }
			const BString&		Name() const	{ return fName; }
			image_type			Type() const	{ return fType; }

			target_addr_t		TextBase() const	{ return fTextBase; }
			target_size_t		TextSize() const	{ return fTextSize; }
			target_addr_t		DataBase() const	{ return fDataBase; }
			target_size_t		DataSize() const	{ return fDataSize; }

private:
			thread_id			fTeam;
			image_id			fImage;
			BString				fName;
			image_type			fType;
			target_addr_t		fTextBase;
			target_size_t		fTextSize;
			target_addr_t		fDataBase;
			target_size_t		fDataSize;
};


#endif	// IMAGE_INFO_H
