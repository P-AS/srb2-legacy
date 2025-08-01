// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_main.c
/// \brief Rendering main loop and setup functions,
///        utility functions (BSP, geometry, trigonometry).
///        See tables.c, too.

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h" // faB(21jan): testing
#include "r_sky.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
#include "p_spec.h" // skyboxmo
#include "p_setup.h"
#include "z_zone.h"
#include "m_random.h" // quake camera shake
#include "r_fps.h"
#include "i_system.h"


#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// increment every time a check is made
size_t validcount = 1;

INT32 centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection;
fixed_t projectiony; // aspect ratio
fixed_t fovtan; // field of view

// just for profiling purposes
size_t framecount;

size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
boolean viewsky, skyVisible;
boolean skyVisible1, skyVisible2; // saved values of skyVisible for P1 and P2, for splitscreen
sector_t *viewsector;
player_t *viewplayer;
mobj_t *r_viewmobj;


fixed_t rendertimefrac;
fixed_t rendertimefrac_unpaused;
fixed_t renderdeltatics;
boolean renderisnewtic;


// PORTALS!
// You can thank and/or curse JTE for these.
UINT8 portalrender;
sector_t *portalcullsector;
typedef struct portal_pair
{
	INT32 line1;
	INT32 line2;
	UINT8 pass;
	struct portal_pair *next;

	fixed_t viewx;
	fixed_t viewy;
	fixed_t viewz;
	angle_t viewangle;

	INT32 start;
	INT32 end;
	INT16 *ceilingclip;
	INT16 *floorclip;
	fixed_t *frontscale;
} portal_pair;
portal_pair *portal_base, *portal_cap;
line_t *portalclipline;
INT32 portalclipstart, portalclipend;

//
// precalculated math tables
//
angle_t clipangle;
angle_t doubleclipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
INT32 viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[MAXVIDWIDTH+1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];



// Hack to support extra boom colormaps.
size_t num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];

// Render stats
precise_t ps_prevframetime = 0;
ps_metric_t ps_rendercalltime = {0};
ps_metric_t ps_otherrendertime = {0};
ps_metric_t ps_uitime = {0};
ps_metric_t ps_swaptime = {0};

ps_metric_t ps_skyboxtime = {0};
ps_metric_t ps_bsptime = {0};

ps_metric_t ps_sw_spritecliptime = {0};
ps_metric_t ps_sw_portaltime = {0};
ps_metric_t ps_sw_planetime = {0};
ps_metric_t ps_sw_maskedtime = {0};

ps_metric_t ps_numbspcalls = {0};
ps_metric_t ps_numsprites = {0};
ps_metric_t ps_numdrawnodes = {0};
ps_metric_t ps_numpolyobjects = {0};

static CV_PossibleValue_t drawdist_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{3072, "3072"},	{4096, "4096"},	{6144, "6144"},
	{8192, "8192"},	{0, "Infinite"},	{0, NULL}};
static CV_PossibleValue_t precipdensity_cons_t[] = {{0, "None"}, {1, "Light"}, {2, "Moderate"}, {4, "Heavy"}, {6, "Thick"}, {8, "V.Thick"}, {0, NULL}};
static CV_PossibleValue_t translucenthud_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
static CV_PossibleValue_t maxportals_cons_t[] = {{0, "MIN"}, {12, "MAX"}, {0, NULL}}; // lmao rendering 32 portals, you're a card
static CV_PossibleValue_t homremoval_cons_t[] = {{0, "No"}, {1, "Yes"}, {2, "Flash"}, {0, NULL}};
static CV_PossibleValue_t fov_cons_t[] = {{MINFOV*FRACUNIT, "MIN"}, {MAXFOV*FRACUNIT, "MAX"}, {0, NULL}};

static void R_SetFov(fixed_t playerfov);

static void Fov_OnChange(void);
static void ChaseCam_OnChange(void);
static void ChaseCam2_OnChange(void);
static void FlipCam_OnChange(void);
static void FlipCam2_OnChange(void);
void SendWeaponPref(void);
void SendWeaponPref2(void);

consvar_t cv_tailspickup = CVAR_INIT ("tailspickup", "On", NULL, CV_NETVAR, CV_OnOff, NULL);
consvar_t cv_chasecam = CVAR_INIT ("chasecam", "On",  NULL,CV_CALL, CV_OnOff, ChaseCam_OnChange);
consvar_t cv_chasecam2 = CVAR_INIT ("chasecam2", "On", NULL, CV_CALL, CV_OnOff, ChaseCam2_OnChange);
consvar_t cv_flipcam = CVAR_INIT ("flipcam", "No", NULL, CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam_OnChange);
consvar_t cv_flipcam2 = CVAR_INIT ("flipcam2", "No", NULL, CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam2_OnChange);

