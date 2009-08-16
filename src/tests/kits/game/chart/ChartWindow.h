/*
	
	ChartWindow.h
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef CHART_WINDOW_H
#define CHART_WINDOW_H

#include <DirectWindow.h>
#include <OS.h>
#include <Locker.h>
#include <StringView.h>
#include <PictureButton.h>

#include "ChartRender.h"
#include "ChartView.h"

/* This window can be used in 3 modes : window mode is just a
   normal window (that you move and resize freely), fullscreen
   resize the window to adjust its content area to the full
   size of the screen. Demo mode resize the window to have the
   animate part of it fit the full screen. */
enum {
	WINDOW_MODE		= 0,
	FULLSCREEN_MODE	= 1,
	FULLDEMO_MODE	= 2
};

/* Special animation mode. Comet create 2 comets flying around
   randomaly. Novas add a couple start with flashing burst of
   light. Battle was never implemented... */
enum {
	SPECIAL_NONE	= 0,
	SPECIAL_COMET	= 1,
	SPECIAL_NOVAS	= 2,
	SPECIAL_BATTLE	= 3
};

/* Three types of display mode : line is supposed to use line
   array to draw pixel (but is not currently implemented). Bitmap
   use an offscreen BBitmap and DrawBitmap for each frame. Direct
   use the DirectWindow API to draw directly on the screen. */
enum {
	DISPLAY_OFF		= 0,
	DISPLAY_LINE	= 1,
	DISPLAY_BITMAP	= 2,
	DISPLAY_DIRECT	= 3
};

/* Five ways of moving the camera around : not at all, simple
   rotation around the center of the starfield, slow straight
   move, fast straight move, and a random move (flying around). */
enum {
	ANIMATION_OFF		= 0,
	ANIMATION_ROTATE	= 1,
	ANIMATION_SLOW_MOVE	= 2,
	ANIMATION_FAST_MOVE = 3,
	ANIMATION_FREE_MOVE = 4
};

/* Three types of star field. A new random starfield is created
   everytime you change the starfield type. The first will just
   put stars randomly in space. The second will concentrate
   star in 10 places. The last one will put half the star in a
   big spiral galaxy, and the other one in a few amas. */
enum {
	SPACE_CHAOS		= 0,
	SPACE_AMAS		= 1,
	SPACE_SPIRAL	= 2
};

/* All messages exchanged between the UI and the engine. */
enum {
	ANIM_OFF_MSG		= 1000,
	ANIM_SLOW_ROT_MSG	= 1001,
	ANIM_SLOW_MOVE_MSG	= 1002,
	ANIM_FAST_MOVE_MSG	= 1003,
	ANIM_FREE_MOVE_MSG	= 1004,
	DISP_OFF_MSG		= 2000,
	DISP_LINE_MSG		= 2001,
	DISP_BITMAP_MSG		= 2002,
	DISP_DIRECT_MSG		= 2003,
	OPEN_COLOR_MSG		= 3000,
	OPEN_DENSITY_MSG	= 3100,
	OPEN_REFRESH_MSG	= 3200,
	SPACE_CHAOS_MSG		= 3300,
	SPACE_AMAS_MSG		= 3301,
	SPACE_SPIRAL_MSG	= 3302,
	FULL_SCREEN_MSG		= 4000,
	AUTO_DEMO_MSG		= 4100,	
	BACK_DEMO_MSG		= 4101,
	SECOND_THREAD_MSG	= 4200,
	COLORS_RED_MSG		= 5000,
	COLORS_GREEN_MSG	= 5001,
	COLORS_BLUE_MSG		= 5002,
	COLORS_YELLOW_MSG	= 5003,
	COLORS_ORANGE_MSG	= 5004,
	COLORS_PINK_MSG		= 5005,
	COLORS_WHITE_MSG	= 5006,
	SPECIAL_NONE_MSG	= 6000,
	SPECIAL_COMET_MSG	= 6001,
	SPECIAL_NOVAS_MSG	= 6002,
	SPECIAL_BATTLE_MSG	= 6003,
	COLOR_PALETTE_MSG	= 7000,
	STAR_DENSITY_MSG	= 8000,
	REFRESH_RATE_MSG	= 9000
};

enum {
	/* Number of star used to generate the special animation */
	SPECIAL_COUNT_MAX	= 512,
	/* Number of keypoint used for the "random" animation */
	KEY_POINT_MAX		= 16
};

/* Basic 3D point/vector class, used for basic vectorial
   operations. */
class TPoint {
public:
	float   	x;
	float   	y;
	float   	z;

	TPoint		operator* (const float k) const;
	TPoint		operator- (const TPoint& v2) const;
	TPoint		operator+ (const TPoint& v2) const;
	TPoint		operator^ (const TPoint& v2) const;
	float		Length() const;
};

/* Basic 3x3 matrix used for 3D transform. */ 
class TMatrix {
public:
	float 		m[3][3];

