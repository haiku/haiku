/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_R5_COMPATIBILITY_H
#define NET_R5_COMPATIBILITY_H

// TODO: judging from R5 headers, this is all wrong, though (this has been taken from
//		our old socket header) - we'll need to test this later on!
// 		Also, more problematic is that IPPROTO_* constants also don't seem to be compatible, too
#define R5_SOCK_DGRAM	10
#define R5_SOCK_STREAM	11
#define R5_SOCK_RAW		12

#endif	// NET_R5_COMPATIBILITY_H
