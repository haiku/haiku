/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_POSIX_H
#define _AUTO_DELETER_POSIX_H


#include <AutoDeleter.h>
#include <stdio.h>
#include <dirent.h>
#include <fs_attr.h>


namespace BPrivate {


typedef CObjectDeleter<FILE, int, fclose> FileCloser;
typedef CObjectDeleter<DIR, int, closedir> DirCloser;
typedef CObjectDeleter<DIR, int, fs_close_attr_dir> AttrDirCloser;


}


using ::BPrivate::FileCloser;
using ::BPrivate::DirCloser;
using ::BPrivate::AttrDirCloser;


#endif	// _AUTO_DELETER_POSIX_H