consvar_t cv_shadow = CVAR_INIT ("shadow", "Off", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_shadowoffs = CVAR_INIT ("offsetshadows", "Off", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_skybox = CVAR_INIT ("skybox", "On", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_skydome = CVAR_INIT ("skydome", "On", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_ffloorclip =  CVAR_INIT("r_ffloorclip", "On", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_spriteclip = CVAR_INIT ("r_spriteclip", "On", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_soniccd = CVAR_INIT ("soniccd", "Off", NULL, CV_NETVAR, CV_OnOff, NULL);
consvar_t cv_allowmlook = CVAR_INIT ("allowmlook", "Yes", NULL, CV_NETVAR, CV_YesNo, NULL);
consvar_t cv_showhud = CVAR_INIT ("showhud", "Yes", "Whether or not to show the Heads-Up Display", CV_CALL,  CV_YesNo, R_SetViewSize);
consvar_t cv_translucenthud = CVAR_INIT ("translucenthud", "10", "How opaque the HUD is, lower values make the HUD more transparent", CV_SAVE, translucenthud_cons_t, NULL);
consvar_t cv_uncappedhud = CVAR_INIT ("uncappedhud", "Yes", NULL, CV_SAVE, CV_YesNo, NULL);
consvar_t cv_modernpause = CVAR_INIT ("modernpause", "On", "Use a blue textbox or a graphic when the game is paused", CV_SAVE, CV_OnOff, NULL);

consvar_t cv_translucency = CVAR_INIT ("translucency", "On", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_drawdist = CVAR_INIT ("drawdist", "Infinite", "Draw distance for map objects", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_nights = CVAR_INIT ("drawdist_nights", "2048", "Draw distance for NiGHTS hoops", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_precip = CVAR_INIT ("drawdist_precip", "1024", "Draw distance for rain and snow", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_precipdensity = CVAR_INIT ("precipdensity", "Moderate", "Density of rain and snow, note that this can have a significant impact on performance", CV_SAVE, precipdensity_cons_t, NULL);

// Okay, whoever said homremoval causes a performance hit should be shot.
consvar_t cv_homremoval = CVAR_INIT ("homremoval", "No", "Fixes the Hall of Mirrors bug in the Software renderer", CV_SAVE, homremoval_cons_t, NULL);

consvar_t cv_fov = CVAR_INIT ("fov", "90", "Change the camera's field of view, giving yourself a wider lens", CV_FLOAT|CV_CALL|CV_SAVE, fov_cons_t, Fov_OnChange);
consvar_t cv_fovchange = CVAR_INIT ("fovchange", "Off", NULL, CV_SAVE, CV_OnOff, NULL);

consvar_t cv_maxportals = CVAR_INIT ("maxportals", "2",  NULL, CV_SAVE, maxportals_cons_t, NULL);


void SplitScreen_OnChange(void)
{
	if (!cv_debug && netgame)
	{
		if (splitscreen)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Splitscreen not supported in netplay, sorry!\n"));
			splitscreen = false;
		}
		return;
	}

	// recompute screen size
	R_ExecuteSetViewSize();

	if (!demoplayback && !botingame)
	{
		if (splitscreen)
			CL_AddSplitscreenPlayer();
		else
			CL_RemoveSplitscreenPlayer();

		if (server && !netgame)
			multiplayer = splitscreen;
	}
	else
	{
		INT32 i;
		secondarydisplayplayer = consoleplayer;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && i != consoleplayer)
			{
				secondarydisplayplayer = i;
				break;
			}
	}
}

static void Fov_OnChange(void)
{
	R_SetViewSize();
}

static void ChaseCam_OnChange(void)
{
	if (!cv_chasecam.value || !cv_useranalog.value)
		CV_SetValue(&cv_analog, 0);
	else
		CV_SetValue(&cv_analog, 1);
}

static void ChaseCam2_OnChange(void)
{
	if (botingame)
		return;
	if (!cv_chasecam2.value || !cv_useranalog2.value)
		CV_SetValue(&cv_analog2, 0);
	else
		CV_SetValue(&cv_analog2, 1);
}

static void FlipCam_OnChange(void)
{
	SendWeaponPref();
}

static void FlipCam2_OnChange(void)
{
	SendWeaponPref2();
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

// This version uses 64-bit variables to avoid overflows with large values.
angle_t R_PointToAngle64(INT64 x, INT64 y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDivEx(y,x)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x,y)] :                               // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDivEx(y,x)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x,y)] :                              // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDivEx(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x,y)] :                             // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDivEx(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x,y)] :                              // octant 5
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1)
{
	angle_t angle;
	fixed_t dx, dy, dist;

	dx = abs(px1 - px2);
	dy = abs(py1 - py2);

	if (dy > dx)
	{
		fixed_t temp;

		temp = dx;
		dx = dy;
		dy = temp;
	}
	if (!dy)
		return dx;

	angle = (tantoangle[FixedDiv(dy, dx)>>DBITS] + ANGLE_90) >> ANGLETOFINESHIFT;

	// use as cosine
	dist = FixedDiv(dx, FINESINE(angle));

	return dist;
}

// Little extra utility. Works in the same way as R_PointToAngle2
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

angle_t R_PointToAngleEx(INT32 x2, INT32 y2, INT32 x1, INT32 y1)
{
	INT64 dx = x1-x2;
	INT64 dy = y1-y2;
	if (dx < INT32_MIN || dx > INT32_MAX || dy < INT32_MIN || dy > INT32_MAX)
	{
		x1 = (int)(dx / 2 + x2);
		y1 = (int)(dy / 2 + y2);
	}
	return (y1 -= y2, (x1 -= x2) || y1) ?
	x1 >= 0 ?
	y1 >= 0 ?
		(x1 > y1) ? tantoangle[SlopeDivEx(y1,x1)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x1,y1)] :                               // octant 1
		x1 > (y1 = -y1) ? 0-tantoangle[SlopeDivEx(y1,x1)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x1,y1)] :                              // octant 7
		y1 >= 0 ? (x1 = -x1) > y1 ? ANGLE_180-tantoangle[SlopeDivEx(y1,x1)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x1,y1)] :                             // octant 2
		(x1 = -x1) > (y1 = -y1) ? ANGLE_180+tantoangle[SlopeDivEx(y1,x1)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x1,y1)] :                              // octant 5
		0;
}

INT32 R_GetHudUncap(boolean menu)
{
	return cv_uncappedhud.value ? ((menu) ? (rendertimefrac_unpaused & FRACMASK) : (rendertimefrac & FRACMASK) ) : 0; // Ternary operators are FUN -chromaticpipe
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// killough 5/2/98: reformatted, cleaned up
//
// note: THIS IS USED ONLY FOR WALLS!
fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
	angle_t anglea = ANGLE_90 + (visangle-viewangle);
	angle_t angleb = ANGLE_90 + (visangle-rw_normalangle);
	fixed_t den = FixedMul(rw_distance, FINESINE(anglea>>ANGLETOFINESHIFT));
	// proff 11/06/98: Changed for high-res
	fixed_t num = FixedMul(projectiony, FINESINE(angleb>>ANGLETOFINESHIFT));

	if (den > num>>16)
	{
		num = FixedDiv(num, den);
		if (num > 64*FRACUNIT)
			return 64*FRACUNIT;
		if (num < 256)
			return 256;
		return num;
	}
	return 64*FRACUNIT;
}

//
// R_DoCulling
// Checks viewz and top/bottom heights of an item against culling planes
// Returns true if the item is to be culled, i.e it shouldn't be drawn!
// if ML_NOCLIMB is set, the camera view is required to be in the same area for culling to occur
boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph)
{
	fixed_t cullplane;

	if (!cullheight)
		return false;

	cullplane = cullheight->frontsector->floorheight;
	if (cullheight->flags & ML_NOCLIMB) // Group culling
	{
		if (!viewcullheight)
			return false;

		// Make sure this is part of the same group
		if (viewcullheight->frontsector == cullheight->frontsector)
		{
			// OK, we can cull
			if (vz > cullplane && toph < cullplane) // Cull if below plane
				return true;

			if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
				return true;
		}
	}
	else // Quick culling
	{
		if (vz > cullplane && toph < cullplane) // Cull if below plane
			return true;

		if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
			return true;
	}

	return false;
}

//
// R_InitTextureMapping
//
static void R_InitTextureMapping(void)
{
	INT32 i;
	INT32 x;
	INT32 t;
	fixed_t focallength;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(projection,
		FINETANGENT(FINEANGLES/4+FIELDOFVIEW/2));


	focallengthf = FIXED_TO_FLOAT(focallength);


	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > fovtan*2)
			t = -1;
		else if (FINETANGENT(i) < -fovtan*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	for (x = 0; x <= viewwidth;x++)
	{
		i = 0;
		while (viewangletox[i] > x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == viewwidth+1)
			viewangletox[i]  = viewwidth;
	}

	clipangle = xtoviewangle[0];
	doubleclipangle = clipangle*2;
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP 2

static inline void R_InitLightTables(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 scale;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{
			//added : 02-02-98 : use BASEVIDWIDTH, vid.width is not set already,
			// and it seems it needs to be calculated only once.
			scale = FixedDiv((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmapl - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = colormaps + level*256;
		}
	}
}


//#define WOUGHMP_WOUGHMP // I got a fish-eye lens - I'll make a rap video with a couple of friends
// it's kinda laggy sometimes

static struct {
	angle_t rollangle; // pre-shifted by fineshift
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye;
#endif

	fixed_t zoomneeded;
	INT32 *scrmap;
	INT32 scrmapsize;

	INT32 x1; // clip rendering horizontally for efficiency
	INT16 ceilingclip[MAXVIDWIDTH], floorclip[MAXVIDWIDTH];

	boolean use;
} viewmorph = {
	0,
#ifdef WOUGHMP_WOUGHMP
	0,
#endif

	FRACUNIT,
	NULL,
	0,

	0,
	{}, {},

	false
};

void R_CheckViewMorph(void)
{
	float zoomfactor, rollcos, rollsin;
	float x1, y1, x2, y2;
	fixed_t temp;
	INT32 end, vx, vy, pos, usedpos;
	INT32 usedx, usedy, halfwidth = vid.width/2, halfheight = vid.height/2;
#ifdef WOUGHMP_WOUGHMP
	float fisheyemap[MAXVIDWIDTH/2 + 1];
#endif

	angle_t rollangle = players[displayplayer].viewrollangle;
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye = cv_cam2_turnmultiplier.value; // temporary test value
#endif

	rollangle >>= ANGLETOFINESHIFT;
	rollangle = ((rollangle+2) & ~3) & FINEMASK; // Limit the distinct number of angles to reduce recalcs from angles changing a lot.

#ifdef WOUGHMP_WOUGHMP
	fisheye &= ~0x7FF; // Same
#endif

	if (rollangle == viewmorph.rollangle &&
#ifdef WOUGHMP_WOUGHMP
		fisheye == viewmorph.fisheye &&
#endif
		viewmorph.scrmapsize == vid.width*vid.height)
		return; // No change

	viewmorph.rollangle = rollangle;
#ifdef WOUGHMP_WOUGHMP
	viewmorph.fisheye = fisheye;
#endif

	if (viewmorph.rollangle == 0
#ifdef WOUGHMP_WOUGHMP
		 && viewmorph.fisheye == 0
#endif
	 )
	{
		viewmorph.use = false;
		viewmorph.x1 = 0;
		if (viewmorph.zoomneeded != FRACUNIT)
			R_SetViewSize();
		viewmorph.zoomneeded = FRACUNIT;

		return;
	}

	if (viewmorph.scrmapsize != vid.width*vid.height)
	{
		if (viewmorph.scrmap)
			free(viewmorph.scrmap);
		viewmorph.scrmap = malloc(vid.width*vid.height * sizeof(INT32));
		viewmorph.scrmapsize = vid.width*vid.height;
	}

	temp = FINECOSINE(rollangle);
	rollcos = FIXED_TO_FLOAT(temp);
	temp = FINESINE(rollangle);
	rollsin = FIXED_TO_FLOAT(temp);

	// Calculate maximum zoom needed
	x1 = (vid.width*fabsf(rollcos) + vid.height*fabsf(rollsin)) / vid.width;
	y1 = (vid.height*fabsf(rollcos) + vid.width*fabsf(rollsin)) / vid.height;

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		float f = FIXED_TO_FLOAT(fisheye);
		for (vx = 0; vx <= halfwidth; vx++)
			fisheyemap[vx] = 1.0f / cos(atan(vx * f / halfwidth));

		f = cos(atan(f));
		if (f < 1.0f)
		{
			x1 /= f;
			y1 /= f;
		}
	}
#endif

	temp = max(x1, y1)*FRACUNIT;
	if (temp < FRACUNIT)
		temp = FRACUNIT;
	else
		temp |= 0x3FFF; // Limit how many times the viewport needs to be recalculated

	//CONS_Printf("Setting zoom to %f\n", FIXED_TO_FLOAT(temp));

	if (temp != viewmorph.zoomneeded)
	{
		viewmorph.zoomneeded = temp;
		R_SetViewSize();
	}

	zoomfactor = FIXED_TO_FLOAT(viewmorph.zoomneeded);

	end = vid.width * vid.height - 1;

	pos = 0;

	// Pre-multiply rollcos and rollsin to use for positional stuff
	rollcos /= zoomfactor;
	rollsin /= zoomfactor;

	x1 = -(halfwidth * rollcos - halfheight * rollsin);
	y1 = -(halfheight * rollcos + halfwidth * rollsin);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
		viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)) * fisheyemap[halfwidth]);
	else
#endif
	viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)));
	//CONS_Printf("saving %d cols\n", viewmorph.x1);

	// Set ceilingclip and floorclip
	for (vx = 0; vx < vid.width; vx++)
	{
		viewmorph.ceilingclip[vx] = vid.height;
		viewmorph.floorclip[vx] = -1;
	}
	x2 = x1;
	y2 = y1;
	for (vx = 0; vx < vid.width; vx++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight-1;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 += rollcos;
		y2 += rollsin;
	}
	x2 = x1;
	y2 = y1;
	for (vy = 0; vy < vid.height; vy++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 -= rollsin;
		y2 += rollcos;
	}

	//CONS_Printf("Top left corner is %f %f\n", x1, y1);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		for (vy = 0; vy < halfheight; vy++)
		{
			x2 = x1;
			y2 = y1;
			x1 -= rollsin;
			y1 += rollcos;

			for (vx = 0; vx < vid.width; vx++)
			{
				usedx = halfwidth + x2*fisheyemap[(int) floorf(fabsf(y2*zoomfactor))];
				usedy = halfheight + y2*fisheyemap[(int) floorf(fabsf(x2*zoomfactor))];

				usedpos = usedx + usedy*vid.width;

				viewmorph.scrmap[pos] = usedpos;
				viewmorph.scrmap[end-pos] = end-usedpos;

				x2 += rollcos;
				y2 += rollsin;
				pos++;
			}
		}
	}
	else
	{
#endif
	x1 += halfwidth;
	y1 += halfheight;

	for (vy = 0; vy < halfheight; vy++)
	{
		x2 = x1;
		y2 = y1;
		x1 -= rollsin;
		y1 += rollcos;

		for (vx = 0; vx < vid.width; vx++)
		{
			usedx = x2;
			usedy = y2;

			usedpos = usedx + usedy*vid.width;

			viewmorph.scrmap[pos] = usedpos;
			viewmorph.scrmap[end-pos] = end-usedpos;

			x2 += rollcos;
			y2 += rollsin;
			pos++;
		}
	}
#ifdef WOUGHMP_WOUGHMP
	}
