/*******************************************************
*   SecondDriver
*   
*   Helper methods for SecondDriver.  This is where we 
*    put all the code that inits the real HW and 
*    seaches for the acceleration methods an all that
*    other stuff.
*
*   @author YNOP (ynop@beunited.org)
*******************************************************/
#ifndef _SECDRIVER_HELPER_H_
#define _SECDRIVER_HELPER_H_

//#include <GraphicsCard.h>
#include <graphic_driver.h>
#include <FindDirectory.h>

#include <dirent.h>
#include <string.h>
#include <malloc.h> // has ioctl ?:P
//#include <errno.h>
#include <stdlib.h>

#define DEV_GRAPHICS "/dev/graphics/"
#define ACCELERANTS_DIR "/accelerants/"
#define DEV_STUB "stub"

/*******************************************************
*   @description Loads the driver from settings ...
*******************************************************/
int get_device(){
// 102b_0519_001300  102b_051a_001400  stub 
  return open(DEV_GRAPHICS"102b_051a_001400", B_READ_WRITE);
//  return -1;
}

/*******************************************************
*   @description Loads the stub driver - never fun
*******************************************************/
int fallback_device(){
   return open(DEV_GRAPHICS DEV_STUB, B_READ_WRITE);
}

/*******************************************************
*   @description Find the first device that it can open
*    in the /dev/graphics dir and returns it.
*******************************************************/
int find_device(){
   DIR *d;
   struct dirent *e;
   char name_buf[1024];
   int fd = -1;
   //bool foundfirst = false; // hacky way of finding second video card in your box
   
   printf("Probeing %s for device...\n",DEV_GRAPHICS);
   
   /* open directory apath */
   d = opendir(DEV_GRAPHICS);
   if(!d){ return B_ERROR; }
   while((e = readdir(d)) != NULL){
      if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, "stub")){
         continue;
      }
      strcpy(name_buf, DEV_GRAPHICS);
//      strcat(name_buf, "/");
      strcat(name_buf, e->d_name);
      fd = open(name_buf, B_READ_WRITE);
      ///printf("Opening device %s\n",name_buf);
      if(fd >= 0){ 
         //printf("Open wend ok\n");
         /*if(!foundfirst){
            foundfirst = true;
            printf("\tbut we are not going to use it as app_server is\n");
         }else{
            // found it :)
         */   printf("\tOpened device %s\n",name_buf);
            closedir(d);
            return fd;
         //}
      }else{
         //foundfirst = true; // this is a proper find as the app_server should let us open the dev
         printf("\tCouldn't open device %s\n",name_buf);
      }
   }
   closedir(d);
   return B_ERROR;
}

/*******************************************************
*   @description Finds and loads the Accelerant for
*    this device
*******************************************************/
image_id load_accelerant(int fd, GetAccelerantHook *hook) {
    status_t result;
	image_id image = -1;
	char
		signature[1024],
		path[PATH_MAX];
	struct stat st;
	const static directory_which vols[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	/* get signature from driver */
	result = ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature));
	if (result != B_OK){
	   printf("Failed to get accelerant signature from drived!\n");
	   return result;
	}
	
	printf("Searching for Accelerant named \"%s\"\n",signature);
   
	// note failure by default
	for(int32 i=0; i < (int32)(sizeof (vols) / sizeof (vols[0])); i++) {
		//printf("attempting to get path for %ld (%d)\n", i, vols[i]);
		if(find_directory(vols[i], -1, false, path, PATH_MAX) != B_OK) {
			printf("warning: find directory failed (continueing)\n");
			continue;
		}

		strcat (path, ACCELERANTS_DIR);
		strcat (path, signature);
		printf("\tTrying %s\n",path);
        
		// don't try to load non-existant files
		if (stat(path, &st) != 0) continue;
		
		// try and load it up..
		image = load_add_on(path);
		if (image >= 0) {
			printf("\tAccelerant loaded - trying to init\n");
			// get entrypoint from accelerant
			result = get_image_symbol(image, B_ACCELERANT_ENTRY_POINT,
#if defined(__INTEL__)
				B_SYMBOL_TYPE_ANY,
#else
				B_SYMBOL_TYPE_TEXT,
#endif
				(void **)hook);
			if (result == B_OK) {
				init_accelerant ia;
				ia = (init_accelerant)(*hook)(B_INIT_ACCELERANT, NULL);
				if(ia && ((result = ia(fd)) == B_OK)) {
					// we have a winner!
					//printf("Accelerant %s \n", path);
					printf("\tAccelerant Init() was B_OK\n");
					break;
				} else {
					printf("\tAccelerant Init() failed: %ld\n", result);
				}
			} else {
				printf("\tCouldn't find the entry point :-(\n");
			}
			// unload the accelerant, as we must be able to init!
			unload_add_on(image);
		}
		if (image < 0) printf("\tLoad Image failure. ID: %.8lx (%s)\n", image, strerror(image));
		// mark failure to load image
		image = -1;
	}

	//printf("Add-on image id: %ld\n", image);
	return image;
}

