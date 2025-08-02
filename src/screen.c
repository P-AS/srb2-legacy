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
#include "i_time.h"
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
UINT8 setrenderneeded = 0;

static CV_PossibleValue_t scr_depth_cons_t[] = {{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}, {0, NULL}};

//added : 03-02-98: default screen mode, as loaded/saved in config
consvar_t cv_scr_width = CVAR_INIT ("scr_width", "1280", NULL, CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height = CVAR_INIT ("scr_height", "800", NULL,  CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_width_w = CVAR_INIT ("scr_width_w", "640", NULL, CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height_w = CVAR_INIT ("scr_height_w", "400", NULL, CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_depth = CVAR_INIT ("scr_depth", "16 bits", "Bit depth of textures", CV_SAVE, scr_depth_cons_t, NULL);
consvar_t cv_renderview = CVAR_INIT ("renderview", "On", NULL, 0, CV_OnOff, NULL);

static void SCR_ActuallyChangeRenderer(void);
static CV_PossibleValue_t cv_renderer_t[] = {{1, "Software"}, {2, "OpenGL"}, {0, NULL}};
consvar_t cv_renderer = CVAR_INIT ("renderer", "Software", "The current renderer", CV_SAVE|CV_CALL, cv_renderer_t, SCR_ChangeRenderer);

static void SCR_ChangeFullscreen(void);

#ifndef __EMSCRIPTEN__
consvar_t cv_fullscreen = CVAR_INIT ("fullscreen", "Yes", "If on, the game will take up the full screen rather than just a desktop window", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen);
#else
consvar_t cv_fullscreen = CVAR_INIT ("fullscreen", "No", "If on, the game will take up the full screen rather than just a desktop window", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen);
#endif
// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

INT32 scr_bpp; // current video mode bytes per pixel
UINT8 *scr_borderpatch; // flat used to fill the reduced view borders set at ST_Init()

// =========================================================================

//  Short and Tall sky drawer, for the current color mode
void (*walldrawerfunc)(void);
void SCR_SetDrawFuncs(void)
{
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

	wallcolfunc = walldrawerfunc;
}

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!(setmodeneeded || setrenderneeded) || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	// Jimita
	if (setrenderneeded)
	{
		needpatchflush = true;
		needpatchrecache = true;
		VID_CheckRenderer();
		if (!setmodeneeded)
			VID_SetMode(vid.modenum);
	}

	if (setmodeneeded)
		VID_SetMode(--setmodeneeded);

	V_SetPalette(0);

	SCR_SetDrawFuncs();

	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
	setrenderneeded = 0;
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
	CV_RegisterVar(&cv_fpssize);
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
		CONS_Printf(M_GetText("Default resolution: %d x %d\n"), cv_scr_width.value, cv_scr_height.value);
		CONS_Printf(M_GetText("Windowed resolution: %d x %d\n"), cv_scr_width_w.value, cv_scr_height_w.value);
		CONS_Printf(M_GetText("Default bit depth: %d bits\n"), cv_scr_depth.value);
		if (cv_fullscreen.value)
			setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1; // see note above
		else
			setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value) + 1; // see note above

		if (setmodeneeded <= 0)
			CONS_Alert(CONS_WARNING, "Invalid resolution given, defaulting to base resolution\n");
	}

	SCR_ActuallyChangeRenderer();
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	// remember the default screen size
	CV_SetValue(cv_fullscreen.value ? &cv_scr_width : &cv_scr_width_w, vid.width);
	CV_SetValue(cv_fullscreen.value ? &cv_scr_height : &cv_scr_height_w, vid.height);
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
		if (cv_fullscreen.value)
			setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
		else
			setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value) + 1;

		if (setmodeneeded <= 0) // hacky safeguard
		{
			CONS_Alert(CONS_WARNING, "Invalid resolution given, defaulting to base resolution.\n");
			setmodeneeded = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT) + 1;
		}
	}
	return;
#endif
}

static int target_renderer = 0;

