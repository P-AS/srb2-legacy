// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.c
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#include "doomdef.h"
#include "doomstat.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_clisrv.h"
#include "f_finale.h"


// --------------------------------------------
// assembly or c drawer routines for 8bpp/16bpp
// --------------------------------------------
void (*wallcolfunc)(void); // new wall column drawer to draw posts >128 high
void (*colfunc)(void); // standard column, up to 128 high posts

void (*basecolfunc)(void);
void (*fuzzcolfunc)(void); // standard fuzzy effect column drawer
void (*transcolfunc)(void); // translation column drawer
void (*shadecolfunc)(void); // smokie test..
void (*spanfunc)(void); // span drawer, use a 64x64 tile
void (*splatfunc)(void); // span drawer w/ transparency
void (*basespanfunc)(void); // default span func for color mode
void (*transtransfunc)(void); // translucent translated column drawer
void (*twosmultipatchfunc)(void); // for cols with transparent pixels
void (*twosmultipatchtransfunc)(void); // for cols with transparent pixels AND translucency

// ------------------
// global video state
// ------------------
viddef_t vid;
INT32 setmodeneeded; //video mode change needed if > 0 (the mode number to set + 1)

static CV_PossibleValue_t scr_depth_cons_t[] = {{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}, {0, NULL}};

