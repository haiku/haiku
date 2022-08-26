/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		John Scipione <jscipione@gmail.com>
 */
#ifndef _TRACKER_THUMBNAILS_H
#define _TRACKER_THUMBNAILS_H


#include <Bitmap.h>
#include <Mime.h>


namespace BPrivate {


class Model;


status_t GetThumbnailFromAttr(Model* model, BBitmap* icon, BSize size);
bool ShouldGenerateThumbnail(const char* type);


} // namespace BPrivate

using namespace BPrivate;


#endif	// _TRACKER_THUMBNAILS_H