void SCR_ActuallyChangeRenderer(void)
{
	setrenderneeded = target_renderer;

#ifdef HWRENDER
	// Well, it didn't even load anyway.
	if ((vid.glstate == VID_GL_LIBRARY_ERROR) && (setrenderneeded == render_opengl))
	{
		if (M_CheckParm("-nogl"))
			CONS_Alert(CONS_ERROR, "OpenGL rendering was disabled!\n");
		else
			CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		setrenderneeded = 0;
		return;
	}
#endif

	// setting the same renderer twice WILL crash your game, so let's not, please
	if (rendermode == setrenderneeded)
		setrenderneeded = 0;
}

// Jimita
void SCR_ChangeRenderer(void)
{
	setrenderneeded = 0;

	if (con_startup)
	{
		target_renderer = cv_renderer.value;
#ifdef HWRENDER
		if (M_CheckParm("-opengl") && (vid.glstate == VID_GL_LIBRARY_LOADED))
			target_renderer = rendermode = render_opengl;
		else
#endif
		if (M_CheckParm("-software"))
			target_renderer = rendermode = render_soft;
		// set cv_renderer back
		SCR_ChangeRendererCVars(rendermode);
		return;
	}

	if (cv_renderer.value == 1)
		target_renderer = render_soft;
	else if (cv_renderer.value == 2)
		target_renderer = render_opengl;
	SCR_ActuallyChangeRenderer();
}

