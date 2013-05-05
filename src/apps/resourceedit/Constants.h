/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H


#define MSG_NEW				'm000'
#define MSG_OPEN			'm001'
#define MSG_OPEN_DONE		'm002'
#define MSG_CLOSE			'm003'
#define MSG_SAVE			'm004'
#define MSG_SAVEAS			'm005'
#define MSG_SAVEAS_DONE		'm006'
#define MSG_SAVEALL			'm007'
#define MSG_MERGEWITH		'm008'
#define MSG_QUIT			'm009'

#define MSG_UNDO			'm010'
#define MSG_REDO			'm011'
#define MSG_CUT				'm012'
#define MSG_COPY			'm013'
#define MSG_PASTE			'm014'
#define MSG_CLEAR			'm015'
#define MSG_SELECTALL		'm016'

#define MSG_ADDAPPRES		'm020'
#define MSG_SETTINGS		'm021'

#define MSG_ADD				'm030'
#define	MSG_REMOVE			'm031'
#define MSG_MOVEUP			'm032'
#define MSG_MOVEDOWN		'm033'

#define MSG_SELECTION		'm040'
#define MSG_INVOCATION		'm041'

#define MSG_ACCEPT			'm050'
#define MSG_CANCEL			'm051'
#define MSG_IGNORE			'm052'

#define MSG_SETTINGS_APPLY	'm022'
#define MSG_SETTINGS_REVERT	'm023'
#define MSG_SETTINGS_CLOSED	'm024'

// TODO: Remove prior to release.
#define DEBUG 1
#include <Debug.h>
// --- ---


#endif
