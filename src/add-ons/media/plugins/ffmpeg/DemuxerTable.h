/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEMUXER_TABLE_H
#define DEMUXER_TABLE_H


#include <MediaDefs.h>


struct AVInputFormat;

const media_file_format* demuxer_format_for(AVInputFormat* format);


#endif // DEMUXER_TABLE_H