void SCR_ChangeRendererCVars(INT32 mode)
{
	// set cv_renderer back
	if (mode == render_soft)
		CV_StealthSetValue(&cv_renderer, 1);
	else if (mode == render_opengl)
		CV_StealthSetValue(&cv_renderer, 2);
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
static boolean ticsgraph[TICRATE];
static tic_t lasttic;

double averageFPS = 0.0f;

#define USE_FPS_SAMPLES

#ifdef USE_FPS_SAMPLES
#define MAX_FRAME_TIME 0.05
#define NUM_FPS_SAMPLES (16) // Number of samples to store

static double total_frame_time = 0.0;
static int frame_index;
#endif

static boolean fps_init = false;
static precise_t fps_enter = 0;

void SCR_CalculateFPS(void)
{
	precise_t fps_finish = 0;

	double frameElapsed = 0.0;

	if (fps_init == false)
	{
		fps_enter = I_GetPreciseTime();
		fps_init = true;
	}

	fps_finish = I_GetPreciseTime();
	frameElapsed = (double)((INT64)(fps_finish - fps_enter)) / I_GetPrecisePrecision();
	fps_enter = fps_finish;

#ifdef USE_FPS_SAMPLES
	total_frame_time += frameElapsed;
	if (frame_index++ >= NUM_FPS_SAMPLES || total_frame_time >= MAX_FRAME_TIME)
	{
		averageFPS = 1.0 / (total_frame_time / frame_index);
		total_frame_time = 0.0;
		frame_index = 0;
	}
#else
	// Direct, unsampled counter.
	averageFPS = 1.0 / frameElapsed;
#endif
}




void SCR_DisplayTicRate(void)
{
	INT32 fpscntcolor = 0, ticcntcolor = 0;
	const INT32 fontheight = cv_fpssize.value == 1 ? 7 : 8;
	const INT32 h = vid.height-((cv_fpssize.value == 2 ? 4 : fontheight)*vid.dupy);
	UINT32 cap = R_GetFramerateCap();
	double fps = round(averageFPS);
	INT32 hstep = 0;
	INT32 flags = V_NOSCALESTART|V_USERHUDTRANS;
	tic_t i;
	tic_t ontic = I_GetTime();
	tic_t totaltics = 0;
	INT32 xadjust = (cv_fpssize.value == 2) ? 4 : 8;
	void (*stringdrawfunc) (INT32 x, INT32 y, INT32 option, const char *string) = NULL;
	INT32 (*stringwidthfunc)(const char *string, INT32 option) = NULL;

	if (gamestate == GS_NULL)
		return;

	if (cap > 0)
	{
		if (fps <= cap / 2.0) fpscntcolor = V_REDMAP;
		else if (fps <= cap * 0.90) fpscntcolor = V_YELLOWMAP;
		else fpscntcolor = V_GREENMAP;
	}
	else
	{
		fpscntcolor = V_GREENMAP;
	}

	for (i = lasttic + 1; i < TICRATE+lasttic && i < ontic; ++i)
	ticsgraph[i % TICRATE] = false;

	ticsgraph[ontic % TICRATE] = true;

	for (i = 0;i < TICRATE;++i)
		if (ticsgraph[i])
			++totaltics;

	if (totaltics <= TICRATE/2) ticcntcolor = V_REDMAP;
	else if (totaltics == TICRATE) ticcntcolor = V_SKYMAP;

	switch (cv_fpssize.value)
	{
		case 0: // Normal
			stringdrawfunc = V_DrawString;
			stringwidthfunc = V_StringWidth;
		break;
		
		case 1: // Thin
			stringdrawfunc = V_DrawThinString;
			stringwidthfunc = V_ThinStringWidth;
		break;

		case 2: // Small
			stringdrawfunc = V_DrawSmallString;
			stringwidthfunc = V_SmallStringWidth;
		break;
	}

	if (cv_ticrate.value == 2) // compact counter
	{
		INT32 width = vid.dupx*stringwidthfunc(va("%04.2f", averageFPS), V_NOSCALESTART);
		stringdrawfunc(vid.width-width, h, fpscntcolor|flags, va("%04.2f", averageFPS)); // use averageFPS directly

		if (cv_fpssize.value == 2)
			hstep = 4*vid.dupy;
		else
			hstep = 8*vid.dupy;
	}
	else if (cv_ticrate.value == 1) // full counter
	{
		const char *drawnstr;
		INT32 width;

		// The highest assignable cap is < 1000, so 3 characters is fine.
		if (cap > 0)
			drawnstr = va("%3.0f/%3u", fps, cap);
		else
			drawnstr = va("%4.2f", averageFPS);

		width = vid.dupx * stringwidthfunc(drawnstr, flags);

		stringdrawfunc(vid.width - ((7 * xadjust * vid.dupx) + (vid.dupx*stringwidthfunc("FPS: ", flags))), h, V_YELLOWMAP|flags, "FPS:");
		stringdrawfunc(vid.width - width, h, fpscntcolor|flags, drawnstr);

		if (cv_fpssize.value == 2)
			hstep = 4*vid.dupy;
		else
			hstep = 8*vid.dupy;
	}

	if (cv_tpscounter.value == 2) // compact counter
	{
		INT32 width = vid.dupx*stringwidthfunc(va("%d", totaltics), flags);
		stringdrawfunc(vid.width-width, h-hstep, ticcntcolor|flags, va("%d", totaltics));
	}
	else if (cv_tpscounter.value == 1) // full counter
	{
		const char *drawnstr = va("%02d/%02d", totaltics, TICRATE);
		INT32 width = vid.dupx*stringwidthfunc(drawnstr, flags);
		stringdrawfunc((vid.width - ((7 * xadjust * vid.dupx) + (vid.dupx*stringwidthfunc("TPS: ", flags)))), h-hstep, V_YELLOWMAP|flags, "TPS: ");
		stringdrawfunc(vid.width - width, h-hstep, ticcntcolor|flags, drawnstr);
	}
	lasttic = ontic;
}

void SCR_DisplayLocalPing(void)
{
	UINT32 ping = playerpingtable[consoleplayer];	// consoleplayer's ping is everyone's ping in a splitnetgame :P
	UINT32 packetloss = playerpacketlosstable[consoleplayer];
	boolean shitping = (cv_showping.value == 2 && servermaxping && ping > servermaxping);

	if (cv_showping.value == 1 || shitping)	// only show 2 (warning) if our ping is at a bad level
	{
		INT32 dispy = 187;
		INT32 transflag = shitping ? V_10TRANS*(leveltime/2) : 0;

		if(cv_tpscounter.value && cv_ticrate.value)
			dispy = 172;
		else if (cv_tpscounter.value || cv_ticrate.value)
			dispy = 180;

		if (paused || P_AutoPause())
			transflag = 0;

		HU_drawPing(307, dispy, ping, packetloss, true, transflag|V_SNAPTORIGHT|V_SNAPTOBOTTOM);
	}
}
