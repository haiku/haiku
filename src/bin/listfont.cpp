/*
 * listfont
 * (c) 2004, François Revol.
 */

#include <stdio.h>
#include <Application.h>
#include <Font.h>
#include <String.h>

int32 th(void *)
{
	be_app->Lock();
	be_app->Run();
	return B_OK;
}

int usage(int ret)
{
	printf("listfont [-s] [-l]\n");
	printf("lists currently installed font families.\n");
	printf("\t-s list styles for each family\n");
	printf("\t-l long listing with more info\n");
	printf("\t-u update font families\n");
	return ret;
}

int main(int argc, char **argv)
{
	int32 family_count, f;
	font_family family;
	int32 style_count, s;
	font_style style;
	uint16 face;
	uint32 flags;
	int i;
	bool display_styles = false;
	bool display_long = false;
	bool update_families = false;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-u"))
			update_families = true;
		else if (!strcmp(argv[i], "-s"))
			display_styles = true;
		else if (!strcmp(argv[i], "-l")) {
			display_long = true;
			display_styles = true;
		} else
			return usage(1);
	}
	BApplication app("application/x-vnd.mmu_man.listfont");
	resume_thread(spawn_thread(th, "be_app", B_NORMAL_PRIORITY, NULL));
	be_app->Unlock();
	if (update_families) {
		bool changed = update_font_families(true);
		printf("font families %schanged.\n", changed?"":"didn't ");
		return 0;
	}
	family_count = count_font_families();
	for (f = 0; f < family_count; f++) {
		if (get_font_family(f, &family) < B_OK)
			continue;
		if (!display_styles) {
			printf("%s\n", family);
			continue;
		}
		style_count = count_font_styles(family);
		for (s = 0; s < style_count; s++) {
			if (get_font_style(family, s, &style, &face, &flags) < B_OK)
				continue;
			if (!display_long) {
				printf("%s/%s\n", family, style);
				continue;
			}
			BString fname;
			fname << family << "/" << style;
			printf("%-30s", fname.String());
			BFont fnt;
			fnt.SetFamilyAndStyle(family, style);
			//fnt.PrintToStream();
			printf("\tface=%08x ", face);
			font_height fh;
			fnt.GetHeight(&fh);
			printf("\theight={%f %f %f} ", fh.ascent, fh.descent, fh.leading);
			printf("\t%s", (flags & B_IS_FIXED)?"fixed ":"");
#ifndef B_BEOS_VERSION_DANO
			/* seems R5 is broken there :-( locks up on 'a> recv' */
			printf("\t%s", (flags & B_HAS_TUNED_FONT)?"hastuned ":"");
#else
			if (flags & B_HAS_TUNED_FONT) {
				int32 tunedcount = fnt.CountTuned();
				printf("%ld tuned: ", tunedcount);
//puts("");
#if 1
				for (i = 0; i < tunedcount; i++) {
					tuned_font_info tfi;
					fnt.GetTunedInfo(i, &tfi);
					printf("{%f, %f, %f, %08lx, %x} ", tfi.size, tfi.shear, tfi.rotation, tfi.flags, tfi.face);
//puts("");
				}
#endif
			}
#endif
			//printf("\tFlags=%08lx ", fnt.Flags());
			printf("\tspacing=%d ", fnt.Spacing());
			printf("\tencoding=%d ", fnt.Encoding());
			puts("");
		}
	}
	be_app->Lock();
	be_app->Quit();
	return 0;
}