#endif

	viewmorph.use = true;
}

void R_ApplyViewMorph(void)
{
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	INT32 p, end = vid.width * vid.height;

	if (!viewmorph.use)
		return;

	if (cv_debug & DBG_VIEWMORPH)
	{
		UINT8 border = 32;
		UINT8 grid = 160;
		INT32 ws = vid.width / 4;
		INT32 hs = vid.width * (vid.height / 4);

		memcpy(tmpscr, srcscr, vid.width*vid.height);
		for (p = 0; p < vid.width; p++)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + hs]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*2]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
		for (p = vid.width; p < end; p += vid.width)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + ws]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*2]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
	}
	else
		for (p = 0; p < end; p++)
			tmpscr[p] = srcscr[viewmorph.scrmap[p]];

	VID_BlitLinearScreen(tmpscr, screens[0],
			vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.width);
}




//
// R_SetViewSize
// Do not really change anything here,
// because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean setsizeneeded;

void R_SetViewSize(void)
{
	setsizeneeded = true;
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;

	setsizeneeded = false;

	if (rendermode == render_none)
		return;

	// status bar overlay
	st_overlay = cv_showhud.value;

	scaledviewwidth = vid.width;
	viewheight = vid.height;

	if (splitscreen)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	centerx = viewwidth/2;
	centery = viewheight/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	R_SetFov(cv_fov.value);


	R_InitViewBuffer(scaledviewwidth, viewheight);


#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_InitTextureMapping();
#endif

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;


	if (rendermode == render_soft)
	{
		if (ds_su)
			Z_Free(ds_su);
		if (ds_sv)
			Z_Free(ds_sv);
		if (ds_sz)
			Z_Free(ds_sz);

		ds_su = ds_sv = ds_sz = NULL;
		ds_sup = ds_svp = ds_szp = NULL;
	}

	memset(scalelight, 0xFF, sizeof(scalelight));

	// Calculate the light levels to use for each level/scale combination.
	for (i = 0; i< LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmapl - j*vid.width/(viewwidth)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}

	// continue to do the software setviewsize as long as we use the reference software view
#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_SetViewSize();
#endif

	am_recalc = true;
}
fixed_t R_GetPlayerFov(player_t *player)
{
	fixed_t fov = cv_fov.value + player->fovadd;
	return max(MINFOV*FRACUNIT, min(fov, MAXFOV*FRACUNIT));
}

