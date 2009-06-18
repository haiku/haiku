/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_INFO_H
#define IMAGE_INFO_H

#include <image.h>
#include <String.h>


class ImageInfo {
public:
								ImageInfo();
								ImageInfo(const ImageInfo& other);
								ImageInfo(team_id team, image_id image,
									const BString& name);

			void				SetTo(team_id team, image_id image,
									const BString& name);

			team_id				TeamID() const	{ return fTeam; }
			image_id			ImageID() const	{ return fImage; }
			const char*			Name() const	{ return fName.String(); }

private:
			thread_id			fTeam;
			image_id			fImage;
			BString				fName;
};


#endif	// IMAGE_INFO_H