	TPoint		operator* (const TPoint& v) const;
	TPoint		Axis(int32 index);
	TMatrix		Transpose() const;
	void		Set(const float alpha, const float theta, const float phi);
};


class BBox;
class BView;

/* The main window class, encapsulating both UI and engine. */
class ChartWindow : public BDirectWindow {
public:
		/* standard constructor and destructor */
				ChartWindow(BRect frame, const char *name); 
virtual			~ChartWindow();

		/* standard window members */
virtual	bool	QuitRequested();
virtual	void	MessageReceived(BMessage *message);
virtual void	ScreenChanged(BRect screen_size, color_space depth);
virtual	void	FrameResized(float new_width, float new_height);

		/* this struct embedded all user settable parameters that
		   can be set by the UI. The idea is to solve all possible
		   synchronisation problem between the UI settings and the
		   engine (when using DirectWindow mode) by defining a
		   current setting state and a next setting state. The UI
		   touches only the next setting state, never the one
		   currently use by the engine. This way the engine doesn't
		   have to be synchronised with the UI. */
		struct setting {
			/* do we want to use the second thread ? */
			bool		second_thread;
			/* which ones of the 7 star colors are we using ? */
			bool		colors[7];
			/* what window configuration mode are we using ? */
			int32		fullscreen_mode;
			/* what special mode are we using ? */
			int32		special;
			/* what display mode are we using ? */
			int32		display;
			/* what starfield model are we using ? */
			int32		space_model;
			/* what camera animation are we using ? */
			int32		animation;
			/* what is the density of the current starfield
			   model ? */
			int32		star_density;
			/* what's the current required refresh rate ? */
			float		refresh_rate;
			/* what's the current background color for the animated
			   view ? */
			rgb_color	back_color;
			/* what's the current color_space of the screen ? */
			color_space	depth;
			/* what's the current dimension of the content area of
			   the window ? */
			int32		width, height;

			/* used to copy a whole setting in another. */			
			void		Set(setting *master);
		};

		/* this union is used to store special animation information
		   that are defined per star. */
		typedef union {
			/* for the comet, it's a speed vector and to
			   counters to define how long the star will
			   continue before disappearing. */
			struct {
				float		dx;
				float		dy;
				float		dz;
				int32		count;
				int32		count0;
			} comet;
			/* for the novas, just counters to define the
			   pulse cycle of the star. */
			struct {
				float		count;
				int32		count0;
			} nova;
			/* battle was not implemented. */
			struct {
				int32		count;
			} battle;
		} special;
	
	/* public instance members */
		/* the current instantenuous potential frame/rate
		   as display by the vue-meter. */
		int32			fInstantLoadLevel;
		/* the offscreen Bitmap used, if any. */	
		BBitmap			*fOffscreen;
		/* the current active setting used by the engine */
		setting			fCurrentSettings;



/* the private stuff... */		
private:
	/* User Interface related stuff. */
		BBox		*fStatusBox;
		BBox		*fColorsBox;
		BBox		*fSpecialBox;
		
		BView		*fLeftView;
		BView		*fTopView;

		/* Find a window by its name if already opened. */
static	BWindow		*GetAppWindow(const char *name);
		/* Used to set the content of PictureButton. */
		BPicture	*ButtonPicture(bool active, int32 button_type);
		/* Those function create and handle the other settings
		   floating windows, for the background color, the star
		   density and the refresh rate. */
		void		OpenColorPalette(BPoint here);
		void		OpenStarDensity(BPoint here);
		void		OpenRefresh(BPoint here);
		/* Draw the state of the instant-load vue-meter */
		void		DrawInstantLoad(float frame_per_second);
		/* Print the performances numbers, based on the real framerate. */
		void		PrintStatNumbers(float fps);
		
	/* Engine setting related functions. */
		/* Init the geometry engine of the camera at startup. */
		void		InitGeometry();
		/* Check and apply changes between a new setting state
		   and the currently used one. */
		void		ChangeSetting(setting new_set);
		/* Initialise a new starfield of the specified model. */	
		void		InitStars(int32 model);
		/* Fill a star list with random stars in [0-1]x[0-1]x[0-1]. */
		void		FillStarList(star *list, int32 count);
		/* Init a new special animation of a specific type. */
		void		InitSpecials(int32 code);
		/* Change the star field colors depending a new set of
		   selected colors. */
		void		SetStarColors(int32	*color_list, int32 color_count);
		/* Change the global geometry of the camera when the
		   viewing area is resized. */
		void		SetGeometry(int32 dh, int32 dv);
		/* Change the color_space configuration of a buffer */
		void		SetColorSpace(buffer *buf, color_space depth);
		/* Used to sync the pre-offset matrix pointer whenever the buffer
		   bits pointer changes. */
		void		SetPatternBits(buffer *buf);
	