static void R_SetFov(fixed_t playerfov)
{
	angle_t fov = FixedAngle(playerfov/2) + ANGLE_90;
	fovtan = FixedMul(FINETANGENT(fov >> ANGLETOFINESHIFT), viewmorph.zoomneeded);
	if (splitscreen == 1) // Splitscreen FOV should be adjusted to maintain expected vertical view
		fovtan = 17*fovtan/10;

	// this is only used for planes rendering in software mode
	INT32 j = viewheight*16;
	for (INT32 i = 0; i < j; i++)
	{
		fixed_t dy = (i - viewheight*8)<<FRACBITS;
		dy = FixedMul(abs(dy), fovtan);
		yslopetab[i] = FixedDiv(centerx*FRACUNIT, dy);
	}

	projection = projectiony = FixedDiv(centerxfrac, fovtan);

	R_InitTextureMapping();

	// setup sky scaling
	R_SetSkyScale();
}

//
// R_Init
//

static fixed_t viewfov[2];


void R_Init(void)
{
	// screensize independent
	//I_OutputMsg("\nR_InitData");
	R_InitData();

	//I_OutputMsg("\nR_InitViewBorder");
	R_InitViewBorder();
	R_SetViewSize(); // setsizeneeded is set true

	//I_OutputMsg("\nR_InitPlanes");
	R_InitPlanes();

	// this is now done by SCR_Recalc() at the first mode set
	//I_OutputMsg("\nR_InitLightTables");
	R_InitLightTables();

	//I_OutputMsg("\nR_InitTranslationTables\n");
	R_InitTranslationTables();

	R_InitDrawNodes();

	framecount = 0;
}

