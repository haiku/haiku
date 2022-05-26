/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITIES_H
#define UTILITIES_H


#include <Font.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>


class BDeskbar;
class BBitmap;
class BWindow;
struct entry_ref;

typedef struct {
	::team_info		team_info;
	BBitmap*		team_icon;
	char			team_name[B_PATH_NAME_LENGTH];
	::thread_info*	thread_info;
} info_pack;

bool get_team_name_and_icon(info_pack& infoPack, bool icon = false);
bool launch(const char* mime, const char* path);
void mix_colors(rgb_color& target, rgb_color& first, rgb_color& second, float mix);
void find_self(entry_ref& ref);
void move_to_deskbar(BDeskbar& deskbar);
void make_window_visible(BWindow* window, bool mayResize = false);

BRect bar_rect(BRect& frame, BFont* font);

extern const uchar k_cpu_mini[];

#endif // UTILITIES_H