	/* Engine processing related functions. */
		/* those functions are the two processing threads launcher */
static	long		Animation(void *data);
static	long		Animation2(void *data);
		/* After every camera move or rotation, reprocess the torus
		   cycle of the starfield to maintain the pyramid of vision
		   of the camera completly inside a 1x1x1 iteration of the
		   starfield. */
		void		SetCubeOffset();
		/* Process the camera animation for a specified time step */
		void		CameraAnimation(float time_factor);
		/* Used by the random move camera animation. */
		void		SelectNewTarget();
		void		FollowTarget();
		/* Process the special animation for a specified time step */ 
		void		AnimSpecials(float time_step);
		/* Sync the embedded camera state with the window class camera
		   state (before calling the embedded C-engine in ChartRender.c */
		void		SyncGeo();
		/* Control the star processing (done by 1 or 2 threads) and
		   executed by the embedded C-engine in ChartRender.c */
		void		RefreshStars(buffer *buf, float time_step);
		
	/* Offscreen bitmap configuration related functions. */
		/* Used to update the state of the offscreen Bitmap to stay
		   in sync with the current settings. */
		void		CheckBitmap(color_space depth, int32 width, int32 height);
		/* Set the basic clipping of the offscreen Bitmap (everything
		   visible) */
		void		SetBitmapClipping(int32 width, int32 height);
		/* Draw the offscreen bitmap background. */
		void		SetBitmapBackGround();
		
	/* DirectWindow related functions */	
		/* Process a change of state in the direct access to the
		   frame buffer. */
		void		SwitchContext(direct_buffer_info *info);
public:
		/* this is the hook controling direct screen connection */
virtual void		DirectConnected(direct_buffer_info *info);


private:
	/* Pseudo-random generator state and increment function. */
		int32		fCrcAlea;
inline	void		CrcStep();
		
		
	/* Various private instance variables. */	
		/* the next setting, as modified by the UI */
		setting			fNextSettings;
		

		/* the buffer descriptors for the offscreen bitmap and
		   the DirectWindow buffer. */
		buffer			fBitmapBuffer;
		buffer			fDirectBuffer;
		
		/* current maximal dimensions of the offscreen Bitmap buffer */
		int32			fMaxWidth, fMaxHeight;

		/* memorize previous state for switch between fullscreen
		   and window mode */
		BRect			fPreviousFrame;
		int32			fPreviousFullscreenMode;
		
		/* cycling threshold for the cubic torus starfield, that
		   guarantees that the 1x1x1 sample will contain the full
		   pyramid of vision. */ 
		TPoint			fCut;
		
		/* maximal depth of the pyramid of vision */
		float			fDepthRef;
		
		int32			fBackColorIndex;
		
		/* target frame duration, in microsecond. */
		bigtime_t		fFrameDelay;
		
		/* various UI object that we need to reach directly. */
		BButton			*fOffwindowButton;
		ChartView		*fChartView;
		BStringView		*fCpuLoadView, *fFramesView;
		InstantView		*fInstantLoad;
		BPictureButton	*fColorButton, *fDensityButton, *fRefreshButton;

		/* states used to describe the camera position, rotation
		   and dynamic (in other case than free move). */
		float			fCameraAlpha, fCameraTheta, fCameraPhi;
		float			fDynamicAlpha;
		float			fDynamicTheta;
		float			fDynamicPhi;
		int32			fCountAlpha, fCountTheta, fCountPhi;
		TPoint			fOrigin;
		TMatrix			fCamera;
		TMatrix			fCameraInvert;

		/* the camera geometry descriptor required by the embedded
		   C-engine (just a copy of part of the previous states) */
		geometry		fGeometry;
		
		/* states used by the free move camera animation */
		int32			fTrackingTarget;
		int32			fKeyPointCount;
		float			fSpeed, fTargetSpeed;
		float			fLastDynamicDelay;
		TPoint			fKeyPoints[KEY_POINT_MAX];
	
		/* main starfield. */
		star_packet		fStars;

		/* used for the special animation */
		TPoint			fComet[2];
		TPoint			fDeltaComet[2];
		special			*fSpecialList;
		star_packet		fSpecials;
		
		/* the two processing threads. */
		thread_id		fAnimationThread;
		thread_id		fSecondAnimationThread;
		
		/* context of the second processing thread (when used). */
		float			fSecondThreadThreshold;
		buffer			*fSecondThreadBuffer;
		sem_id			fSecondThreadLock;
		sem_id			fSecondThreadRelease;
		bigtime_t			fSecondThreadDelay;
		star_packet		fStars2;
		star_packet		fSpecials2;
		
		/* Flag used to terminate the processing threads */
		bool			fKillThread;
		
		/* Direct connection status */
		bool			fDirectConnected;
		
		/* used to synchronise the star animation drawing. */
		sem_id			fDrawingLock;
};

#endif