//
// R_IsPointInSector
//
boolean R_IsPointInSector(sector_t *sector, fixed_t x, fixed_t y)
{
	size_t i;
	size_t passes = 0;

	for (i = 0; i < sector->linecount; i++)
	{
		line_t *line = sector->lines[i];
		vertex_t *v1, *v2;

		if (line->frontsector == line->backsector)
			continue;

		v1 = line->v1;
		v2 = line->v2;

		// make sure v1 is below v2
		if (v1->y > v2->y)
		{
			vertex_t *tmp = v1;
			v1 = v2;
			v2 = tmp;
		}
		else if (v1->y == v2->y)
			// horizontal line, we can't match this
			continue;

		if (v1->y < y && y <= v2->y)
		{
			// if the y axis in inside the line, find the point where we intersect on the x axis...
			fixed_t vx = v1->x + (INT64)(v2->x - v1->x) * (y - v1->y) / (v2->y - v1->y);

			// ...and if that point is to the left of the point, count it as inside.
			if (vx < x)
				passes++;
		}
	}

	// and odd number of passes means we're inside the polygon.
	return passes % 2;
}

//
// R_SetupFrame
//
void R_SetupFrame(player_t *player, boolean skybox)
{
	camera_t *thiscam;
	boolean chasecam = R_ViewpointHasChasecam(player);
	
	if (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	newview->sky = !skybox;
	if (player->awayviewtics)
	{
		// cut-away view stuff
		r_viewmobj = player->awayviewmobj; // should be a MT_ALTVIEWMAN
		I_Assert(r_viewmobj != NULL);
		newview->z = r_viewmobj->z + 20*FRACUNIT;
		newview->aim = player->awayviewaiming;
		newview->angle = r_viewmobj->angle;
	}
	else if (!player->spectator && chasecam)
	// use outside cam view
	{
		r_viewmobj = NULL;
		newview->z = thiscam->z + (thiscam->height>>1);
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;

	}
	else
	// use the player's eyes view
	{
		newview->z = player->viewz;

		r_viewmobj = player->mo;
		I_Assert(r_viewmobj != NULL);

		newview->aim = player->aiming;
		newview->angle = r_viewmobj->angle;

		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle; // WARNING: camera uses this
				newview->aim = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;

			}
		}
	}
	newview->z += quake.z;

	newview->player = player;

	if (chasecam && !player->awayviewtics && !player->spectator)
	{
		newview->x = thiscam->x;
		newview->y = thiscam->y;
		newview->x += quake.x;
		newview->y += quake.y;


		if (thiscam->subsector)
			newview->sector = thiscam->subsector->sector;
		else
			newview->sector = R_PointInSubsectorFast(viewx, viewy)->sector;
	}
	else
	{
		newview->x = r_viewmobj->x;
		newview->y = r_viewmobj->y;
		newview->x += quake.x;
		newview->y += quake.y;

		if (r_viewmobj->subsector)
			newview->sector = r_viewmobj->subsector->sector;
		else
			newview->sector = R_PointInSubsectorFast(viewx, viewy)->sector;
	}

	//newview->sin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	//newview->cos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
}

void R_SkyboxFrame(player_t *player)
{
	camera_t *thiscam;

	if (splitscreen && player == &players[secondarydisplayplayer]
	&& player != &players[consoleplayer])
	{
        R_SetViewContext(VIEWCONTEXT_SKY2);
		thiscam = &camera2;
	}
	else
	{
		R_SetViewContext(VIEWCONTEXT_SKY1);
		thiscam = &camera;
	}
	// cut-away view stuff
	newview->sky = true;
	r_viewmobj = skyboxmo[0];
#ifdef PARANOIA
	if (!(r_viewmobj))
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: viewmobj null (player %s)", sizeu1(playeri));
	}