//added : 03-02-98: default screen mode, as loaded/saved in config
#ifdef WII
consvar_t cv_scr_width = {"scr_width", "640", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_scr_height = {"scr_height", "480", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_scr_depth = {"scr_depth", "16 bits", CV_SAVE, scr_depth_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_scr_width = {"scr_width", "1280", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_scr_height = {"scr_height", "800", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_scr_depth = {"scr_depth", "16 bits", CV_SAVE, scr_depth_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
consvar_t cv_renderview = {"renderview", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void SCR_ChangeFullscreen (void);

consvar_t cv_fullscreen = {"fullscreen", "Yes", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen, 0, NULL, NULL, 0, 0, NULL};

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

INT32 scr_bpp; // current video mode bytes per pixel
UINT8 *scr_borderpatch; // flat used to fill the reduced view borders set at ST_Init()

// =========================================================================

//  Short and Tall sky drawer, for the current color mode
void (*walldrawerfunc)(void);

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!setmodeneeded || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	VID_SetMode(--setmodeneeded);

	V_SetPalette(0);

	//
	//  setup the right draw routines for either 8bpp or 16bpp
	//
	if (true)//vid.bpp == 1) //Always run in 8bpp. todo: remove all 16bpp code?
	{
		spanfunc = basespanfunc = R_DrawSpan_8;
		splatfunc = R_DrawSplat_8;
		transcolfunc = R_DrawTranslatedColumn_8;
		transtransfunc = R_DrawTranslatedTranslucentColumn_8;

		colfunc = basecolfunc = R_DrawColumn_8;
		shadecolfunc = R_DrawShadeColumn_8;
		fuzzcolfunc = R_DrawTranslucentColumn_8;
		walldrawerfunc = R_DrawWallColumn_8;
		twosmultipatchfunc = R_Draw2sMultiPatchColumn_8;
		twosmultipatchtransfunc = R_Draw2sMultiPatchTranslucentColumn_8;
	}
/*	else if (vid.bpp > 1)
	{
		I_OutputMsg("using highcolor mode\n");
		spanfunc = basespanfunc = R_DrawSpan_16;
		transcolfunc = R_DrawTranslatedColumn_16;
		transtransfunc = R_DrawTranslucentColumn_16; // No 16bit operation for this function

		colfunc = basecolfunc = R_DrawColumn_16;
		shadecolfunc = NULL; // detect error if used somewhere..
		fuzzcolfunc = R_DrawTranslucentColumn_16;
		walldrawerfunc = R_DrawWallColumn_16;
	}*/
	else
		I_Error("unknown bytes per pixel mode %d\n", vid.bpp);
/*#if !defined (DC) && !defined (WII)
	if (SCR_IsAspectCorrect(vid.width, vid.height))
		CONS_Alert(CONS_WARNING, M_GetText("Resolution is not aspect-correct!\nUse a multiple of %dx%d\n"), BASEVIDWIDTH, BASEVIDHEIGHT);
#endif*/
	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
}

// do some initial settings for the game loading screen
//
void SCR_Startup(void)
{
	if (dedicated)
	{
		V_Init();
		V_SetPalette(0);
		return;
	}

	vid.modenum = 0;

	vid.dupx = vid.width / BASEVIDWIDTH;
	vid.dupy = vid.height / BASEVIDHEIGHT;
	vid.dupx = vid.dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	vid.fdupx = FixedDiv(vid.width*FRACUNIT, BASEVIDWIDTH*FRACUNIT);
	vid.fdupy = FixedDiv(vid.height*FRACUNIT, BASEVIDHEIGHT*FRACUNIT);

#ifdef HWRENDER
	if (rendermode != render_opengl && rendermode != render_none) // This was just placing it incorrectly at non aspect correct resolutions in opengl
#endif
		vid.fdupx = vid.fdupy = (vid.fdupx < vid.fdupy ? vid.fdupx : vid.fdupy);

	vid.meddupx = (UINT8)(vid.dupx >> 1) + 1;
	vid.meddupy = (UINT8)(vid.dupy >> 1) + 1;
#ifdef HWRENDER
	vid.fmeddupx = vid.meddupx*FRACUNIT;
	vid.fmeddupy = vid.meddupy*FRACUNIT;
#endif

	vid.smalldupx = (UINT8)(vid.dupx / 3) + 1;
	vid.smalldupy = (UINT8)(vid.dupy / 3) + 1;
#ifdef HWRENDER
	vid.fsmalldupx = vid.smalldupx*FRACUNIT;
	vid.fsmalldupy = vid.smalldupy*FRACUNIT;
#endif

	vid.baseratio = FRACUNIT;

	V_Init();
	CV_RegisterVar(&cv_ticrate);
	CV_RegisterVar(&cv_tpscounter);
	CV_RegisterVar(&cv_constextsize);

	V_SetPalette(0);
}

// Called at new frame, if the video mode has changed
//
void SCR_Recalc(void)
{
	if (dedicated)
		return;

	// bytes per pixel quick access
	scr_bpp = vid.bpp;

	// scale 1,2,3 times in x and y the patches for the menus and overlays...
	// calculated once and for all, used by routines in v_video.c
	vid.dupx = vid.width / BASEVIDWIDTH;
	vid.dupy = vid.height / BASEVIDHEIGHT;
	vid.dupx = vid.dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	vid.fdupx = FixedDiv(vid.width*FRACUNIT, BASEVIDWIDTH*FRACUNIT);
	vid.fdupy = FixedDiv(vid.height*FRACUNIT, BASEVIDHEIGHT*FRACUNIT);

#ifdef HWRENDER
	//if (rendermode != render_opengl && rendermode != render_none) // This was just placing it incorrectly at non aspect correct resolutions in opengl
	// 13/11/18:
	// The above is no longer necessary, since we want OpenGL to be just like software now
	// -- Monster Iestyn
#endif
		vid.fdupx = vid.fdupy = (vid.fdupx < vid.fdupy ? vid.fdupx : vid.fdupy);

	//vid.baseratio = FixedDiv(vid.height << FRACBITS, BASEVIDHEIGHT << FRACBITS);
	vid.baseratio = FRACUNIT;

	vid.meddupx = (UINT8)(vid.dupx >> 1) + 1;
	vid.meddupy = (UINT8)(vid.dupy >> 1) + 1;
#ifdef HWRENDER
	vid.fmeddupx = vid.meddupx*FRACUNIT;
	vid.fmeddupy = vid.meddupy*FRACUNIT;
#endif

	vid.smalldupx = (UINT8)(vid.dupx / 3) + 1;
	vid.smalldupy = (UINT8)(vid.dupy / 3) + 1;
#ifdef HWRENDER
	vid.fsmalldupx = vid.smalldupx*FRACUNIT;
	vid.fsmalldupy = vid.smalldupy*FRACUNIT;
#endif

	// toggle off automap because some screensize-dependent values will
	// be calculated next time the automap is activated.
	if (automapactive)
		AM_Stop();

	// set the screen[x] ptrs on the new vidbuffers
	V_Init();

	// scr_viewsize doesn't change, neither detailLevel, but the pixels
	// per screenblock is different now, since we've changed resolution.
	R_SetViewSize(); //just set setsizeneeded true now ..

	// vid.recalc lasts only for the next refresh...
	con_recalc = true;
	am_recalc = true;
}

// Check for screen cmd-line parms: to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode(void)
{
	INT32 scr_forcex, scr_forcey; // resolution asked from the cmd-line

	if (dedicated)
		return;

	// 0 means not set at the cmd-line
	scr_forcex = scr_forcey = 0;

	if (M_CheckParm("-width") && M_IsNextParm())
		scr_forcex = atoi(M_GetNextParm());

	if (M_CheckParm("-height") && M_IsNextParm())
		scr_forcey = atoi(M_GetNextParm());

	if (scr_forcex && scr_forcey)
	{
		CONS_Printf(M_GetText("Using resolution: %d x %d\n"), scr_forcex, scr_forcey);
		// returns -1 if not found, thus will be 0 (no mode change) if not found
		setmodeneeded = VID_GetModeForSize(scr_forcex, scr_forcey) + 1;
	}
	else
	{
		CONS_Printf(M_GetText("Default resolution: %d x %d (%d bits)\n"), cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		// see note above
		setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
	}
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	// remember the default screen size
	CV_SetValue(&cv_scr_width, vid.width);
	CV_SetValue(&cv_scr_height, vid.height);
	CV_SetValue(&cv_scr_depth, vid.bpp*8);
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen(void)
{
#ifdef DIRECTFULLSCREEN
	// allow_fullscreen is set by VID_PrepareModeList
	// it is used to prevent switching to fullscreen during startup
	if (!allow_fullscreen)
		return;

	if (graphics_started)
	{
		VID_PrepareModeList();
		setmodeneeded = VID_GetModeForSize(vid.width, vid.height) + 1;
	}
	return;
#endif
}

boolean SCR_IsAspectCorrect(INT32 width, INT32 height)
{
	return
	 (  width % BASEVIDWIDTH == 0
	 && height % BASEVIDHEIGHT == 0
	 && width / BASEVIDWIDTH == height / BASEVIDHEIGHT
	 );
}

// XMOD FPS display
// moved out of os-specific code for consistency
double averageFPS = 0.0f;

#define FPS_SAMPLE_RATE (50000) // How often to update FPS samples, in microseconds
#define NUM_FPS_SAMPLES 16 // Number of samples to store

static double fps_samples[NUM_FPS_SAMPLES];

void SCR_CalculateFPS(void)
{
	static boolean init = false;

	static precise_t startTime = 0;
	precise_t endTime = 0;

	static precise_t updateTime = 0;
	int updateElapsed = 0;
	int i;

	endTime = I_GetPreciseTime();

	if (init == false)
	{
		startTime = updateTime = endTime;
		init = true;
		return;
	}

	updateElapsed = I_PreciseToMicros(endTime - updateTime);

	if (updateElapsed >= FPS_SAMPLE_RATE)
	{
		static int sampleIndex = 0;
		int frameElapsed = I_PreciseToMicros(endTime - startTime);

		fps_samples[sampleIndex] = frameElapsed / 1000.0f;

		sampleIndex++;
		if (sampleIndex >= NUM_FPS_SAMPLES)
			sampleIndex = 0;

		averageFPS = 0.0f;
		for (i = 0; i < NUM_FPS_SAMPLES; i++)
		{
			averageFPS += fps_samples[i];
		}
		averageFPS = 1000.0f / (averageFPS / NUM_FPS_SAMPLES);

		updateTime = endTime;
	}

	startTime = endTime;
}


void SCR_DisplayTicRate(void)
{
	INT32 ticcntcolor = 0;
	const INT32 h = vid.height-(8*vid.dupy);
	UINT32 cap = R_GetFramerateCap();
	double fps = ceil(averageFPS);

	if (gamestate == GS_NULL)
		return;

	if (cap > 0)
	{
		if (fps <= cap / 2.0) ticcntcolor = V_REDMAP;
		else if (fps <= cap * 0.90) ticcntcolor = V_YELLOWMAP;
		else ticcntcolor = V_GREENMAP;
	}
	else
	{
		ticcntcolor = V_GREENMAP;
	}

	if (cv_ticrate.value == 2) // compact counter
		V_DrawString(vid.width-(32*vid.dupx), h,
			ticcntcolor|V_NOSCALESTART, va("%04.0f", fps));
	else if (cv_ticrate.value == 1) // full counter
	{

		if (cap > 0)
		{
			V_DrawString(vid.width-(88*vid.dupx), h,
				V_YELLOWMAP|V_NOSCALESTART, "FPS:");
			V_DrawString(vid.width-(56*vid.dupx), h,
				ticcntcolor|V_NOSCALESTART, va("%4.0f/%4u", fps, cap));
		}
		else
		{
			V_DrawString(vid.width-(88*vid.dupx), h,
				V_YELLOWMAP|V_NOSCALESTART, "FPS:");
			V_DrawString(vid.width-(24*vid.dupx), h,
				ticcntcolor|V_NOSCALESTART, va("%4.0f", fps));
		}
	}
}



