/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <sys/types.h>

struct ext_mime {
	char *extension;
	char *mime;
};

struct ext_mime mimes[] = {
	{ "gz", "application/x-gzip" },
	{ "hqx", "application/x-binhex40" },
	{ "lha", "application/x-lharc" },
	{ "pcl", "application/x-pcl" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ "sit", "application/x-stuff-it" },
	{ "tar", "application/x-tar" },
	{ "tgz", "application/x-gzip" },
	{ "uue", "application/x-uuencode" },
	{ "z", "application/x-compress" },
	{ "zip", "application/zip" },
	{ "zoo", "application/x-zoo" },
	{ "rar", "application/x-rar" },
	{ "pkg", "application/x-scode-UPkg" },
	{ "7z", "application/x-7z-compressed" },
	{ "bz2", "application/x-bzip2" },
	{ "xz", "application/x-xz" },
	
	{ "jar", "application/x-jar" },

	{ "aif", "audio/x-aiff" },
	{ "aiff", "audio/x-aiff" },
	{ "au", "audio/basic" },
	{ "mid", "audio/x-midi" },
	{ "midi", "audio/x-midi" },
	{ "mod", "audio/mod" },
	{ "ra", "audio/x-real-audio" },
	{ "wav", "audio/x-wav" },
	{ "mp3", "audio/x-mpeg" },
	{ "ogg", "audio/x-vorbis" },
	{ "flac", "audio/x-flac" },
	{ "wma", "audio/x-ms-wma" },

	{ "avi", "video/x-msvideo" },
	{ "mov", "video/quicktime" },
	{ "qt", "video/quicktime" },
	{ "mpg", "video/mpeg" },
	{ "mpeg", "video/mpeg" },
	{ "flv", "video/x-flv" },
	{ "mp4", "video/mp4" },
	{ "mkv", "video/x-matroska" },	
	{ "asf", "application/x-asf" },
	{ "rm", "video/vnd.rn-realvideo" },
	{ "wmv", "video/x-ms-wmv" },

	{ "bmp", "image/x-bmp" },
	{ "fax", "image/g3fax" },
	{ "gif", "image/gif" },
	{ "iff", "image/x-iff" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "pbm", "image/x-portable-bitmap" },
	{ "pcx", "image/x-pcx" },
	{ "pgm", "image/x-portable-graymap" },
	{ "png", "image/png" },
	{ "ppm", "image/x-portable-pixmap" },
	{ "rgb", "image/x-rgb" },
	{ "tga", "image/x-targa" },
	{ "tif", "image/tiff" },
	{ "tiff", "image/tiff" },
	{ "xbm", "image/x-xbitmap" },
	{ "djvu", "image/x-djvu" },
	{ "svg", "image/svg+xml" },
	{ "ico", "image/vnd.microsoft.icon" },

	{ "doc", "application/msword" },
	{ "xls", "application/vnd.ms-excel" },
	{ "xls", "application/vnd.ms-excel" },
	{ "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
	{ "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
	{ "ppt", "application/vnd.ms-powerpoint" },
	{ "pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
	{ "chm", "application/x-chm" },
	
	{ "txt", "text/plain" },
	{ "xml", "text/plain" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "rtf", "text/rtf" },
	{ "c", "text/x-source-code" },
	{ "cc", "text/x-source-code" },
	{ "c++", "text/x-source-code" },
	{ "h", "text/x-source-code" },
	{ "hh", "text/x-source-code" },
	{ "hpp", "text/x-source-code" },
	{ "cxx", "text/x-source-code" },
	{ "cpp", "text/x-source-code" },
	{ "S", "text/x-source-code" },
	{ "java", "text/x-source-code" },
	{ "ini", "text/plain" },
	{ "inf", "text/plain" },
		
	{ "ttf", "application/x-truetype" },

	{ NULL, NULL }
};