#endif
	if (player->awayviewtics)
	{
		newview->aim = player->awayviewaiming;
		newview->angle = player->awayviewmobj->angle;
	}
	else if (thiscam->chase)
	{
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;

	}
	else
	{
		newview->aim = player->aiming;
		newview->angle = player->mo->angle;

		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle; // WARNING: camera uses this
				newview->aim = localaiming;

			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;

			}
		}
	}
	newview->angle += r_viewmobj->angle;

	newview->player = player;

	newview->x = r_viewmobj->x;
	newview->y = r_viewmobj->y;
	newview->z = 0;

	if (r_viewmobj->spawnpoint)
		newview->z = ((fixed_t)r_viewmobj->spawnpoint->angle)<<FRACBITS;

	newview->x += quake.x;
	newview->y += quake.y;
	newview->z += quake.z;


	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		if (player->awayviewtics)
		{
			if (skyboxmo[1])
			{
				fixed_t x = 0, y = 0;
				if (mh->skybox_scalex > 0)
					x = (player->awayviewmobj->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					x = (player->awayviewmobj->x - skyboxmo[1]->x) * -mh->skybox_scalex;

				if (mh->skybox_scaley > 0)
					y = (player->awayviewmobj->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					y = (player->awayviewmobj->y - skyboxmo[1]->y) * -mh->skybox_scaley;

				if (r_viewmobj->angle == 0)
				{
					newview->x += x;
					newview->y += y;

				}
				else if (r_viewmobj->angle == ANGLE_90)
				{
					newview->x -= y;
					newview->y += x;

				}
				else if (r_viewmobj->angle == ANGLE_180)
				{
					newview->x -= x;
					newview->y -= y;

				}
				else if (r_viewmobj->angle == ANGLE_270)
				{
					newview->x += y;
					newview->y -= x;

				}
				else
				{
					angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
					newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
					newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));

				}
			}
			if (mh->skybox_scalez > 0)
				newview->z += (player->awayviewmobj->z + 20*FRACUNIT) / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				newview->z += (player->awayviewmobj->z + 20*FRACUNIT) * -mh->skybox_scalez;
		}
		else if (thiscam->chase)
		{
			if (skyboxmo[1])
			{
				fixed_t x = 0, y = 0;
				if (mh->skybox_scalex > 0)
					x = (thiscam->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					x = (thiscam->x - skyboxmo[1]->x) * -mh->skybox_scalex;

				if (mh->skybox_scaley > 0)
					y = (thiscam->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					y = (thiscam->y - skyboxmo[1]->y) * -mh->skybox_scaley;

				if (r_viewmobj->angle == 0)
				{
					newview->x += x;
					newview->y += y;

				}
				else if (r_viewmobj->angle == ANGLE_90)
				{
					newview->x -= y;
					newview->y += x;

				}
				else if (r_viewmobj->angle == ANGLE_180)
				{
					newview->x -= x;
					newview->y -= y;

				}
				else if (r_viewmobj->angle == ANGLE_270)
				{
					newview->x += y;
					newview->y -= x;

				}
				else
				{
					angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
					newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
					newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));

				}
			}
			if (mh->skybox_scalez > 0)
				newview->z += (thiscam->z + (thiscam->height>>1)) / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				newview->z += (thiscam->z + (thiscam->height>>1)) * -mh->skybox_scalez;
		}
		else
		{
			if (skyboxmo[1])
			{
				fixed_t x = 0, y = 0;
				if (mh->skybox_scalex > 0)
					x = (player->mo->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					x = (player->mo->x - skyboxmo[1]->x) * -mh->skybox_scalex;
				if (mh->skybox_scaley > 0)
					y = (player->mo->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					y = (player->mo->y - skyboxmo[1]->y) * -mh->skybox_scaley;

				if (r_viewmobj->angle == 0)
				{
					newview->x  += x;
					newview->y += y;
				}
				else if (r_viewmobj->angle == ANGLE_90)
				{
					newview->x  -= y;
					newview->y += x;
				}
				else if (r_viewmobj->angle == ANGLE_180)
				{
					newview->x  -= x;
					newview->y -= y;
				}
				else if (r_viewmobj->angle == ANGLE_270)
				{
					newview->x  += y;
					newview->y -= x;
				}
				else
				{
					angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
					newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
					newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
				}
			}
			if (mh->skybox_scalez > 0)
				newview->z += player->viewz / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				newview->z += player->viewz * -mh->skybox_scalez;
		}
	}

	if (r_viewmobj->subsector)
		newview->sector = r_viewmobj->subsector->sector;
	else
		newview->sector = R_PointInSubsectorFast(viewx, viewy)->sector;

	//newview->sin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	//newview->cos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);

}

boolean R_ViewpointHasChasecam(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = false;
	boolean isplayer2 = (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer]);

	if (isplayer2)
	{
		thiscam = &camera2;
		chasecam = (cv_chasecam2.value != 0);
	}
	else
	{
		thiscam = &camera;
		chasecam = (cv_chasecam.value != 0);
	}

	if (player->climbing || (player->pflags & PF_NIGHTSMODE) || player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN)
		chasecam = true; // force chasecam on
	else if (player->spectator) // no spectator chasecam
		chasecam = false; // force chasecam off
		
	if (chasecam && !thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = true;
	}
	else if (!chasecam && thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = false;
	}
	
	if (isplayer2)
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER2);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(2);
			thiscam->reset = false;
		}
	}
	else
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER1);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(1);
			thiscam->reset = false;
		}
	}

	return chasecam;
}



boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox)
{
	boolean chasecam = R_ViewpointHasChasecam(player);

	// cut-away view stuff
	if (player->awayviewtics || skybox)
		return chasecam;
	// use outside cam view
	else if (!player->spectator && chasecam)
		return true;

	// use the player's eyes view
	return false;
}


