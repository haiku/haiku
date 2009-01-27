/*
 * Copyright 2005-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _WINDOW_INFO_H_
#define _WINDOW_INFO_H_


#include <SupportDefs.h>


struct window_info {
	team_id		team;
    int32   	server_token;

	int32		thread;
	int32		client_token;
	int32		client_port;
	uint32		workspaces;

	int32		layer;	// see ServerWindow::GetInfo()
    uint32	  	feel;
    uint32      flags;
	int32		window_left;
	int32		window_top;
	int32		window_right;
	int32		window_bottom;

	int32		show_hide_level;
	bool		is_mini;
} _PACKED;

struct client_window_info : window_info {
	float		tab_height;
	float		border_size;
	char		name[1];
} _PACKED;

enum window_action {
	B_MINIMIZE_WINDOW,
	B_BRING_TO_FRONT
};


// Private BeOS compatible window API

void do_window_action(int32 window_id, int32 action, BRect zoomRect, bool zoom);
client_window_info* get_window_info(int32 token);
int32* get_token_list(team_id app, int32 *count);
void do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom);
void do_minimize_team(BRect zoomRect, team_id app, bool zoom);

// Haiku additions

namespace BPrivate {

status_t get_application_order(int32 workspace, team_id** _apps, int32* _count);
status_t get_window_order(int32 workspace, int32** _tokens, int32* _count);

}	// namespace BPrivate


#endif	// _WINDOW_INFO_H_
