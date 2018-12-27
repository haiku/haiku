/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H


class AppManager;
class NodeManager;
class BufferManager;
class NotificationManager;
class MediaFilesManager;
class AddOnManager;

extern AppManager* gAppManager;
extern NodeManager* gNodeManager;
extern BufferManager* gBufferManager;
extern NotificationManager* gNotificationManager;
extern MediaFilesManager* gMediaFilesManager;
extern AddOnManager* gAddOnManager;

#endif	// MEDIA_SERVER_H