#define ANGLED_PORTALS

static void R_PortalFrame(line_t *start, line_t *dest, portal_pair *portal)
{
	vertex_t dest_c, start_c;
#ifdef ANGLED_PORTALS
	// delta angle
	angle_t dangle = R_PointToAngle2(0,0,dest->dx,dest->dy) - R_PointToAngle2(start->dx,start->dy,0,0);
#endif

	//R_SetupFrame(player, false);
	viewx = portal->viewx;
	viewy = portal->viewy;
	viewz = portal->viewz;

	viewangle = portal->viewangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	portalcullsector = dest->frontsector;
	viewsector = dest->frontsector;
	portalclipline = dest;
	portalclipstart = portal->start;
	portalclipend = portal->end;

	// Offset the portal view by the linedef centers

	// looking glass center
	start_c.x = (start->v1->x + start->v2->x) / 2;
	start_c.y = (start->v1->y + start->v2->y) / 2;

	// other side center
	dest_c.x = (dest->v1->x + dest->v2->x) / 2;
	dest_c.y = (dest->v1->y + dest->v2->y) / 2;

	// Heights!
	viewz += dest->frontsector->floorheight - start->frontsector->floorheight;

	// calculate the difference in position and rotation!
#ifdef ANGLED_PORTALS
	if (dangle == 0)
#endif
	{ // the entrance goes straight opposite the exit, so we just need to mess with the offset.
		viewx += dest_c.x - start_c.x;
		viewy += dest_c.y - start_c.y;
		return;
	}

#ifdef ANGLED_PORTALS
	viewangle += dangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);
	//CONS_Printf("dangle == %u\n", AngleFixed(dangle)>>FRACBITS);

	// ????
	{
		fixed_t disttopoint;
		angle_t angtopoint;

		disttopoint = R_PointToDist2(start_c.x, start_c.y, viewx, viewy);
		angtopoint = R_PointToAngle2(start_c.x, start_c.y, viewx, viewy);
		angtopoint += dangle;

		viewx = dest_c.x+FixedMul(FINECOSINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
		viewy = dest_c.y+FixedMul(FINESINE(angtopoint>>ANGLETOFINESHIFT), disttopoint);
	}
#endif
}

void R_AddPortal(INT32 line1, INT32 line2, INT32 x1, INT32 x2)
{
	portal_pair *portal = Z_Malloc(sizeof(portal_pair), PU_LEVEL, NULL);
	INT16 *ceilingclipsave = Z_Malloc(sizeof(INT16)*(x2-x1), PU_LEVEL, NULL);
	INT16 *floorclipsave = Z_Malloc(sizeof(INT16)*(x2-x1), PU_LEVEL, NULL);
	fixed_t *frontscalesave = Z_Malloc(sizeof(fixed_t)*(x2-x1), PU_LEVEL, NULL);

	portal->line1 = line1;
	portal->line2 = line2;
	portal->pass = portalrender+1;
	portal->next = NULL;

	R_PortalStoreClipValues(x1, x2, ceilingclipsave, floorclipsave, frontscalesave);

	portal->ceilingclip = ceilingclipsave;
	portal->floorclip = floorclipsave;
	portal->frontscale = frontscalesave;

	portal->start = x1;
	portal->end = x2;

	portalline = true; // this tells R_StoreWallRange that curline is a portal seg

	portal->viewx = viewx;
	portal->viewy = viewy;
	portal->viewz = viewz;
	portal->viewangle = viewangle;

	if (!portal_base)
	{
		portal_base = portal;
		portal_cap = portal;
	}
	else
	{
		portal_cap->next = portal;
		portal_cap = portal;
	}
}

// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void R_RenderPlayerView(player_t *player)
{
	portal_pair *portal;
	const boolean skybox = (skyboxmo[0] && cv_skybox.value);

	if (cv_homremoval.value && player == &players[displayplayer]) // if this is display player 1
	{
		if (cv_homremoval.value == 1)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31); // No HOM effect!
		else //'development' HOM removal -- makes it blindingly obvious if HOM is spotted.
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 128+(timeinmap&15));
	}

	// load previous saved value of skyVisible for the player
	if (splitscreen && player == &players[secondarydisplayplayer])
		skyVisible = skyVisible2;
	else
		skyVisible = skyVisible1;

	portalrender = 0;
	portal_base = portal_cap = NULL;

	PS_START_TIMING(ps_skyboxtime);
	if (skybox && skyVisible)
	{
		R_SkyboxFrame(player);

		R_ClearClipSegs();
		R_ClearDrawSegs();
		R_ClearPlanes();
		R_ClearSprites();
#ifdef FLOORSPLATS
		R_ClearVisibleFloorSplats();
#endif

		R_RenderBSPNode((INT32)numnodes - 1);
		R_ClipSprites();
		R_DrawPlanes();
#ifdef FLOORSPLATS
		R_DrawVisibleFloorSplats();
#endif
		R_DrawMasked();
	}
	PS_STOP_TIMING(ps_skyboxtime);

	fixed_t fov = R_GetPlayerFov(player);

	if (player == &players[displayplayer] && viewfov[0] != fov)
	{
		viewfov[0] = fov;
		R_SetFov(fov);
	}
	else if (player == &players[secondarydisplayplayer] && viewfov[1] != fov)
	{
		viewfov[1] = fov;
		R_SetFov(fov);
	}

	R_SetupFrame(player, skybox);
	skyVisible = false;
	framecount++;
	validcount++;

	// Clear buffers.
	R_ClearPlanes();
	R_ClearSprites();
	if (viewmorph.use)
	{
		portalclipstart = viewmorph.x1;
		portalclipend = viewwidth-viewmorph.x1-1;
		R_PortalClearClipSegs(portalclipstart, portalclipend);
		memcpy(ceilingclip, viewmorph.ceilingclip, sizeof(INT16)*vid.width);
		memcpy(floorclip, viewmorph.floorclip, sizeof(INT16)*vid.width);
	}
	else
	{
		portalclipstart = 0;
		portalclipend = viewwidth-1;
		R_ClearClipSegs();
	}
	R_ClearDrawSegs();