/*******************************************************
*   @description
*******************************************************/
static const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32);
		s2s(B_RGBA32);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15);
		s2s(B_RGBA15);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YCbCr422);
		s2s(B_YCbCr420);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

/*******************************************************
*   @description
*******************************************************/
void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	printf("  pixel_clock: %ldKHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh rate: %4.2f\n", ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total));
	printf("  color space: %s\n", spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags:");
	if (dm->flags & B_SCROLL) printf(" B_SCROLL");
	if (dm->flags & B_8_BIT_DAC) printf(" B_8_BIT_DAC");
	if (dm->flags & B_HARDWARE_CURSOR) printf(" B_HARDWARE_CURSOR");
	if (dm->flags & B_PARALLEL_ACCESS) printf(" B_PARALLEL_ACCESS");
//	if (dm->flags & B_SUPPORTS_OVERLAYS) printf(" B_SUPPORTS_OVERLAYS");
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

/*******************************************************
*   @description
*******************************************************/
void simple_dump_mode(display_mode *dm){
   //printf("H: %4d\n", t->h_display);
   //printf("V: %4d\n", t->v_display);
   display_timing *t = &(dm->timing);
   printf("Virtual size: %d x %d\n", dm->virtual_width, dm->virtual_height);
   printf("Color space: %s\n", spaceToString(dm->space));
   printf("Refresh rate: %4.2f\n", ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total));
}

/*******************************************************
*   @description
*******************************************************/
void mode_from_settings(display_mode *dm){
 //  display_timing *t = &(dm->timing);
   dm->space = B_RGB32;
   dm->virtual_width = 800;
   dm->virtual_height = 600;
   dm->h_display_start = 0;
   dm->v_display_start = 0;
}

/*******************************************************
*   @description
*******************************************************/
status_t get_and_set_mode(GetAccelerantHook gah, display_mode *dm) {
   printf("Setting up display mode\n");
	accelerant_mode_count gmc;
	uint32 mode_count;
	get_mode_list gml;
	display_mode *mode_list, target, high, low;
	propose_display_mode pdm;
	status_t result = B_ERROR;
	set_display_mode sdm;

	/* find the propose mode hook */
	pdm = (propose_display_mode)gah(B_PROPOSE_DISPLAY_MODE, NULL);
	if (!pdm) {
		printf("No B_PROPOSE_DISPLAY_MODE\n");
		goto exit0;
	}
	/* and the set mode hook */
	sdm = (set_display_mode)gah(B_SET_DISPLAY_MODE, NULL);
	if (!sdm) {
		printf("No B_SET_DISPLAY_MODE\n");
		goto exit0;
	}

	/* how many modes does the driver support */
	gmc = (accelerant_mode_count)gah(B_ACCELERANT_MODE_COUNT, NULL);
	if (!gmc) {
		printf("No B_ACCELERANT_MODE_COUNT\n");
		goto exit0;
	}
	mode_count = gmc();
	printf("\tDriver supports %lu differnat video modes\n", mode_count);
	if (mode_count == 0) goto exit0;

	/* get a list of graphics modes from the driver */
	gml = (get_mode_list)gah(B_GET_MODE_LIST, NULL);
	if (!gml) {
		printf("No B_GET_MODE_LIST\n");
		goto exit0;
	}
	mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
	if (!mode_list) {
		printf("Couldn't calloc() for mode list\n");
		goto exit0;
	}
	if (gml(mode_list) != B_OK) {
		printf("mode list retrieval failed\n");
		goto free_mode_list;
	}

	/* take the first mode in the list */
	//printf("\tPropose Display Mode\n------------------------\n");
	//simple_dump_mode(&mode_list[69]);
	//mode_from_settings(&mode_list[69]);
	target = mode_list[69];
	high = mode_list[69];
	low = mode_list[69];
	/* make as tall a virtual height as possible */
	//target.virtual_height = high.virtual_height = 0xffff;
	// virtual hight should only be used if a res larger than
	// the screen can handle
	/* propose the display mode */
	if (pdm(&target, &low, &high) == B_ERROR) {
		printf("propose_display_mode failed\n");
		goto free_mode_list;
	}
	printf("------Target display mode------\n");
	simple_dump_mode(&target);
	printf("-------------------------------\n");
	/* we got a display mode, now set it */
	if (sdm(&target) == B_ERROR) {
		printf("set display mode failed\n");
		goto free_mode_list;
	}
	/* note the mode and success */
	*dm = target;
	result = B_OK;
	
free_mode_list:
	free(mode_list);
exit0:
	return result;
}