#ifdef FLOORSPLATS
	R_ClearVisibleFloorSplats();
#endif

	// check for new console commands.
	NetUpdate();

	// The head node is the last node output.
	ps_numbspcalls.value.i = ps_numpolyobjects.value.i = ps_numdrawnodes.value.i = 0;
	PS_START_TIMING(ps_bsptime);
	R_RenderBSPNode((INT32)numnodes - 1);
	PS_STOP_TIMING(ps_bsptime);
	PS_START_TIMING(ps_sw_spritecliptime);
	R_ClipSprites();
	PS_STOP_TIMING(ps_sw_spritecliptime);
	ps_numsprites.value.i = numvisiblesprites;

	// PORTAL RENDERING
	PS_START_TIMING(ps_sw_portaltime);
	for(portal = portal_base; portal; portal = portal_base)
	{
		// render the portal
		CONS_Debug(DBG_RENDER, "Rendering portal from line %d to %d\n", portal->line1, portal->line2);
		portalrender = portal->pass;

		R_PortalFrame(&lines[portal->line1], &lines[portal->line2], portal);

		R_PortalClearClipSegs(portal->start, portal->end);

		R_PortalRestoreClipValues(portal->start, portal->end, portal->ceilingclip, portal->floorclip, portal->frontscale);

		validcount++;

		R_RenderBSPNode((INT32)numnodes - 1);
		R_ClipSprites();
		//R_DrawPlanes();
		//R_DrawMasked();

		// okay done. free it.
		portalcullsector = NULL; // Just in case...
		portal_base = portal->next;
		Z_Free(portal->ceilingclip);
		Z_Free(portal->floorclip);
		Z_Free(portal->frontscale);
		Z_Free(portal);
	}
	PS_STOP_TIMING(ps_sw_portaltime);
	// END PORTAL RENDERING

	PS_START_TIMING(ps_sw_planetime);
	R_DrawPlanes();
	PS_STOP_TIMING(ps_sw_planetime);
#ifdef FLOORSPLATS
	R_DrawVisibleFloorSplats();
#endif
	// draw mid texture and sprite
	// And now 3D floors/sides!
	PS_START_TIMING(ps_sw_maskedtime);
	R_DrawMasked();
	PS_STOP_TIMING(ps_sw_maskedtime);

	// Check for new console commands.
	NetUpdate();

	// save value to skyVisible1 or skyVisible2
	// this is so that P1 can't affect whether P2 can see a skybox or not, or vice versa
	if (splitscreen && player == &players[secondarydisplayplayer])
		skyVisible2 = skyVisible;
	else
		skyVisible1 = skyVisible;
}

// Jimita
#ifdef HWRENDER
void R_InitHardwareMode(void)
{
	//HWR_AddCommands();
	HWR_Switch();
	HWR_LoadTextures(numtextures);
	if (gamestate == GS_LEVEL)
		HWR_SetupLevel();
}
#endif

void R_ReloadHUDGraphics(void)
{
	CONS_Debug(DBG_RENDER, "R_ReloadHUDGraphics()...\n");
	ST_LoadGraphics();
	HU_LoadGraphics();
	ST_ReloadSkinFaceGraphics();
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	CV_RegisterVar(&cv_gravity);
	CV_RegisterVar(&cv_tailspickup);
	CV_RegisterVar(&cv_soniccd);
	CV_RegisterVar(&cv_allowmlook);
	CV_RegisterVar(&cv_homremoval);
	CV_RegisterVar(&cv_flipcam);
	CV_RegisterVar(&cv_flipcam2);

	// Enough for dedicated server
	if (dedicated)
		return;

	CV_RegisterVar(&cv_precipdensity);
	CV_RegisterVar(&cv_translucency);
	CV_RegisterVar(&cv_drawdist);
	CV_RegisterVar(&cv_drawdist_nights);
	CV_RegisterVar(&cv_drawdist_precip);
	CV_RegisterVar(&cv_fov);
	CV_RegisterVar(&cv_fovchange);

	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);
	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_shadowoffs);
	CV_RegisterVar(&cv_skydome);
	CV_RegisterVar(&cv_skybox);
	CV_RegisterVar(&cv_ffloorclip);
	CV_RegisterVar(&cv_spriteclip);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_speed);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);
	CV_RegisterVar(&cv_cam_orbit);
	CV_RegisterVar(&cv_cam_adjust);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);
	CV_RegisterVar(&cv_cam2_orbit);
	CV_RegisterVar(&cv_cam2_adjust);

	CV_RegisterVar(&cv_viewroll);
	CV_RegisterVar(&cv_quakeiiiarena);
	CV_RegisterVar(&cv_quakeiv);
	CV_RegisterVar(&cv_quakelive);

	CV_RegisterVar(&cv_showhud);
	CV_RegisterVar(&cv_translucenthud);
	CV_RegisterVar(&cv_uncappedhud);
	CV_RegisterVar(&cv_modernpause);

	CV_RegisterVar(&cv_maxportals);

	// Default viewheight is changeable,
	// initialized to standard viewheight
	CV_RegisterVar(&cv_viewheight);
	// Uncapped
	CV_RegisterVar(&cv_fpscap);
}