/*******************************************************
*   @description
*******************************************************/
void get_frame_buffer(GetAccelerantHook gah, frame_buffer_config *fbc) {
	get_frame_buffer_config gfbc;
	gfbc = (get_frame_buffer_config)gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
	gfbc(fbc);
}

/*******************************************************
*   @description
*******************************************************/
sem_id get_sem(GetAccelerantHook gah) {
	accelerant_retrace_semaphore ars;
	ars = (accelerant_retrace_semaphore)gah(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	return ars();
}

/*******************************************************
*   @description
*******************************************************/
void set_palette(GetAccelerantHook gah) {
	set_indexed_colors sic;
	sic = (set_indexed_colors)gah(B_SET_INDEXED_COLORS, NULL);
	if (sic) {
		/* booring grey ramp for now */
		uint8 map[3 * 256];
		uint8 *p = map;
		int i;
		for (i = 0; i < 256; i++) {
			*p++ = i;
			*p++ = i;
			*p++ = i;
		}
		sic(256, 0, map, 0);
	}
}

// This is a ll the stuff needed to stroke a Bezier Curve..
// Note that we should probably wait and alloc this data latter
// on when we stroke our first Curve. That way if no curve ever
// gets render (unlikely i know) then this space would be saved.
#define SEGMENTS 32
#define Fixed float
static Fixed weight1[SEGMENTS + 1];
static Fixed weight2[SEGMENTS + 1];
#define w1(s)  weight1[s]
#define w2(s)  weight2[s]
#define w3(s)  weight2[SEGMENTS - s]
#define w4(s)  weight1[SEGMENTS - s]
#define FixMul(a,b) (a)*(b)
#define FixRound(a) ((a-int32(a))>=.5)?(int32(a)+1):(int32(a))
#define FixRatio(a,b) ((a)/float(b))

/*******************************************************
*   @description Creats the two Bernstien Pollynomials
*    We only need to creat two becase the 1 & 4 are 
*    mirrors and the 2 & 3 are mirros. 
*******************************************************/
void SetupBezier(){
   Fixed t, zero, one;
   int s;
   zero = FixRatio(0, 1);
   one = FixRatio(1, 1);
   
   weight1[0] = one;
   weight2[0] = zero;
   for(s = 1 ;s < SEGMENTS ;++s){
      t = FixRatio(s, SEGMENTS);
      weight1[s] = FixMul(one - t, FixMul(one - t, one - t));
      weight2[s] = 3 * FixMul(t, FixMul(t - one, t - one));
   }
   weight1[SEGMENTS] = zero;
   weight2[SEGMENTS] = zero;
}

/*******************************************************
*   @description Useing the weights of the 4 bernstein 
*    pollynomials calc the segment endpoints. This is 
*    the real worker method
*******************************************************/
static void computeSegments(BPoint p1,BPoint p2, BPoint p3, BPoint p4, BPoint *segment){ 
   int s;
   segment[0] = p1;
   for(s = 1;s < SEGMENTS;++s){
      segment[s].y = FixRound(w1(s) * p1.y + w2(s) * p2.y +  w3(s) * p3.y + w4(s) * p4.y);
      segment[s].x = FixRound(w1(s) * p1.x + w2(s) * p2.x +  w3(s) * p3.x + w4(s) * p4.x);
   }
   segment[SEGMENTS] = p4;
}

#endif

