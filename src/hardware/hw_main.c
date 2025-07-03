// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief hardware renderer, using the standard HardWareRender driver DLL for SRB2

#include <math.h>

#include "../doomstat.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_light.h"
#include "hw_drv.h"
#include "hw_batching.h"

#include "../i_video.h" // for rendermode == render_glide
#include "../v_video.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../r_local.h"
#include "../r_bsp.h"
#include "../d_clisrv.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_splats.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../i_system.h"
#include "../m_cheat.h"
#include "../d_main.h"
#include "../p_slopes.h"
#include "hw_md2.h"

#ifdef NEWCLIP
#include "hw_clip.h"
#endif
#include "../r_fps.h"
#define R_FAKEFLOORS
#define HWPRECIP
//#define POLYSKY

// ==========================================================================
// the hardware driver object
// ==========================================================================
struct hwdriver_s hwdriver;

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================


static void HWR_AddSprites(sector_t *sec);
static void HWR_ProjectSprite(mobj_t *thing);
#ifdef HWPRECIP
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing);
#endif


void HWR_AddTransparentFloor(lumpnum_t lumpnum, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap);

void HWR_AddTransparentPolyobjectFloor(lumpnum_t lumpnum, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap);

static void CV_grmodellighting_OnChange(void);
static void CV_filtermode_ONChange(void);
static void CV_anisotropic_ONChange(void);
// ==========================================================================
//                                          3D ENGINE COMMANDS & CONSOLE VARS
// ==========================================================================

static CV_PossibleValue_t grfiltermode_cons_t[]= {{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"}, {HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
	{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
	{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
	{HWD_SET_TEXTUREFILTER_MIXED3, "Nearest_Mipmap"},
	{0, NULL}};

CV_PossibleValue_t granisotropicmode_cons_t[] = {{1, "MIN"}, {16, "MAX"}, {0, NULL}};

CV_PossibleValue_t glloadingscreen_cons_t[] = {{0, "OFF"}, {1, "2.1.0-2.1.4"}, {2, "Pre 2.1"},  {0, NULL}};

static CV_PossibleValue_t grfakecontrast_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Smooth"}, {0, NULL}};
static CV_PossibleValue_t grshearing_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Third-person"}, {0, NULL}};

consvar_t cv_grshaders = {"gr_shaders", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grfakecontrast = {"gr_fakecontrast", "Smooth", CV_SAVE, grfakecontrast_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grslopecontrast = {"gr_slopecontrast", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t grmodelinterpolation_cons_t[] = {{0, "Off"}, {1, "Sometimes"}, {2, "Always"}, {0, NULL}};


boolean drawsky = true;

// needs fix: walls are incorrectly clipped one column less
#ifndef NEWCLIP
static consvar_t cv_grclipwalls = {"gr_clipwalls", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
//development variables for diverse uses
static consvar_t cv_gralpha = {"gr_alpha", "160", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_grbeta = {"gr_beta", "0", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

static float HWRWipeCounter = 1.0f;
consvar_t cv_grrounddown = {"gr_rounddown", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// Unfortunately, this can no longer be saved..
consvar_t cv_grfiltermode = {"gr_filtermode", "Nearest", CV_CALL|CV_SAVE, grfiltermode_cons_t,
                             CV_filtermode_ONChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_granisotropicmode = {"gr_anisotropicmode", "1", CV_CALL|CV_SAVE, granisotropicmode_cons_t,
                             CV_anisotropic_ONChange, 0, NULL, NULL, 0, 0, NULL};
//static consvar_t cv_grzbuffer = {"gr_zbuffer", "On", 0, CV_OnOff};
consvar_t cv_grcorrecttricks = {"gr_correcttricks", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grsolvetjoin = {"gr_solvetjoin", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grbatching = {"gr_batching", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grwireframe = {"gr_wireframe", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grmodellighting = {"gr_modellighting", "Off", CV_SAVE|CV_CALL, CV_OnOff, CV_grmodellighting_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grshearing = {"gr_shearing", "Off", CV_SAVE, grshearing_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_glloadingscreen = {"glloadingscreen", "Off", CV_SAVE, glloadingscreen_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_grmd2 = {"gr_md2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grmodelinterpolation = {"gr_modelinterpolation", "Sometimes", CV_SAVE, grmodelinterpolation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grspritebillboarding = {"gr_spritebillboarding", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CV_filtermode_ONChange(void)
{
  if (rendermode == render_opengl)
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}

static void CV_anisotropic_ONChange(void)
{
  if (rendermode == render_opengl)
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_granisotropicmode.value);
}

static void CV_grmodellighting_OnChange(void)
{
  if (rendermode == render_opengl)
		HWD.pfnSetSpecialState(HWD_SET_MODEL_LIGHTING, cv_grmodellighting.value);
}

// ==========================================================================
//                                                               VIEW GLOBALS
// ==========================================================================
// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW ANGLE_90
#define ABS(x) ((x) < 0 ? -(x) : (x))

static angle_t gr_clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
static INT32 gr_viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t gr_xtoviewangle[MAXVIDWIDTH+1];

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

// uncomment to remove the plane rendering
#define DOPLANES
//#define DOWALLS

// test of drawing sky by polygons like in software with visplane, unfortunately
// this doesn't work since we must have z for pixel and z for texture (not like now with z = oow)
//#define POLYSKY

// test change fov when looking up/down but bsp projection messup :(
//#define NOCRAPPYMLOOK

// base values set at SetViewSize
static float gr_basecentery;

float gr_baseviewwindowy, gr_basewindowcentery;
float gr_viewwidth, gr_viewheight; // viewport clipping boundaries (screen coords)
float gr_viewwindowx;

static float gr_centerx, gr_centery;
static float gr_viewwindowy; // top left corner of view window
static float gr_windowcenterx; // center of view window, for projection
static float gr_windowcentery;

static float gr_pspritexscale, gr_pspriteyscale;

static seg_t *gr_curline;
static side_t *gr_sidedef;
static line_t *gr_linedef;
static sector_t *gr_frontsector;
static sector_t *gr_backsector;

// --------------------------------------------------------------------------
//                                              STUFF FOR THE PROJECTION CODE
// --------------------------------------------------------------------------

FTransform atransform;
// duplicates of the main code, set after R_SetupFrame() passed them into sharedstruct,
// copied here for local use
static fixed_t dup_viewx, dup_viewy, dup_viewz;
static angle_t dup_viewangle;

static float gr_viewx, gr_viewy, gr_viewz;
static float gr_viewsin, gr_viewcos;

// Maybe not necessary with the new T&L code (needs to be checked!)
static float gr_viewludsin, gr_viewludcos; // look up down kik test
static float gr_fovlud;


static angle_t gr_aimingangle;
static void HWR_SetTransformAiming(FTransform *trans, player_t *player, boolean skybox);

boolean gr_shadersavailable = true;


// Render stats
ps_metric_t ps_hw_nodesorttime = {0};
ps_metric_t ps_hw_nodedrawtime = {0};
ps_metric_t ps_hw_spritesorttime = {0};
ps_metric_t ps_hw_spritedrawtime = {0};

// Render stats for batching
ps_metric_t ps_hw_numpolys = {0};
ps_metric_t ps_hw_numverts = {0};
ps_metric_t ps_hw_numcalls = {0};
ps_metric_t ps_hw_numshaders = {0};
ps_metric_t ps_hw_numtextures = {0};
ps_metric_t ps_hw_numpolyflags = {0};
ps_metric_t ps_hw_numcolors = {0};
ps_metric_t ps_hw_batchsorttime = {0};
ps_metric_t ps_hw_batchdrawtime = {0};

// ==========================================================================
//   Lighting
// ==========================================================================

static boolean HWR_UseShader(void)
{
	return (cv_grshaders.value && gr_shadersavailable);
}

static boolean HWR_IsWireframeMode(void)
{
	return (cv_grwireframe.value /*&& cv_debug*/); // Wireframe at any time
}

void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap)
{
	RGBA_t poly_color, tint_color, fade_color;

	poly_color.rgba = 0xFFFFFFFF;
	tint_color.rgba = (colormap != NULL) ? (UINT32)colormap->rgba : GL_DEFAULTMIX;
	fade_color.rgba = (colormap != NULL) ? (UINT32)colormap->fadergba : GL_DEFAULTFOG;

	// Crappy backup coloring if you can't do shaders
	if (!HWR_UseShader())
	{
		// be careful, this may get negative for high lightlevel values.
		float tint_alpha, fade_alpha;
		float red, green, blue;

		red = (float)poly_color.s.red;
		green = (float)poly_color.s.green;
		blue = (float)poly_color.s.blue;

		// 48 is just an arbritrary value that looked relatively okay.
		tint_alpha = (float)(sqrt(tint_color.s.alpha) * 48) / 255.0f;

		// 8 is roughly the brightness of the "close" color in Software, and 16 the brightness of the "far" color.
		// 8 is too bright for dark levels, and 16 is too dark for bright levels.
		// 12 is the compromise value. It doesn't look especially good anywhere, but it's the most balanced.
		// (Also, as far as I can tell, fade_color's alpha is actually not used in Software, so we only use light level.)
		fade_alpha = (float)(sqrt(255-light_level) * 12) / 255.0f;

		// Clamp the alpha values
		tint_alpha = min(max(tint_alpha, 0.0f), 1.0f);
		fade_alpha = min(max(fade_alpha, 0.0f), 1.0f);

		red = (tint_color.s.red * tint_alpha) + (red * (1.0f - tint_alpha));
		green = (tint_color.s.green * tint_alpha) + (green * (1.0f - tint_alpha));
		blue = (tint_color.s.blue * tint_alpha) + (blue * (1.0f - tint_alpha));

		red = (fade_color.s.red * fade_alpha) + (red * (1.0f - fade_alpha));
		green = (fade_color.s.green * fade_alpha) + (green * (1.0f - fade_alpha));
		blue = (fade_color.s.blue * fade_alpha) + (blue * (1.0f - fade_alpha));

		poly_color.s.red = (UINT8)red;
		poly_color.s.green = (UINT8)green;
		poly_color.s.blue = (UINT8)blue;
	}

	// Clamp the light level, since it can sometimes go out of the 0-255 range from animations
	light_level = min(max(light_level, 0), 255);

	V_CubeApply(&tint_color.s.red, &tint_color.s.green, &tint_color.s.blue);
	V_CubeApply(&fade_color.s.red, &fade_color.s.green, &fade_color.s.blue);
	Surface->PolyColor.rgba = poly_color.rgba;
	Surface->TintColor.rgba = tint_color.rgba;
	Surface->FadeColor.rgba = fade_color.rgba;
	Surface->LightInfo.light_level = light_level;
	Surface->LightInfo.fade_start = (colormap != NULL) ? colormap->fadestart : 0;
	Surface->LightInfo.fade_end = (colormap != NULL) ? colormap->fadeend : 31;
}



UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap) // Let's see if this can work
{
	RGBA_t realcolor, surfcolor;
	INT32 alpha;

	realcolor.rgba = (colormap != NULL) ? colormap->rgba : GL_DEFAULTMIX;

	if (cv_grshaders.value && gr_shadersavailable)
	{
		surfcolor.s.alpha = (255 - light);
	}
	else
	{
		light = light - (255 - light);

		// Don't go out of bounds
		if (light < 0)
			light = 0;
		else if (light > 255)
			light = 255;

		alpha = (realcolor.s.alpha*255)/25;

		// at 255 brightness, alpha is between 0 and 127, at 0 brightness alpha will always be 255
		surfcolor.s.alpha = (alpha*light) / (2*256) + 255-light;
	}

	return surfcolor.s.alpha;
}

static FUINT HWR_CalcWallLight(FUINT lightnum, fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y)
{
	INT16 finallight = lightnum;

	if (cv_grfakecontrast.value != 0)
	{
		const UINT8 contrast = 8;
		fixed_t extralight = 0;

		if (cv_grfakecontrast.value == 2) // Smooth setting
		{
			extralight = (-(contrast<<FRACBITS) +
			FixedDiv(AngleFixed(R_PointToAngle2(0, 0,
				abs(v1x - v2x),
				abs(v1y - v2y))), 90<<FRACBITS)
			* (contrast * 2)) >> FRACBITS;
		}
		else
		{
			if (v1y == v2y)
				extralight = -contrast;
			else if (v1x == v2x)
				extralight = contrast;
		}

		if (extralight != 0)
		{
			finallight += extralight;

			if (finallight < 0)
				finallight = 0;
			if (finallight > 255)
				finallight = 255;
		}
	}

	return (FUINT)finallight;
}

static FUINT HWR_CalcSlopeLight(FUINT lightnum, angle_t dir, fixed_t delta)
{
	INT16 finallight = lightnum;

	if (cv_grfakecontrast.value != 0 && cv_grslopecontrast.value != 0)
	{
		const UINT8 contrast = 8;
		fixed_t extralight = 0;

		if (cv_grfakecontrast.value == 2) // Smooth setting
		{
			fixed_t dirmul = abs(FixedDiv(AngleFixed(dir) - (180<<FRACBITS), 180<<FRACBITS));

			extralight = -(contrast<<FRACBITS) + (dirmul * (contrast * 2));

			extralight = FixedMul(extralight, delta*4) >> FRACBITS;
		}
		else
		{
			dir = ((dir + ANGLE_45) / ANGLE_90) * ANGLE_90;

			if (dir == ANGLE_180)
				extralight = -contrast;
			else if (dir == 0)
				extralight = contrast;

			if (delta >= FRACUNIT/2)
				extralight *= 2;
		}

		if (extralight != 0)
		{
			finallight += extralight;

			if (finallight < 0)
				finallight = 0;
			if (finallight > 255)
				finallight = 255;
		}
	}

	return (FUINT)finallight;
}

// ==========================================================================
//                                   FLOOR/CEILING GENERATION FROM SUBSECTORS
// ==========================================================================

#ifdef DOPLANES


// HWR_RenderPlane
// Render a floor or ceiling convex polygon
static void HWR_RenderPlane(subsector_t *subsector, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                           FBITFIELD PolyFlags, INT32 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector, UINT8 alpha, boolean fogplane, extracolormap_t *planecolormap)
{
	polyvertex_t *  pv;
	float           height; //constant y for all points on the convex flat polygon
	FOutVector      *v3d;
	INT32             nrPlaneVerts;   //verts original define of convex flat polygon
	INT32             i;
	float           flatxref,flatyref;
	float fflatsize;
	INT32 flatflag;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	FSurfaceInfo    Surf;
	fixed_t tempxs, tempyt;
	pslope_t *slope = NULL;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	INT32 shader = SHADER_DEFAULT;

	(void)fogplane; ///@TODO remove shitty unused variable

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;


	// Get the slope pointer to simplify future code
	if (FOFsector)
	{
		if (FOFsector->f_slope && !isceiling)
			slope = FOFsector->f_slope;
		else if (FOFsector->c_slope && isceiling)
			slope = FOFsector->c_slope;
	}
	else
	{
		if (gr_frontsector->f_slope && !isceiling)
			slope = gr_frontsector->f_slope;
		else if (gr_frontsector->c_slope && isceiling)
			slope = gr_frontsector->c_slope;
	}

	// Set fixedheight to the slope's height from our viewpoint, if we have a slope
	if (slope)
		fixedheight = P_GetZAt(slope, viewx, viewy);


	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;



	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	len = W_LumpLength(lumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			fflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			fflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			fflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			fflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			fflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			fflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			fflatsize = 64.0f;
			flatflag = 63;
			break;
	}

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)(((fixed_t)pv->x & (~flatflag)) / fflatsize);
	flatyref = (float)(((fixed_t)pv->y & (~flatflag)) / fflatsize);

	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatsize;
			angle = FOFsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatsize;
			angle = FOFsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatsize;
			angle = gr_frontsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatsize;
			angle = gr_frontsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}

	if (angle) // Only needs to be done if there's an altered angle
	{

		// This needs to be done so that it scrolls in a different direction after rotation like software
		tempxs = FLOAT_TO_FIXED(scrollx);
		tempyt = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxs, FINESINE(angle)) + FixedMul(tempyt, FINECOSINE(angle))));

		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxs = FLOAT_TO_FIXED(flatxref);
		tempyt = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxs, FINESINE(angle)) + FixedMul(tempyt, FINECOSINE(angle))));
	}

#define SETUP3DVERT(vert, vx, vy) {\
		/* Hurdler: add scrolling texture on floor/ceiling */\
		vert->s = (float)(((vx) / fflatsize) - flatxref + scrollx);\
		vert->t = (float)(flatyref - ((vy) / fflatsize) + scrolly);\
\
		/* Need to rotate before translate */\
		if (angle) /* Only needs to be done if there's an altered angle */\
		{\
			tempxs = FLOAT_TO_FIXED(vert->s);\
			tempyt = FLOAT_TO_FIXED(vert->t);\
			vert->s = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));\
			vert->t = (FIXED_TO_FLOAT(-FixedMul(tempxs, FINESINE(angle)) - FixedMul(tempyt, FINECOSINE(angle))));\
		}\
\
		vert->x = (vx);\
		vert->y = height;\
		vert->z = (vy);\
\
		if (slope)\
		{\
			fixedheight = P_GetZAt(slope, FLOAT_TO_FIXED((vx)), FLOAT_TO_FIXED((vy)));\
			vert->y = FIXED_TO_FLOAT(fixedheight);\
		}\
}

	for (i = 0, v3d = planeVerts; i < (INT32)nrPlaneVerts; i++,v3d++,pv++)
		SETUP3DVERT(v3d, pv->x, pv->y);

	if (slope)
		lightlevel = HWR_CalcSlopeLight(lightlevel, R_PointToAngle2(0, 0, slope->normal.x, slope->normal.y), abs(slope->zdelta));

	HWR_Lighting(&Surf, lightlevel, planecolormap);

if (PolyFlags & (PF_Translucent|PF_Fog))
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		PolyFlags |= PF_Modulated;
	}
	else
		PolyFlags |= PF_Masked|PF_Modulated;

	if (PolyFlags & PF_Fog)
		shader = SHADER_FOG;	// fog shader
	else if (PolyFlags & PF_Ripple)
		shader = SHADER_WATER;	// water shader
	else
		shader = SHADER_FLOOR;	// floor shader



	if (HWR_UseShader())
	{
		if (PolyFlags & PF_Fog)
			shader = SHADER_FOG;
		else if (PolyFlags & PF_Ripple)
			shader = SHADER_WATER;
		else
			shader = SHADER_FLOOR;

		PolyFlags |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, planeVerts, nrPlaneVerts, PolyFlags, shader, false);

	if (subsector)
	{
		// Horizon lines
		FOutVector horizonpts[6];
		float dist, vx, vy;
		float x1, y1, xd, yd;
		UINT8 numplanes, j;
		vertex_t v; // For determining the closest distance from the line to the camera, to split render planes for minimum distortion;

		const float renderdist = 27000.0f; // How far out to properly render the plane
		const float farrenderdist = 32768.0f; // From here, raise plane to horizon level to fill in the line with some texture distortion

		seg_t *line = &segs[subsector->firstline];

		for (i = 0; i < subsector->numlines; i++, line++)
		{
			if (line->linedef->special == 41 && R_PointOnSegSide(dup_viewx, dup_viewy, line) == 0)
			{
				P_ClosestPointOnLine(viewx, viewy, line->linedef, &v);
				dist = FIXED_TO_FLOAT(R_PointToDist(v.x, v.y));

				x1 = ((polyvertex_t *)line->pv1)->x;
				y1 = ((polyvertex_t *)line->pv1)->y;
				xd = ((polyvertex_t *)line->pv2)->x - x1;
				yd = ((polyvertex_t *)line->pv2)->y - y1;

				// Based on the seg length and the distance from the line, split horizon into multiple poly sets to reduce distortion
				dist = sqrtf((xd*xd) + (yd*yd)) / dist / 16.0f;
				if (dist > 100.0f)
					numplanes = 100;
				else
					numplanes = (UINT8)dist + 1;

				for (j = 0; j < numplanes; j++)
				{
					// Left side
					vx = x1 + xd * j / numplanes;
					vy = y1 + yd * j / numplanes;
					SETUP3DVERT((&horizonpts[1]), vx, vy);

					dist = sqrtf(powf(vx - gr_viewx, 2) + powf(vy - gr_viewy, 2));
					vx = (vx - gr_viewx) * renderdist / dist + gr_viewx;
					vy = (vy - gr_viewy) * renderdist / dist + gr_viewy;
					SETUP3DVERT((&horizonpts[0]), vx, vy);

					// Right side
					vx = x1 + xd * (j+1) / numplanes;
					vy = y1 + yd * (j+1) / numplanes;
					SETUP3DVERT((&horizonpts[2]), vx, vy);

					dist = sqrtf(powf(vx - gr_viewx, 2) + powf(vy - gr_viewy, 2));
					vx = (vx - gr_viewx) * renderdist / dist + gr_viewx;
					vy = (vy - gr_viewy) * renderdist / dist + gr_viewy;
					SETUP3DVERT((&horizonpts[3]), vx, vy);

					// Horizon fills
					vx = (horizonpts[0].x - gr_viewx) * farrenderdist / renderdist + gr_viewx;
					vy = (horizonpts[0].z - gr_viewy) * farrenderdist / renderdist + gr_viewy;
					SETUP3DVERT((&horizonpts[5]), vx, vy);
					horizonpts[5].y = gr_viewz;

					vx = (horizonpts[3].x - gr_viewx) * farrenderdist / renderdist + gr_viewx;
					vy = (horizonpts[3].z - gr_viewy) * farrenderdist / renderdist + gr_viewy;
					SETUP3DVERT((&horizonpts[4]), vx, vy);
					horizonpts[4].y = gr_viewz;

					// Draw
					HWR_ProcessPolygon(&Surf, horizonpts, 6, PolyFlags, shader, true);
				}
			}
		}
	}
}

#ifdef POLYSKY
// this don't draw anything it only update the z-buffer so there isn't problem with
// wall/things upper that sky (map12)
static void HWR_RenderSkyPlane(extrasubsector_t *xsub, fixed_t fixedheight)
{
	polyvertex_t *pv;
	float height; //constant y for all points on the convex flat polygon
	FOutVector *v3d;
	INT32 nrPlaneVerts;   //verts original define of convex flat polygon
	INT32 i;

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;

	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3) // not even a triangle?
		return;

	if (nrPlaneVerts > MAXPLANEVERTICES) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %d exceeds max value of %d vertices\n", nrPlaneVerts, MAXPLANEVERTICES);
		return;
	}

	// transform
	v3d = planeVerts;
	for (i = 0; i < nrPlaneVerts; i++,v3d++,pv++)
	{
		v3d->s = 0.0f;
		v3d->t = 0.0f;
		v3d->x = pv->x;
		v3d->y = height;
		v3d->z = pv->y;
	}

	HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts, PF_Invisible|PF_NoTexture|PF_Occlude);
}
#endif //polysky

#endif //doplanes

/*
   wallVerts order is :
		3--2
		| /|
		|/ |
		0--1
*/
#ifdef WALLSPLATS
static void HWR_DrawSegsSplats(FSurfaceInfo * pSurf)
{
	wallsplat_t *splat;
	GLPatch_t *gpatch;
	fixed_t i;
	// seg bbox
	fixed_t segbbox[4];

	M_ClearBox(segbbox);
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y));
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y));

	splat = (wallsplat_t *)gr_curline->linedef->splats;
	for (; splat; splat = splat->next)
	{
		//BP: don't draw splat extern to this seg
		//    this is quick fix best is explain in logboris.txt at 12-4-2000
		if (!M_PointInBox(segbbox,splat->v1.x,splat->v1.y) && !M_PointInBox(segbbox,splat->v2.x,splat->v2.y))
			continue;

		gpatch = W_CachePatchNum(splat->patch, PU_CACHE);
		HWR_GetPatch(gpatch);

		wallVerts[0].x = wallVerts[3].x = FIXED_TO_FLOAT(splat->v1.x);
		wallVerts[0].z = wallVerts[3].z = FIXED_TO_FLOAT(splat->v1.y);
		wallVerts[2].x = wallVerts[1].x = FIXED_TO_FLOAT(splat->v2.x);
		wallVerts[2].z = wallVerts[1].z = FIXED_TO_FLOAT(splat->v2.y);

		i = splat->top;
		if (splat->yoffset)
			i += *splat->yoffset;

		wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(i)+(gpatch->height>>1);
		wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(i)-(gpatch->height>>1);

		wallVerts[3].s = wallVerts[3].t = wallVerts[2].s = wallVerts[0].t = 0.0f;
		wallVerts[1].s = wallVerts[1].t = wallVerts[2].t = wallVerts[0].s = 1.0f;


		switch (splat->flags & SPLATDRAWMODE_MASK)
		{
			case SPLATDRAWMODE_OPAQUE :
				pSurf.PolyColor.s.alpha = 0xff;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_TRANS :
				pSurf.PolyColor.s.alpha = 128;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_SHADE :
				pSurf.PolyColor.s.alpha = 0xff;
				i = PF_Substractive;
				break;
		}

			HWD.pfnSetShader(SHADER_WALL);	// wall shader
			HWD.pfnDrawPolygon(&pSurf, wallVerts, 4, i|PF_Modulated|PF_Decal);
	}
}
#endif




FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf)
{
	switch (transtablenum)
	{
		case tr_trans10 : pSurf->PolyColor.s.alpha = 0xe6;return  PF_Translucent;
		case tr_trans20 : pSurf->PolyColor.s.alpha = 0xcc;return  PF_Translucent;
		case tr_trans30 : pSurf->PolyColor.s.alpha = 0xb3;return  PF_Translucent;
		case tr_trans40 : pSurf->PolyColor.s.alpha = 0x99;return  PF_Translucent;
		case tr_trans50 : pSurf->PolyColor.s.alpha = 0x80;return  PF_Translucent;
		case tr_trans60 : pSurf->PolyColor.s.alpha = 0x66;return  PF_Translucent;
		case tr_trans70 : pSurf->PolyColor.s.alpha = 0x4c;return  PF_Translucent;
		case tr_trans80 : pSurf->PolyColor.s.alpha = 0x33;return  PF_Translucent;
		case tr_trans90 : pSurf->PolyColor.s.alpha = 0x19;return  PF_Translucent;
	}
	return PF_Translucent;
}



static void HWR_AddTransparentWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);


// ==========================================================================
// Wall generation from subsector segs
// ==========================================================================

//
// HWR_ProjectWall
//
static void HWR_ProjectWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blendmode, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	INT32 shader = SHADER_DEFAULT;

	HWR_Lighting(pSurf, lightlevel, wallcolormap);


	if (HWR_UseShader())
	{
		shader = SHADER_WALL;
		blendmode |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(pSurf, wallVerts, 4, blendmode|PF_Modulated|PF_Occlude, shader, false);

#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);
#endif
}

// ==========================================================================
//                                                          BSP, CULL, ETC..
// ==========================================================================

// return the frac from the interception of the clipping line
// (in fact a clipping plane that has a constant, so can clip with simple 2d)
// with the wall segment
//
#ifndef NEWCLIP
static float HWR_ClipViewSegment(INT32 x, polyvertex_t *v1, polyvertex_t *v2)
{
	float num, den;
	float v1x, v1y, v1dx, v1dy, v2dx, v2dy;
	angle_t pclipangle = gr_xtoviewangle[x];

	// a segment of a polygon
	v1x  = v1->x;
	v1y  = v1->y;
	v1dx = (v2->x - v1->x);
	v1dy = (v2->y - v1->y);

	// the clipping line
	pclipangle = pclipangle + dup_viewangle; //back to normal angle (non-relative)
	v2dx = FIXED_TO_FLOAT(FINECOSINE(pclipangle>>ANGLETOFINESHIFT));
	v2dy = FIXED_TO_FLOAT(FINESINE(pclipangle>>ANGLETOFINESHIFT));

	den = v2dy*v1dx - v2dx*v1dy;
	if (den == 0)
		return -1; // parallel

	// calc the frac along the polygon segment,
	//num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
	//num = -v1x * v2dy + v1y * v2dx;
	num = (gr_viewx - v1x)*v2dy + (v1y - gr_viewy)*v2dx;

	return num / den;
}
#endif


// SoM: split up and light walls according to the lightlist.
// This may also include leaving out parts of the wall that can't be seen
static void HWR_SplitWall(sector_t *sector, FOutVector *wallVerts, INT32 texnum, FSurfaceInfo* Surf, INT32 cutflag, ffloor_t *pfloor)
{
	float realtop, realbot, top, bot;
	float pegt, pegb, pegmul;
	float height = 0.0f, bheight = 0.0f;


	float endrealtop, endrealbot, endtop, endbot;
	float endpegt, endpegb, endpegmul;
	float endheight = 0.0f, endbheight = 0.0f;

	fixed_t v1x = FloatToFixed(wallVerts[0].x);
	fixed_t v1y = FloatToFixed(wallVerts[0].z);
	fixed_t v2x = FloatToFixed(wallVerts[1].x);
	fixed_t v2y = FloatToFixed(wallVerts[1].z);


	float diff;

	const UINT8 alpha = Surf->PolyColor.s.alpha;
	FUINT lightnum = HWR_CalcWallLight(sector->lightlevel, v1x, v1y, v2x, v2y);
	extracolormap_t *colormap = NULL;

	realtop = top = wallVerts[3].y;
	realbot = bot = wallVerts[0].y;
	diff = top - bot;

	pegt = wallVerts[3].t;
	pegb = wallVerts[0].t;
	
	// Lactozilla: If both heights of a side lay on the same position, then this wall is a triangle.
	// To avoid division by zero, which would result in a NaN, we check if the vertical difference
	// between the two vertices is not zero.
	if (fpclassify(diff) == FP_ZERO)
		pegmul = 0.0;
	else
		pegmul = (pegb - pegt) / diff;


	endrealtop = endtop = wallVerts[2].y;
	endrealbot = endbot = wallVerts[1].y;
	diff = endtop - endbot;

	endpegt = wallVerts[2].t;
	endpegb = wallVerts[1].t;
	
	if (fpclassify(diff) == FP_ZERO)
		endpegmul = 0.0;
	else
		endpegmul = (endpegb - endpegt) / diff;

	for (INT32 i = 0; i < sector->numlights; i++)
	{
		if (endtop < endrealbot && top < realbot)
			return;

		lightlist_t *list = sector->lightlist;

		if (!(list[i].flags & FF_NOSHADE))
		{
			if (pfloor && (pfloor->flags & FF_FOG))
			{
				lightnum = HWR_CalcWallLight(pfloor->master->frontsector->lightlevel, v1x, v1y, v2x, v2y);
				colormap = pfloor->master->frontsector->extra_colormap;
			}
			else
			{
				lightnum = HWR_CalcWallLight(*list[i].lightlevel, v1x, v1y, v2x, v2y);
				colormap = list[i].extra_colormap;
			}
		}

		boolean solid = false; // Mixed Declarations AAAAA

		if ((sector->lightlist[i].flags & FF_CUTSOLIDS) && !(cutflag & FF_EXTRA))
			solid = true;
		else if ((sector->lightlist[i].flags & FF_CUTEXTRA) && (cutflag & FF_EXTRA))
		{
			if (sector->lightlist[i].flags & FF_EXTRA)
			{
				if ((sector->lightlist[i].flags & (FF_FOG|FF_SWIMMABLE)) == (cutflag & (FF_FOG|FF_SWIMMABLE))) // Only merge with your own types
					solid = true;
			}
			else
				solid = true;
		}
		else
			solid = false;


		height = FixedToFloat(P_GetLightZAt(&list[i], v1x, v1y));
		endheight = FixedToFloat(P_GetLightZAt(&list[i], v2x, v2y));
		if (solid)
		{
			bheight = FixedToFloat(P_GetFFloorBottomZAt(list[i].caster, v1x, v1y));
			endbheight = FixedToFloat(P_GetFFloorBottomZAt(list[i].caster, v2x, v2y));
		}


		if (endheight >= endtop && height >= top)
		{
			if (solid && top > bheight)
				top = bheight;

			if (solid && endtop > endbheight)
				endtop = endbheight;
		}


		if (i + 1 < sector->numlights)
		{
			bheight = FixedToFloat(P_GetLightZAt(&list[i+1], v1x, v1y));
			endbheight = FixedToFloat(P_GetLightZAt(&list[i+1], v2x, v2y));
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}



		// Found a break
		// The heights are clamped to ensure the polygon doesn't cross itself.
		bot = min(max(bheight, realbot), top);
		endbot = min(max(endbheight, endrealbot), endtop);

		Surf->PolyColor.s.alpha = alpha;


		wallVerts[3].t = pegt + ((realtop - top) * pegmul);
		wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
		wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
		wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

		// set top/bottom coords
		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;


		if (cutflag & FF_FOG)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
		else if (cutflag & FF_TRANSLUCENT)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
		else
			HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot)
	if (top <= realbot)
		return;

	Surf->PolyColor.s.alpha = alpha;


	wallVerts[3].t = pegt + ((realtop - top) * pegmul);
	wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
	wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
	wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

	// set top/bottom coords
	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;


	if (cutflag & FF_FOG)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
	else if (cutflag & FF_TRANSLUCENT)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
	else
		HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);
}

// HWR_DrawSkyWalls
// Draw walls into the depth buffer so that anything behind is culled properly
static void HWR_DrawSkyWall(FOutVector *wallVerts, FSurfaceInfo *Surf, fixed_t bottom, fixed_t top)
{
	HWR_SetCurrentTexture(NULL);
	// no texture
	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = 0;
	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = 0;
	// set top/bottom coords
	wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(top); // No real way to find the correct height of this
	wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(bottom); // worldlow/bottom because it needs to cover up the lower thok barrier wall
	HWR_ProjectWall(wallVerts, Surf, PF_Invisible|PF_NoTexture, 255, NULL);
	// PF_Invisible so it's not drawn into the colour buffer
	// PF_NoTexture for no texture
	// PF_Occlude is set in HWR_ProjectWall to draw into the depth buffer
}


// Returns true if the midtexture is visible, and false if... it isn't...
static boolean HWR_BlendMidtextureSurface(FSurfaceInfo *pSurf)
{
	FUINT blendmode = PF_Masked;

	pSurf->PolyColor.s.alpha = 0xFF;

	if (!gr_curline->polyseg)
	{
		// set alpha for transparent walls (new boom and legacy linedef types)
		switch (gr_linedef->special)
		{
				case 900:
					blendmode = HWR_TranstableToAlpha(tr_trans10, pSurf);
					break;
				case 901:
					blendmode = HWR_TranstableToAlpha(tr_trans20, pSurf);
					break;
				case 902:
					blendmode = HWR_TranstableToAlpha(tr_trans30, pSurf);
					break;
				case 903:
					blendmode = HWR_TranstableToAlpha(tr_trans40, pSurf);
					break;
				case 904:
					blendmode = HWR_TranstableToAlpha(tr_trans50, pSurf);
					break;
				case 905:
					blendmode = HWR_TranstableToAlpha(tr_trans60, pSurf);
					break;
				case 906:
					blendmode = HWR_TranstableToAlpha(tr_trans70, pSurf);
					break;
				case 907:
					blendmode = HWR_TranstableToAlpha(tr_trans80, pSurf);
					break;
				case 908:
					blendmode = HWR_TranstableToAlpha(tr_trans90, pSurf);
					break;
				//  Translucent
				case 102:
				case 121:
				case 123:
				case 124:
				case 125:
				case 141:
				case 142:
				case 144:
				case 145:
				case 174:
				case 175:
				case 192:
				case 195:
				case 221:
				case 253:
				case 256:
					blendmode = PF_Translucent;
					break;
				default:
					blendmode = PF_Masked;
					break;
		}
	}
	else if (gr_curline->polyseg->translucency > 0)
	{
		// Polyobject translucency is done differently
		if (gr_curline->polyseg->translucency >= NUMTRANSMAPS) // wall not drawn
			return false;
		else
			blendmode = HWR_TranstableToAlpha(gr_curline->polyseg->translucency, pSurf);
	}

	if (blendmode != PF_Masked && pSurf->PolyColor.s.alpha == 0x00)
		return false;

	pSurf->PolyFlags = blendmode;

	return true;
}

//
// HWR_ProcessSeg
// A portion or all of a wall segment will be drawn, from startfrac to endfrac,
//  where 0 is the start of the segment, 1 the end of the segment
// Anything between means the wall segment has been clipped with solidsegs,
//  reducing wall overdraw to a minimum
//
static void HWR_ProcessSeg(void) // Sort of like GLWall::Process in GZDoom
{
	FOutVector wallVerts[4];
	v2d_t vs, ve; // start, end vertices of 2d line (view from above)

	fixed_t worldtop, worldbottom;
	fixed_t worldhigh = 0, worldlow = 0;
	fixed_t worldtopslope, worldbottomslope;
	fixed_t worldhighslope = 0, worldlowslope = 0;
	fixed_t v1x, v1y, v2x, v2y;


	float cliplow = 0.0f, cliphigh = 0.0f;
	fixed_t h, l; // 3D sides and 2s middle textures
	fixed_t hS, lS;

	FUINT lightnum = 0; // shut up compiler
	extracolormap_t *colormap;
	FSurfaceInfo Surf;

#ifndef NEWCLIP
	if (startfrac > endfrac)
		return;
#endif

	gr_sidedef = gr_curline->sidedef;
	gr_linedef = gr_curline->linedef;

	vs.x = ((polyvertex_t *)gr_curline->pv1)->x;
	vs.y = ((polyvertex_t *)gr_curline->pv1)->y;
	ve.x = ((polyvertex_t *)gr_curline->pv2)->x;
	ve.y = ((polyvertex_t *)gr_curline->pv2)->y;


	v1x = FLOAT_TO_FIXED(vs.x);
	v1y = FLOAT_TO_FIXED(vs.y);
	v2x = FLOAT_TO_FIXED(ve.x);
	v2y = FLOAT_TO_FIXED(ve.y);


#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	if (slope) { \
		end1 = P_GetZAt(slope, v1x, v1y); \
		end2 = P_GetZAt(slope, v2x, v2y); \
	} else \
		end1 = end2 = normalheight;

	SLOPEPARAMS(gr_frontsector->c_slope, worldtop,    worldtopslope,    gr_frontsector->ceilingheight)
	SLOPEPARAMS(gr_frontsector->f_slope, worldbottom, worldbottomslope, gr_frontsector->floorheight)


	// remember vertices ordering
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].x = wallVerts[3].x = vs.x;
	wallVerts[0].z = wallVerts[3].z = vs.y;
	wallVerts[2].x = wallVerts[1].x = ve.x;
	wallVerts[2].z = wallVerts[1].z = ve.y;

	// x offset the texture
	fixed_t texturehpeg = gr_sidedef->textureoffset + gr_curline->offset;
	cliplow = (float)texturehpeg;
	cliphigh = (float)(texturehpeg + (gr_curline->flength*FRACUNIT));

	lightnum = HWR_CalcWallLight(gr_frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
	colormap = gr_frontsector->extra_colormap;

	Surf.PolyColor.s.alpha = 255;
	
	INT32 gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
	GLTexture_t *grTex = NULL;

	// two sided line
	if (gr_backsector)
	{
		INT32 gr_toptexture, gr_bottomtexture;


		SLOPEPARAMS(gr_backsector->c_slope, worldhigh, worldhighslope, gr_backsector->ceilingheight)
		SLOPEPARAMS(gr_backsector->f_slope, worldlow,  worldlowslope,  gr_backsector->floorheight)
#undef SLOPEPARAMS


		// hack to allow height changes in outdoor areas
		// This is what gets rid of the upper textures if there should be sky
		if (gr_frontsector->ceilingpic == skyflatnum &&
			gr_backsector->ceilingpic  == skyflatnum)
		{
			worldtop = worldhigh;
			worldtopslope = worldhighslope;
		}

		gr_toptexture = R_GetTextureNum(gr_sidedef->toptexture);
		gr_bottomtexture = R_GetTextureNum(gr_sidedef->bottomtexture);

		// check TOP TEXTURE
		if ((worldhighslope < worldtopslope || worldhigh < worldtop) && gr_toptexture)
		{
			{
				fixed_t texturevpegtop; // top

				grTex = HWR_GetTexture(gr_toptexture);

				// PEGGING
				if (gr_linedef->flags & ML_DONTPEGTOP)
					texturevpegtop = 0;

				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
				else
					texturevpegtop = gr_backsector->ceilingheight + textureheight[gr_sidedef->toptexture] - gr_frontsector->ceilingheight;

				texturevpegtop += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegtop %= SHORT(textures[gr_toptexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegtop * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegtop + gr_frontsector->ceilingheight - gr_backsector->ceilingheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;


				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[3].t -= (worldtop - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[2].t -= (worldtopslope - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[0].t -= (worldhigh - gr_backsector->ceilingheight) * grTex->scaleY;
					wallVerts[1].t -= (worldhighslope - gr_backsector->ceilingheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGTOP)
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[1].t = (texturevpegtop + worldtopslope - worldhighslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[3].t = wallVerts[0].t - (worldtop - worldhigh) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t - (worldtopslope - worldhighslope) * grTex->scaleY;
				}
			}

			// set top/bottom coords
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldhigh);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldhighslope);

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_toptexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_toptexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}

		// check BOTTOM TEXTURE
		if ((worldlowslope > worldbottomslope || worldlow > worldbottom) && gr_bottomtexture) //only if VISIBLE!!!
		{
			{
				fixed_t texturevpegbottom = 0; // bottom

				grTex = HWR_GetTexture(gr_bottomtexture);

				// PEGGING
				if (!(gr_linedef->flags & ML_DONTPEGBOTTOM))
					texturevpegbottom = 0;
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegbottom = worldbottom - worldlow;
				else
					texturevpegbottom = gr_frontsector->floorheight - gr_backsector->floorheight;
				texturevpegbottom += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegbottom %= SHORT(textures[gr_bottomtexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegbottom * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + gr_backsector->floorheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;


				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[0].t -= (worldbottom - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[1].t -= (worldbottomslope - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[3].t -= (worldlow - gr_backsector->floorheight) * grTex->scaleY;
					wallVerts[2].t -= (worldlowslope - gr_backsector->floorheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGBOTTOM)
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					//wallVerts[3].t = wallVerts[0].t - (worldlow - worldbottom) * grTex->scaleY; // no need, [3] is already this
					wallVerts[2].t = wallVerts[1].t - (worldlowslope - worldbottomslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					wallVerts[1].t = (texturevpegbottom + worldlowslope - worldbottomslope) * grTex->scaleY;
				}
			}

			// set top/bottom coords
			wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_bottomtexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_bottomtexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}
		
		// Render midtexture if there's one. Determine if it's visible first, though
		if (gr_midtexture && HWR_BlendMidtextureSurface(&Surf))
		{
			sector_t *front, *back;
			fixed_t  popentop, popenbottom, polytop, polybottom, lowcut, highcut;
			fixed_t     texturevpeg = 0;
			INT32 repeats;

			if (gr_linedef->frontsector->heightsec != -1)
				front = &sectors[gr_linedef->frontsector->heightsec];
			else
				front = gr_linedef->frontsector;

			if (gr_linedef->backsector->heightsec != -1)
				back = &sectors[gr_linedef->backsector->heightsec];
			else
				back = gr_linedef->backsector;

			if (gr_sidedef->repeatcnt)
				repeats = 1 + gr_sidedef->repeatcnt;
			else if (gr_linedef->flags & ML_EFFECT5)
			{
				fixed_t high, low;

				if (front->ceilingheight > back->ceilingheight)
					high = back->ceilingheight;
				else
					high = front->ceilingheight;

				if (front->floorheight > back->floorheight)
					low = front->floorheight;
				else
					low = back->floorheight;

				repeats = (high - low)/textureheight[gr_sidedef->midtexture];
				if ((high-low)%textureheight[gr_sidedef->midtexture])
					repeats++; // tile an extra time to fill the gap -- Monster Iestyn
			}
			else
				repeats = 1;

			// SoM: a little note: This code re-arranging will
			// fix the bug in Nimrod map02. popentop and popenbottom
			// record the limits the texture can be displayed in.
			// polytop and polybottom, are the ideal (i.e. unclipped)
			// heights of the polygon, and h & l, are the final (clipped)
			// poly coords.


			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			// From CB
			if (gr_curline->polyseg)
			{
				popentop = back->ceilingheight;
				popenbottom = back->floorheight;
			}
			else

            {
				popentop = min(worldtop, worldhigh);
				popenbottom = max(worldbottom, worldlow);
			}


			if (gr_linedef->flags & ML_EFFECT2)
			{
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
				{
					polybottom = max(front->floorheight, back->floorheight) + gr_sidedef->rowoffset;
					polytop = polybottom + textureheight[gr_midtexture]*repeats;
				}
				else
				{
					polytop = min(front->ceilingheight, back->ceilingheight) + gr_sidedef->rowoffset;
					polybottom = polytop - textureheight[gr_midtexture]*repeats;
				}
			}
			else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
			{
				polybottom = popenbottom + gr_sidedef->rowoffset;
				polytop = polybottom + textureheight[gr_midtexture]*repeats;
			}
			else
			{
				polytop = popentop + gr_sidedef->rowoffset;
				polybottom = polytop - textureheight[gr_midtexture]*repeats;
			}
			// CB
			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			if (gr_curline->polyseg)
			{
				lowcut = polybottom;
				highcut = polytop;
			}

			else
			{
				// The cut-off values of a linedef can always be constant, since every line has an absoulute front and or back sector
				lowcut = popenbottom;
				highcut = popentop;
			}

			h = min(highcut, polytop);
			l = max(polybottom, lowcut);

			{
				// PEGGING
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
					texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
				else
					texturevpeg = polytop - h;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
			}

			// set top/bottom coords
			// Take the texture peg into account, rather than changing the offsets past
			// where the polygon might not be.
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(h);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(l);


			// Correct to account for slopes
			{
				fixed_t midtextureslant;

				if (gr_linedef->flags & ML_EFFECT2)
					midtextureslant = 0;
				else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
					midtextureslant = worldlow < worldbottom
							  ? worldbottomslope-worldbottom
							  : worldlowslope-worldlow;
				else
					midtextureslant = worldtop < worldhigh
							  ? worldtopslope-worldtop
							  : worldhighslope-worldhigh;

				polytop += midtextureslant;
				polybottom += midtextureslant;

				highcut += worldtop < worldhigh
						 ? worldtopslope-worldtop
						 : worldhighslope-worldhigh;
				lowcut += worldlow < worldbottom
						? worldbottomslope-worldbottom
						: worldlowslope-worldlow;

				// Texture stuff
				h = min(highcut, polytop);
				l = max(polybottom, lowcut);

				{
					// PEGGING
					if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
						texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
					else
						texturevpeg = polytop - h;
					wallVerts[2].t = texturevpeg * grTex->scaleY;
					wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				}

				wallVerts[2].y = FIXED_TO_FLOAT(h);
				wallVerts[1].y = FIXED_TO_FLOAT(l);
			}


			// TODO: Actually use the surface's flags so that I don't have to do this
			FUINT blendmode = Surf.PolyFlags;

			if (gr_curline->polyseg && gr_curline->polyseg->translucency > 0)
			{
				if (gr_curline->polyseg->translucency >= NUMTRANSMAPS) // wall not drawn
				{
					Surf.PolyColor.s.alpha = 0x00; // This shouldn't draw anything regardless of blendmode
					blendmode = PF_Masked;
				}
				else
					blendmode = HWR_TranstableToAlpha(gr_curline->polyseg->translucency, &Surf);
			}

			if (gr_frontsector->numlights)
			{
				if (!(blendmode & PF_Masked))
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_TRANSLUCENT, NULL);
				else
				{
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
				}
			}
			else if (!(blendmode & PF_Masked))
				HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, blendmode, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, blendmode, lightnum, colormap);

		}

		// Isn't this just the most lovely mess
		if (!gr_curline->polyseg) // Don't do it for polyobjects
		{
			if (gr_frontsector->ceilingpic == skyflatnum || gr_backsector->ceilingpic == skyflatnum)
			{
				fixed_t depthwallheight;

				if (!gr_sidedef->toptexture || (gr_frontsector->ceilingpic == skyflatnum && gr_backsector->ceilingpic == skyflatnum)) // when both sectors are sky, the top texture isn't drawn
					depthwallheight = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
				else
					depthwallheight = gr_frontsector->ceilingheight > gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;

				if (gr_frontsector->ceilingheight-gr_frontsector->floorheight <= 0) // current sector is a thok barrier
				{
					if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is also a thok barrier
					{
						if (!gr_sidedef->bottomtexture) // Only extend further down if there's no texture
							HWR_DrawSkyWall(wallVerts, &Surf, worldbottom < worldlow ? worldbottom : worldlow, INT32_MAX);
						else
							HWR_DrawSkyWall(wallVerts, &Surf, worldbottom > worldlow ? worldbottom : worldlow, INT32_MAX);
					}
					// behind sector is not a thok barrier
					else if (gr_backsector->ceilingheight <= gr_frontsector->ceilingheight) // behind sector ceiling is lower or equal to current sector
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
						// gr_front/backsector heights need to be used here because of the worldtop being set to worldhigh earlier on
				}
				else if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is a thok barrier, current sector is not
				{
					if (gr_backsector->ceilingheight >= gr_frontsector->ceilingheight // thok barrier ceiling height is equal to or greater than current sector ceiling height
						|| gr_backsector->floorheight <= gr_frontsector->floorheight // thok barrier ceiling height is equal to or less than current sector floor height
						|| gr_backsector->ceilingpic != skyflatnum) // thok barrier is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
				}
				else // neither sectors are thok barriers
				{
					if ((gr_backsector->ceilingheight < gr_frontsector->ceilingheight && !gr_sidedef->toptexture) // no top texture and sector behind is lower
						|| gr_backsector->ceilingpic != skyflatnum) // behind sector is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
				}
			}
			// And now for sky floors!
			if (gr_frontsector->floorpic == skyflatnum || gr_backsector->floorpic == skyflatnum)
			{
				fixed_t depthwallheight;

				if (!gr_sidedef->bottomtexture)
					depthwallheight = worldbottom > worldlow ? worldbottom : worldlow;
				else
					depthwallheight = worldbottom < worldlow ? worldbottom : worldlow;

				if (gr_frontsector->ceilingheight-gr_frontsector->floorheight <= 0) // current sector is a thok barrier
				{
					if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is also a thok barrier
					{
						if (!gr_sidedef->toptexture) // Only extend up if there's no texture
							HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldtop > worldhigh ? worldtop : worldhigh);
						else
							HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldtop < worldhigh ? worldtop : worldhigh);
					}
					// behind sector is not a thok barrier
					else if (gr_backsector->floorheight >= gr_frontsector->floorheight) // behind sector floor is greater or equal to current sector
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
				else if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is a thok barrier, current sector is not
				{
					if (gr_backsector->floorheight <= gr_frontsector->floorheight // thok barrier floor height is equal to or less than current sector floor height
						|| gr_backsector->ceilingheight >= gr_frontsector->ceilingheight // thok barrier floor height is equal to or greater than current sector ceiling height
						|| gr_backsector->floorpic != skyflatnum) // thok barrier is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
				else // neither sectors are thok barriers
				{
					if ((gr_backsector->floorheight > gr_frontsector->floorheight && !gr_sidedef->bottomtexture) // no bottom texture and sector behind is higher
						|| gr_backsector->floorpic != skyflatnum) // behind sector is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
			}
		}
	}
	else
	{
		// Single sided line... Deal only with the middletexture (if one exists)
		gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
		if (gr_midtexture)
		{
			{
				fixed_t     texturevpeg;
				// PEGGING
				if ((gr_linedef->flags & (ML_DONTPEGBOTTOM|ML_EFFECT2)) == (ML_DONTPEGBOTTOM|ML_EFFECT2))
					texturevpeg = gr_frontsector->floorheight + textureheight[gr_sidedef->midtexture] - gr_frontsector->ceilingheight + gr_sidedef->rowoffset;
				else

				if (gr_linedef->flags & ML_DONTPEGBOTTOM)
					texturevpeg = worldbottom + textureheight[gr_sidedef->midtexture] - worldtop + gr_sidedef->rowoffset;
				else
					// top of texture at top
					texturevpeg = gr_sidedef->rowoffset;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpeg + gr_frontsector->ceilingheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;


				// Texture correction for slopes
				if (gr_linedef->flags & ML_EFFECT2) {
					wallVerts[3].t += (gr_frontsector->ceilingheight - worldtop) * grTex->scaleY;
					wallVerts[2].t += (gr_frontsector->ceilingheight - worldtopslope) * grTex->scaleY;
					wallVerts[0].t += (gr_frontsector->floorheight - worldbottom) * grTex->scaleY;
					wallVerts[1].t += (gr_frontsector->floorheight - worldbottomslope) * grTex->scaleY;
				} else if (gr_linedef->flags & ML_DONTPEGBOTTOM) {
					wallVerts[3].t = wallVerts[0].t + (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t + (worldbottomslope-worldtopslope) * grTex->scaleY;
				} else {
					wallVerts[0].t = wallVerts[3].t - (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[1].t = wallVerts[2].t - (worldbottomslope-worldtopslope) * grTex->scaleY;
				}
			}
			//Set textures properly on single sided walls that are sloped
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);
			// I don't think that solid walls can use translucent linedef types...
			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
			else
			{
				if (grTex->mipmap.flags & TF_TRANSPARENT)
					HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, PF_Environment, false, lightnum, colormap);
				else
					HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
			}
		}

		if (!gr_curline->polyseg)
		{
			if (gr_frontsector->ceilingpic == skyflatnum) // It's a single-sided line with sky for its sector
				HWR_DrawSkyWall(wallVerts, &Surf, worldtop, INT32_MAX);
			if (gr_frontsector->floorpic == skyflatnum)
				HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldbottom);
		}
	}


	//Hurdler: 3d-floors test
#ifdef R_FAKEFLOORS
	if (gr_frontsector && gr_backsector && gr_frontsector->tag != gr_backsector->tag && (gr_backsector->ffloors || gr_frontsector->ffloors))
	{
		ffloor_t * rover;
		fixed_t    highcut = 0, lowcut = 0;

		INT32 texnum;
		line_t * newline = NULL; // Multi-Property FOF

        ///TODO add slope support (fixing cutoffs, proper wall clipping) - maybe just disable highcut/lowcut if either sector or FOF has a slope
        ///     to allow fun plane intersecting in OGL? But then people would abuse that and make software look bad. :C
		highcut = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
		lowcut = gr_frontsector->floorheight > gr_backsector->floorheight ? gr_frontsector->floorheight : gr_backsector->floorheight;

		if (gr_backsector->ffloors)
		{
			for (rover = gr_backsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES) || (rover->flags & FF_INVERTSIDES))
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}


				h  = *rover->t_slope ? P_GetZAt(*rover->t_slope, v1x, v1y) : *rover->topheight;
				hS = *rover->t_slope ? P_GetZAt(*rover->t_slope, v2x, v2y) : *rover->topheight;
				l  = *rover->b_slope ? P_GetZAt(*rover->b_slope, v1x, v1y) : *rover->bottomheight;
				lS = *rover->b_slope ? P_GetZAt(*rover->b_slope, v2x, v2y) : *rover->bottomheight;
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{
					fixed_t texturevpeg;
					boolean attachtobottom = false;
					boolean slopeskew = false; // skew FOF walls with slopes?

					// Wow, how was this missing from OpenGL for so long?
					// ...Oh well, anyway, Lower Unpegged now changes pegging of FOFs like in software
					// -- Monster Iestyn 26/06/18
					if (newline)
					{
						texturevpeg = sides[newline->sidenum[0]].rowoffset;
						attachtobottom = !!(newline->flags & ML_DONTPEGBOTTOM);
						slopeskew = !!(newline->flags & ML_DONTPEGTOP);
					}
					else
					{
						texturevpeg = sides[rover->master->sidenum[0]].rowoffset;
						attachtobottom = !!(gr_linedef->flags & ML_DONTPEGBOTTOM);
						slopeskew = !!(rover->master->flags & ML_DONTPEGTOP);
					}

					grTex = HWR_GetTexture(texnum);


					if (!slopeskew) // no skewing
					{
						if (attachtobottom)
							texturevpeg -= *rover->topheight - *rover->bottomheight;
						wallVerts[3].t = (*rover->topheight - h + texturevpeg) * grTex->scaleY;
						wallVerts[2].t = (*rover->topheight - hS + texturevpeg) * grTex->scaleY;
						wallVerts[0].t = (*rover->topheight - l + texturevpeg) * grTex->scaleY;
						wallVerts[1].t = (*rover->topheight - lS + texturevpeg) * grTex->scaleY;
					}
					else
					{
						if (!attachtobottom) // skew by top
						{
							wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
							wallVerts[0].t = (h - l + texturevpeg) * grTex->scaleY;
							wallVerts[1].t = (hS - lS + texturevpeg) * grTex->scaleY;
						}
						else // skew by bottom
						{
							wallVerts[0].t = wallVerts[1].t = texturevpeg * grTex->scaleY;
							wallVerts[3].t = wallVerts[0].t - (h - l) * grTex->scaleY;
							wallVerts[2].t = wallVerts[1].t - (hS - lS) * grTex->scaleY;
						}
					}

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}
				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

					lightnum = HWR_CalcWallLight(rover->master->frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
					colormap = rover->master->frontsector->extra_colormap;

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel, rover->master->frontsector->extra_colormap);

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.PolyColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}

		if (gr_frontsector->ffloors) // Putting this seperate should allow 2 FOF sectors to be connected without too many errors? I think?
		{
			for (rover = gr_frontsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_ALLSIDES))
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}
				 //backsides
				h  = *rover->t_slope ? P_GetZAt(*rover->t_slope, v1x, v1y) : *rover->topheight;
				hS = *rover->t_slope ? P_GetZAt(*rover->t_slope, v2x, v2y) : *rover->topheight;
				l  = *rover->b_slope ? P_GetZAt(*rover->b_slope, v1x, v1y) : *rover->bottomheight;
				lS = *rover->b_slope ? P_GetZAt(*rover->b_slope, v2x, v2y) : *rover->bottomheight;
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else
				{
					grTex = HWR_GetTexture(texnum);

					if (newline)
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset)) * grTex->scaleY;
					}
					else
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset)) * grTex->scaleY;
					}

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}

				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

					lightnum = HWR_CalcWallLight(rover->master->frontsector->lightlevel, vs.x, vs.y, ve.x, ve.y);
					colormap = rover->master->frontsector->extra_colormap;

					Surf.PolyColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel, rover->master->frontsector->extra_colormap);

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.PolyColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}
	}
#endif
//Hurdler: end of 3d-floors test
}

// From PrBoom:
//
// e6y: Check whether the player can look beyond this line
//
#ifdef NEWCLIP
boolean checkforemptylines = true;
// Don't modify anything here, just check
// Kalaron: Modified for sloped linedefs
static boolean CheckClip(seg_t * seg, sector_t * afrontsector, sector_t * abacksector)
{
	fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
	fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends

	// GZDoom method of sloped line clipping


	if (afrontsector->f_slope || afrontsector->c_slope || abacksector->f_slope || abacksector->c_slope)
	{
		fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
		v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x);
		v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y);
		v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x);
		v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y);
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		if (slope) { \
			end1 = P_GetZAt(slope, v1x, v1y); \
			end2 = P_GetZAt(slope, v2x, v2y); \
		} else \
			end1 = end2 = normalheight;

		SLOPEPARAMS(afrontsector->f_slope, frontf1, frontf2, afrontsector->floorheight)
		SLOPEPARAMS(afrontsector->c_slope, frontc1, frontc2, afrontsector->ceilingheight)
		SLOPEPARAMS( abacksector->f_slope, backf1,  backf2,  abacksector->floorheight)
		SLOPEPARAMS( abacksector->c_slope, backc1,  backc2,  abacksector->ceilingheight)
#undef SLOPEPARAMS
	}
	else
	{
		frontf1 = frontf2 = afrontsector->floorheight;
		frontc1 = frontc2 = afrontsector->ceilingheight;
		backf1 = backf2 = abacksector->floorheight;
		backc1 = backc2 = abacksector->ceilingheight;
	}

	// now check for closed sectors!
	if (backc1 <= frontf1 && backc2 <= frontf2)
	{
		checkforemptylines = false;
		if (!seg->sidedef->toptexture)
			return false;

		if (abacksector->ceilingpic == skyflatnum && afrontsector->ceilingpic == skyflatnum)
			return false;

		return true;
	}

	if (backf1 >= frontc1 && backf2 >= frontc2)
	{
		checkforemptylines = false;
		if (!seg->sidedef->bottomtexture)
			return false;

		// properly render skies (consider door "open" if both floors are sky):
		if (abacksector->ceilingpic == skyflatnum && afrontsector->ceilingpic == skyflatnum)
			return false;

		return true;
	}

	if (backc1 <= backf1 && backc2 <= backf2)
	{
		checkforemptylines = false;
		// preserve a kind of transparent door/lift special effect:
		if (backc1 < frontc1 || backc2 < frontc2)
		{
			if (!seg->sidedef->toptexture)
				return false;
		}
		if (backf1 > frontf1 || backf2 > frontf2)
		{
			if (!seg->sidedef->bottomtexture)
				return false;
		}
		if (abacksector->ceilingpic == skyflatnum && afrontsector->ceilingpic == skyflatnum)
			return false;

		if (abacksector->floorpic == skyflatnum && afrontsector->floorpic == skyflatnum)
			return false;

		return true;
	}

	if (backc1 != frontc1 || backc2 != frontc2
		|| backf1 != frontf1 || backf2 != frontf2)
		{
			checkforemptylines = false;
			return false;
		}

	return false;
}
#else
//Hurdler: just like in r_bsp.c
#if 1
#define MAXSEGS         MAXVIDWIDTH/2+1
#else
//Alam_GBC: Or not (may cause overflow)
#define MAXSEGS         128
#endif

// hw_newend is one past the last valid seg
static cliprange_t *   hw_newend;
static cliprange_t     gr_solidsegs[MAXSEGS];


static void printsolidsegs(void)
{
	cliprange_t *       start;
	if (!hw_newend || cv_grbeta.value != 2)
		return;
	for (start = gr_solidsegs;start != hw_newend;start++)
	{
		CONS_Debug(DBG_RENDER, "%d-%d|",start->first,start->last);
	}
	CONS_Debug(DBG_RENDER, "\n\n");
}

//
//
//
static void HWR_ClipSolidWallSegment(INT32 first, INT32 last)
{
	cliprange_t *next, *start;
	float lowfrac, highfrac;
	boolean poorhack = false;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start),
			//  so insert a new clippost.
			HWR_StoreWallRange(first, last);

			next = hw_newend;
			hw_newend++;

			while (next != start)
			{
				*next = *(next-1);
				next--;
			}

			next->first = first;
			next->last = last;
			printsolidsegs();
			return;
		}

		// There is a fragment above *start.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first, last);
			poorhack = true;
		}
		else
		{
			highfrac = HWR_ClipViewSegment(start->first+1, (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(0, highfrac);
		}
		// Now adjust the clip size.
		start->first = first;
	}

	// Bottom contained in start?
	if (last <= start->last)
	{
		printsolidsegs();
		return;
	}
	next = start;
	while (last >= (next+1)->first-1)
	{
		// There is a fragment between two posts.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(next->last-1, (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			highfrac = HWR_ClipViewSegment((next+1)->first+1, (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, highfrac);
		}
		next++;

		if (last <= next->last)
		{
			// Bottom is contained in next.
			// Adjust the clip size.
			start->last = next->last;
			goto crunch;
		}
	}

	if (first == next->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
	// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(next->last-1, (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}

	// Adjust the clip size.
	start->last = last;

	// Remove start+1 to next from the clip list,
	// because start now covers their area.
crunch:
	if (next == start)
	{
		printsolidsegs();
		// Post just extended past the bottom of one post.
		return;
	}


	while (next++ != hw_newend)
	{
		// Remove a post.
		*++start = *next;
	}

	hw_newend = start;
	printsolidsegs();
}

//
//  handle LineDefs with upper and lower texture (windows)
//
static void HWR_ClipPassWallSegment(INT32 first, INT32 last)
{
	cliprange_t *start;
	float lowfrac, highfrac;
	//to allow noclipwalls but still solidseg reject of non-visible walls
	boolean poorhack = false;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start).
			HWR_StoreWallRange(0, 1);
			return;
		}

		// There is a fragment above *start.
		if (!cv_grclipwalls.value)
		{	//20/08/99: Changed by Hurdler (taken from faB's code)
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			highfrac = HWR_ClipViewSegment(min(start->first + 1,
				start->last), (polyvertex_t *)gr_curline->pv1,
				(polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(0, highfrac);
		}
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	while (last >= (start+1)->first-1)
	{
		// There is a fragment between two posts.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(max(start->last-1,start->first), (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			highfrac = HWR_ClipViewSegment(min((start+1)->first+1,(start+1)->last), (polyvertex_t *)gr_curline->pv1, (polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, highfrac);
		}
		start++;

		if (last <= start->last)
			return;
	}

	if (first == start->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
		// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0,1);
			poorhack = true;
		}
		else
		{
			lowfrac = HWR_ClipViewSegment(max(start->last - 1,
				start->first), (polyvertex_t *)gr_curline->pv1,
				(polyvertex_t *)gr_curline->pv2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}
}

// --------------------------------------------------------------------------
//  HWR_ClipToSolidSegs check if it is hide by wall (solidsegs)
// --------------------------------------------------------------------------
static boolean HWR_ClipToSolidSegs(INT32 first, INT32 last)
{
	cliprange_t * start;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
		return true;

	// Bottom contained in start?
	if (last <= start->last)
		return false;

	return true;
}

//
// HWR_ClearClipSegs
//
static void HWR_ClearClipSegs(void)
{
	gr_solidsegs[0].first = -0x7fffffff;
	gr_solidsegs[0].last = -1;
	gr_solidsegs[1].first = vid.width; //viewwidth;
	gr_solidsegs[1].last = 0x7fffffff;
	hw_newend = gr_solidsegs+2;
}
#endif // NEWCLIP

// -----------------+
// HWR_AddLine      : Clips the given segment and adds any visible pieces to the line list.
// Notes            : gr_cursectorlight is set to the current subsector -> sector -> light value
//                  : (it may be mixed with the wall's own flat colour in the future ...)
// -----------------+
static void HWR_AddLine(seg_t * line)
{
	angle_t angle1, angle2;
#ifndef NEWCLIP
	INT32 x1, x2;
	angle_t span, tspan;
#endif

	// SoM: Backsector needs to be run through R_FakeFlat
	static sector_t tempsec;

	fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	gr_curline = line;

	v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->x);
	v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv1)->y);
	v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->x);
	v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->pv2)->y);

	// OPTIMIZE: quickly reject orthogonal back sides.
	angle1 = R_PointToAngle64(v1x, v1y);
	angle2 = R_PointToAngle64(v2x, v2y);

#ifdef NEWCLIP
	 // PrBoom: Back side, i.e. backface culling - read: endAngle >= startAngle!
	if (angle2 - angle1 < ANGLE_180)
		return;

	// PrBoom: use REAL clipping math YAYYYYYYY!!!

	if (!gld_clipper_SafeCheckRange(angle2, angle1))
    {
		return;
    }

	checkforemptylines = true;
#else
	// Clip to view edges.
	span = angle1 - angle2;

	// backface culling : span is < ANGLE_180 if ang1 > ang2 : the seg is facing
	if (span >= ANGLE_180)
		return;

	// Global angle needed by segcalc.
	//rw_angle1 = angle1;
	angle1 -= dup_viewangle;
	angle2 -= dup_viewangle;

	tspan = angle1 + gr_clipangle;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle1 = gr_clipangle;
	}
	tspan = gr_clipangle - angle2;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle2 = (angle_t)-(signed)gr_clipangle;
	}

#if 0
	{
		float fx1,fx2,fy1,fy2;
		//BP: test with a better projection than viewangletox[R_PointToAngle(angle)]
		// do not enable this at release 4 mul and 2 div
		fx1 = ((polyvertex_t *)(line->pv1))->x-gr_viewx;
		fy1 = ((polyvertex_t *)(line->pv1))->y-gr_viewy;
		fy2 = (fx1 * gr_viewcos + fy1 * gr_viewsin);
		if (fy2 < 0)
			// the point is back
			fx1 = 0;
		else
			fx1 = gr_windowcenterx + (fx1 * gr_viewsin - fy1 * gr_viewcos) * gr_centerx / fy2;

		fx2 = ((polyvertex_t *)(line->pv2))->x-gr_viewx;
		fy2 = ((polyvertex_t *)(line->pv2))->y-gr_viewy;
		fy1 = (fx2 * gr_viewcos + fy2 * gr_viewsin);
		if (fy1 < 0)
			// the point is back
			fx2 = vid.width;
		else
			fx2 = gr_windowcenterx + (fx2 * gr_viewsin - fy2 * gr_viewcos) * gr_centerx / fy1;

		x1 = fx1+0.5f;
		x2 = fx2+0.5f;
	}
#else
	// The seg is in the view range,
	// but not necessarily visible.
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;

	x1 = gr_viewangletox[angle1];
	x2 = gr_viewangletox[angle2];
#endif
	// Does not cross a pixel?
//	if (x1 == x2)
/*	{
		// BP: HERE IS THE MAIN PROBLEM !
		//CONS_Debug(DBG_RENDER, "tineline\n");
		return;
	}
*/
#endif

	gr_backsector = line->backsector;

#ifdef NEWCLIP
	if (!line->backsector)
    {
		gld_clipper_SafeAddClipRange(angle2, angle1);
    }
    else
    {
		gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);

		if ((gr_backsector->ceilingpic == skyflatnum && gr_frontsector->ceilingpic == skyflatnum) && (gr_backsector->floorpic == skyflatnum && gr_frontsector->floorpic == skyflatnum)) // everything's sky? let's save us a bit of time then
		{
			if (!line->polyseg &&
				!line->sidedef->midtexture
				&& ((!gr_frontsector->ffloors && !gr_backsector->ffloors)
					|| gr_frontsector->tag == gr_backsector->tag))
				return; // line is empty, don't even bother
			// treat like wide open window instead
			HWR_ProcessSeg(); // Doesn't need arguments because they're defined globally :D
			return;
		}

		if (CheckClip(line, gr_frontsector, gr_backsector))
		{
			gld_clipper_SafeAddClipRange(angle2, angle1);
			checkforemptylines = false;
		}
		// Reject empty lines used for triggers and special events.
		// Identical floor and ceiling on both sides,
		//  identical light levels on both sides,
		//  and no middle texture.
		if (checkforemptylines && R_IsEmptyLine(line, gr_frontsector, gr_backsector))
			return;
    }

	HWR_ProcessSeg(); // Doesn't need arguments because they're defined globally :D
	return;
#else
	// Single sided line?
	if (!gr_backsector)
		goto clipsolid;

	gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);


	if (gr_frontsector->f_slope || gr_frontsector->c_slope || gr_backsector->f_slope || gr_backsector->c_slope)
	{
		fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
		fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		if (slope) { \
			end1 = P_GetZAt(slope, v1x, v1y); \
			end2 = P_GetZAt(slope, v2x, v2y); \
		} else \
			end1 = end2 = normalheight;

		SLOPEPARAMS(gr_frontsector->f_slope, frontf1, frontf2, gr_frontsector->floorheight)
		SLOPEPARAMS(gr_frontsector->c_slope, frontc1, frontc2, gr_frontsector->ceilingheight)
		SLOPEPARAMS( gr_backsector->f_slope, backf1,  backf2,  gr_backsector->floorheight)
		SLOPEPARAMS( gr_backsector->c_slope, backc1,  backc2,  gr_backsector->ceilingheight)
#undef SLOPEPARAMS

		// Closed door.
		if ((backc1 <= frontf1 && backc2 <= frontf2)
			|| (backf1 >= frontc1 && backf2 >= frontc2))
		{
			goto clipsolid;
		}

		// Check for automap fix.
		if (backc1 <= backf1 && backc2 <= backf2
		&& ((backc1 >= frontc1 && backc2 >= frontc2) || gr_curline->sidedef->toptexture)
		&& ((backf1 <= frontf1 && backf2 >= frontf2) || gr_curline->sidedef->bottomtexture)
		&& (gr_backsector->ceilingpic != skyflatnum || gr_frontsector->ceilingpic != skyflatnum))
			goto clipsolid;

		// Window.
		if (backc1 != frontc1 || backc2 != frontc2
			|| backf1 != frontf1 || backf2 != frontf2)
		{
			goto clippass;
		}
	}
	else
	{
		// Closed door.
		if (gr_backsector->ceilingheight <= gr_frontsector->floorheight ||
			gr_backsector->floorheight >= gr_frontsector->ceilingheight)
			goto clipsolid;

		// Check for automap fix.
		if (gr_backsector->ceilingheight <= gr_backsector->floorheight
		&& ((gr_backsector->ceilingheight >= gr_frontsector->ceilingheight) || gr_curline->sidedef->toptexture)
		&& ((gr_backsector->floorheight <= gr_backsector->floorheight) || gr_curline->sidedef->bottomtexture)
		&& (gr_backsector->ceilingpic != skyflatnum || gr_frontsector->ceilingpic != skyflatnum))
			goto clipsolid;

		// Window.
		if (gr_backsector->ceilingheight != gr_frontsector->ceilingheight ||
			gr_backsector->floorheight != gr_frontsector->floorheight)
			goto clippass;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides,
	//  identical light levels on both sides,
	//  and no middle texture.
	if (R_IsEmptyLine(gr_curline, gr_frontsector, gr_backsector))
		return;

clippass:
	if (x1 == x2)
		{  x2++;x1 -= 2; }
	HWR_ClipPassWallSegment(x1, x2-1);
	return;

clipsolid:
	if (x1 == x2)
		goto clippass;
	HWR_ClipSolidWallSegment(x1, x2-1);
#endif
}

// HWR_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
// modified to use local variables

static boolean HWR_CheckBBox(const fixed_t *bspcoord)
{
	INT32 boxpos;
	fixed_t px1, py1, px2, py2;
	angle_t angle1, angle2;
#ifndef NEWCLIP
	INT32 sx1, sx2;
	angle_t span, tspan;
#endif

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (dup_viewx <= bspcoord[BOXLEFT])
		boxpos = 0;
	else if (dup_viewx < bspcoord[BOXRIGHT])
		boxpos = 1;
	else
		boxpos = 2;

	if (dup_viewy >= bspcoord[BOXTOP])
		boxpos |= 0;
	else if (dup_viewy > bspcoord[BOXBOTTOM])
		boxpos |= 1<<2;
	else
		boxpos |= 2<<2;

	if (boxpos == 5)
		return true;

	px1 = bspcoord[checkcoord[boxpos][0]];
	py1 = bspcoord[checkcoord[boxpos][1]];
	px2 = bspcoord[checkcoord[boxpos][2]];
	py2 = bspcoord[checkcoord[boxpos][3]];

#ifdef NEWCLIP
	angle1 = R_PointToAngle64(px1, py1);
	angle2 = R_PointToAngle64(px2, py2);
	return gld_clipper_SafeCheckRange(angle2, angle1);
#else
	// check clip list for an open space
	angle1 = R_PointToAngle2(dup_viewx>>1, dup_viewy>>1, px1>>1, py1>>1) - dup_viewangle;
	angle2 = R_PointToAngle2(dup_viewx>>1, dup_viewy>>1, px2>>1, py2>>1) - dup_viewangle;

	span = angle1 - angle2;

	// Sitting on a line?
	if (span >= ANGLE_180)
		return true;

	tspan = angle1 + gr_clipangle;

	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle1 = gr_clipangle;
	}
	tspan = gr_clipangle - angle2;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle2 = (angle_t)-(signed)gr_clipangle;
	}

	// Find the first clippost
	//  that touches the source post
	//  (adjacent pixels are touching).
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	sx1 = gr_viewangletox[angle1];
	sx2 = gr_viewangletox[angle2];

	// Does not cross a pixel.
	if (sx1 == sx2)
		return false;

	return HWR_ClipToSolidSegs(sx1, sx2 - 1);
#endif
}



//
// HWR_AddPolyObjectSegs
//
// haleyjd 02/19/06
// Adds all segs in all polyobjects in the given subsector.
// Modified for hardware rendering.
//
static inline void HWR_AddPolyObjectSegs(void)
{
	size_t i, j;
	seg_t *gr_fakeline = Z_Calloc(sizeof(seg_t), PU_STATIC, NULL);
	polyvertex_t *pv1 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);
	polyvertex_t *pv2 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);

	// Sort through all the polyobjects
	for (i = 0; i < numpolys; ++i)
	{
		// Render the polyobject's lines
		for (j = 0; j < po_ptrs[i]->segCount; ++j)
		{
			// Copy the info of a polyobject's seg, then convert it to OpenGL floating point
			M_Memcpy(gr_fakeline, po_ptrs[i]->segs[j], sizeof(seg_t));

			// Now convert the line to float and add it to be rendered
			pv1->x = FIXED_TO_FLOAT(gr_fakeline->v1->x);
			pv1->y = FIXED_TO_FLOAT(gr_fakeline->v1->y);
			pv2->x = FIXED_TO_FLOAT(gr_fakeline->v2->x);
			pv2->y = FIXED_TO_FLOAT(gr_fakeline->v2->y);

			gr_fakeline->pv1 = pv1;
			gr_fakeline->pv2 = pv2;

			HWR_AddLine(gr_fakeline);
		}
	}

	// Free temporary data no longer needed
	Z_Free(pv2);
	Z_Free(pv1);
	Z_Free(gr_fakeline);
}


static void HWR_RenderPolyObjectPlane(polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
									FBITFIELD blendmode, UINT8 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector,
									UINT8 alpha, extracolormap_t *planecolormap)
{
	FSurfaceInfo Surf;
	FOutVector *v3d;
	INT32 shader = SHADER_DEFAULT;

	size_t nrPlaneVerts = polysector->numVertices;
	INT32 i;

	float height = FIXED_TO_FLOAT(fixedheight); // constant y for all points on the convex flat polygon
	float flatxref, flatyref;
	float fflatsize;
	INT32 flatflag;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	fixed_t tempxs, tempyt;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;

	else if (nrPlaneVerts > (size_t)UINT16_MAX) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %s exceeds max value of %d vertices\n", sizeu1(nrPlaneVerts), UINT16_MAX);
		return;
	}

	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	len = W_LumpLength(lumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			fflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			fflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			fflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			fflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			fflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			fflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			fflatsize = 64.0f;
			flatflag = 63;
			break;
	}

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)(((fixed_t)FIXED_TO_FLOAT(polysector->origVerts[0].x) & (~flatflag)) / fflatsize);
	flatyref = (float)(((fixed_t)FIXED_TO_FLOAT(polysector->origVerts[0].y) & (~flatflag)) / fflatsize);

	// transform
	v3d = planeVerts;

	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatsize;
			angle = FOFsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatsize;
			angle = FOFsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatsize;
			angle = gr_frontsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatsize;
			angle = gr_frontsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}

	if (angle) // Only needs to be done if there's an altered angle
	{
		// This needs to be done so that it scrolls in a different direction after rotation like software
		tempxs = FLOAT_TO_FIXED(scrollx);
		tempyt = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxs, FINESINE(angle)) + FixedMul(tempyt, FINECOSINE(angle))));

		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxs = FLOAT_TO_FIXED(flatxref);
		tempyt = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxs, FINESINE(angle)) + FixedMul(tempyt, FINECOSINE(angle))));
	}

	for (i = 0; i < (INT32)nrPlaneVerts; i++,v3d++)
	{
		// Hurdler: add scrolling texture on floor/ceiling
		v3d->s = (float)((FIXED_TO_FLOAT(polysector->origVerts[i].x) / fflatsize) - flatxref + scrollx); // Go from the polysector's original vertex locations
		v3d->t = (float)(flatyref - (FIXED_TO_FLOAT(polysector->origVerts[i].y) / fflatsize) + scrolly); // Means the flat is offset based on the original vertex locations

		// Need to rotate before translate
		if (angle) // Only needs to be done if there's an altered angle
		{
			tempxs = FLOAT_TO_FIXED(v3d->s);
			tempyt = FLOAT_TO_FIXED(v3d->t);
			v3d->s = (FIXED_TO_FLOAT(FixedMul(tempxs, FINECOSINE(angle)) - FixedMul(tempyt, FINESINE(angle))));
			v3d->t = (FIXED_TO_FLOAT(-FixedMul(tempxs, FINESINE(angle)) - FixedMul(tempyt, FINECOSINE(angle))));
		}

		v3d->x = FIXED_TO_FLOAT(polysector->vertices[i]->x);
		v3d->y = height;
		v3d->z = FIXED_TO_FLOAT(polysector->vertices[i]->y);
	}


	HWR_Lighting(&Surf, lightlevel, planecolormap);


	if (blendmode & PF_Translucent)
	{
		Surf.PolyColor.s.alpha = (UINT8)alpha;
		blendmode |= PF_Modulated|PF_Occlude;
	}
	else
		blendmode |= PF_Masked|PF_Modulated;

	if (HWR_UseShader())
	{
		shader = SHADER_FLOOR;
		blendmode |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, planeVerts, nrPlaneVerts, blendmode, shader, false);
}

static void HWR_AddPolyObjectPlanes(void)
{
	size_t i;
	sector_t *polyobjsector;
	INT32 light = 0;

	// Polyobject Planes need their own function for drawing because they don't have extrasubsectors by themselves
	// It should be okay because polyobjects should always be convex anyway

	for (i  = 0; i < numpolys; i++)
	{
		polyobjsector = po_ptrs[i]->lines[0]->backsector; // the in-level polyobject sector

		if (!(po_ptrs[i]->flags & POF_RENDERPLANES)) // Only render planes when you should
			continue;

		if (po_ptrs[i]->translucency >= NUMTRANSMAPS)
			continue;

		if (polyobjsector->floorheight <= gr_frontsector->ceilingheight
			&& polyobjsector->floorheight >= gr_frontsector->floorheight
			&& (viewz < polyobjsector->floorheight))
		{
			light = R_GetPlaneLight(gr_frontsector, polyobjsector->floorheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->floorpic].lumpnum, po_ptrs[i], false, polyobjsector->floorheight,
												(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), Surf.PolyColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gr_frontsector->extra_colormap : gr_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->floorpic].lumpnum);
				HWR_RenderPolyObjectPlane(po_ptrs[i], false, polyobjsector->floorheight, PF_Occlude,
											(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), levelflats[polyobjsector->floorpic].lumpnum,
											polyobjsector, 255, (light == -1 ? gr_frontsector->extra_colormap : gr_frontsector->lightlist[light].extra_colormap));
			}
		}

		if (polyobjsector->ceilingheight >= gr_frontsector->floorheight
			&& polyobjsector->ceilingheight <= gr_frontsector->ceilingheight
			&& (viewz > polyobjsector->ceilingheight))
		{
			light = R_GetPlaneLight(gr_frontsector, polyobjsector->ceilingheight, true);
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->ceilingpic].lumpnum, po_ptrs[i], true, polyobjsector->ceilingheight,
												(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), Surf.PolyColor.s.alpha, polyobjsector, blendmode, (light == -1 ? gr_frontsector->extra_colormap : gr_frontsector->lightlist[light].extra_colormap));
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->ceilingpic].lumpnum);
				HWR_RenderPolyObjectPlane(po_ptrs[i], true, polyobjsector->ceilingheight, PF_Occlude,
										(light == -1 ? gr_frontsector->lightlevel : *gr_frontsector->lightlist[light].lightlevel), levelflats[polyobjsector->ceilingpic].lumpnum,
										polyobjsector, 255, (light == -1 ? gr_frontsector->extra_colormap : gr_frontsector->lightlist[light].extra_colormap));
			}
		}
	}
}

// -----------------+
// HWR_Subsector    : Determine floor/ceiling planes.
//                  : Add sprites of things in sector.
//                  : Draw one or more line segments.
// Notes            : Sets gr_cursectorlight to the light of the parent sector, to modulate wall textures
// -----------------+
static void HWR_Subsector(size_t num)
{
	INT16 count;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec; //SoM: 4/7/2000
	INT32 floorlightlevel;
	INT32 ceilinglightlevel;
	INT32 locFloorHeight, locCeilingHeight;
	INT32 cullFloorHeight, cullCeilingHeight;
	INT32 light = 0;
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;
    ffloor_t *rover;

#ifdef PARANOIA //no risk while developing, enough debugging nights!
	if (num >= addsubsector)
		I_Error("HWR_Subsector: ss %s with numss = %s, addss = %s\n",
			sizeu1(num), sizeu2(numsubsectors), sizeu3(addsubsector));

	/*if (num >= numsubsectors)
		I_Error("HWR_Subsector: ss %i with numss = %i",
		        num,
		        numsubsectors);*/
#endif

	if (num < numsubsectors)
	{
		// subsector
		sub = &subsectors[num];
		// sector
		gr_frontsector = sub->sector;
		// how many linedefs
		count = sub->numlines;
		// first line seg
		line = &segs[sub->firstline];
	}
	else
	{
		// there are no segs but only planes
		sub = &subsectors[0];
		gr_frontsector = sub->sector;
		count = 0;
		line = NULL;
	}

	//SoM: 4/7/2000: Test to make Boom water work in Hardware mode.
	gr_frontsector = R_FakeFlat(gr_frontsector, &tempsec, &floorlightlevel,
								&ceilinglightlevel, false);
	//FIXME: Use floorlightlevel and ceilinglightlevel insted of lightlevel.

	floorcolormap = ceilingcolormap = gr_frontsector->extra_colormap;

	// ------------------------------------------------------------------------
	// sector lighting, DISABLED because it's done in HWR_StoreWallRange
	// ------------------------------------------------------------------------
	/// \todo store a RGBA instead of just intensity, allow coloured sector lighting
	//light = (FUBYTE)(sub->sector->lightlevel & 0xFF) / 255.0f;
	//gr_cursectorlight.red   = light;
	//gr_cursectorlight.green = light;
	//gr_cursectorlight.blue  = light;
	//gr_cursectorlight.alpha = light;

	cullFloorHeight   = locFloorHeight   = gr_frontsector->floorheight;
	cullCeilingHeight = locCeilingHeight = gr_frontsector->ceilingheight;


	if (gr_frontsector->f_slope)
	{
		cullFloorHeight = P_GetZAt(gr_frontsector->f_slope, viewx, viewy);
		locFloorHeight = P_GetZAt(gr_frontsector->f_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
	}

	if (gr_frontsector->c_slope)
	{
		cullCeilingHeight = P_GetZAt(gr_frontsector->c_slope, viewx, viewy);
		locCeilingHeight = P_GetZAt(gr_frontsector->c_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
	}	
	
	if (gr_frontsector->ffloors)
	{
		boolean anyMoved = gr_frontsector->moved;

		if (anyMoved == false)
		{
			for (rover = gr_frontsector->ffloors; rover; rover = rover->next)
			{
				sector_t *controlSec = &sectors[rover->secnum];
				if (controlSec->moved == true)
				{
					anyMoved = true;
					break;
				}
			}
		}

		if (anyMoved == true)
		{
			gr_frontsector->numlights = sub->sector->numlights = 0;
			R_Prep3DFloors(gr_frontsector);
			sub->sector->lightlist = gr_frontsector->lightlist;
			sub->sector->numlights = gr_frontsector->numlights;
			sub->sector->moved = gr_frontsector->moved = false;
		}

		light = R_GetPlaneLight(gr_frontsector, locFloorHeight, false);
		if (gr_frontsector->floorlightsec == -1)
			floorlightlevel = *gr_frontsector->lightlist[light].lightlevel;
		floorcolormap = gr_frontsector->lightlist[light].extra_colormap;

		light = R_GetPlaneLight(gr_frontsector, locCeilingHeight, false);
		if (gr_frontsector->ceilinglightsec == -1)
			ceilinglightlevel = *gr_frontsector->lightlist[light].lightlevel;
		ceilingcolormap = gr_frontsector->lightlist[light].extra_colormap;
	}

	sub->sector->extra_colormap = gr_frontsector->extra_colormap;

	// render floor ?
#ifdef DOPLANES
	// yeah, easy backface cull! :)
	if (cullFloorHeight < dup_viewz)
	{
		if (gr_frontsector->floorpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
				HWR_GetFlat(levelflats[gr_frontsector->floorpic].lumpnum);
				HWR_RenderPlane(sub, &extrasubsectors[num], false,
					// Hack to make things continue to work around slopes.
					locFloorHeight == cullFloorHeight ? locFloorHeight : gr_frontsector->floorheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, floorlightlevel, levelflats[gr_frontsector->floorpic].lumpnum, NULL, 255, false, floorcolormap);
			}
		}
		else
		{
#ifdef POLYSKY
			HWR_RenderSkyPlane(&extrasubsectors[num], locFloorHeight);
#endif
		}
	}

	if (cullCeilingHeight > dup_viewz)
	{
		if (gr_frontsector->ceilingpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
				HWR_GetFlat(levelflats[gr_frontsector->ceilingpic].lumpnum);
				HWR_RenderPlane(sub, &extrasubsectors[num], true,
					// Hack to make things continue to work around slopes.
					locCeilingHeight == cullCeilingHeight ? locCeilingHeight : gr_frontsector->ceilingheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, ceilinglightlevel, levelflats[gr_frontsector->ceilingpic].lumpnum,NULL, 255, false, ceilingcolormap);
			}
		}
		else
		{
#ifdef POLYSKY
			HWR_RenderSkyPlane(&extrasubsectors[num], locCeilingHeight);
#endif
		}
	}

#ifndef POLYSKY
	// Moved here because before, when above the ceiling and the floor does not have the sky flat, it doesn't draw the sky
	if (gr_frontsector->ceilingpic == skyflatnum || gr_frontsector->floorpic == skyflatnum)
		drawsky = true;
#endif

#ifdef R_FAKEFLOORS
	if (gr_frontsector->ffloors)
	{
		/// \todo fix light, xoffs, yoffs, extracolormap ?
		for (rover = gr_frontsector->ffloors;
			rover; rover = rover->next)
		{
			fixed_t cullHeight, centerHeight;

            // bottom plane
			if (*rover->b_slope)
			{
				cullHeight = P_GetZAt(*rover->b_slope, viewx, viewy);
				centerHeight = P_GetZAt(*rover->b_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
			}
			else
		    cullHeight = centerHeight = *rover->bottomheight;

			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
				continue;
			if (sub->validcount == validcount)
				continue;

			if (centerHeight <= locCeilingHeight &&
			    centerHeight >= locFloorHeight &&
			    ((dup_viewz < cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (dup_viewz > cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap);

					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256) // SoM: Flags are more efficient
				{
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_AddTransparentFloor(levelflats[*rover->bottompic].lumpnum,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Translucent,
					                       false, gr_frontsector->lightlist[light].extra_colormap);

				}
				else
				{
					HWR_GetFlat(levelflats[*rover->bottompic].lumpnum);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
										HWR_RenderPlane(sub, &extrasubsectors[num], false, *rover->bottomheight, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->bottompic].lumpnum,
					                rover->master->frontsector, 255, false, gr_frontsector->lightlist[light].extra_colormap);

				}
			}

			// top plane
			if (*rover->t_slope)
			{
				cullHeight = P_GetZAt(*rover->t_slope, viewx, viewy);
				centerHeight = P_GetZAt(*rover->t_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
			}
			else
		    cullHeight = centerHeight = *rover->topheight;

			if (centerHeight >= locFloorHeight &&
			    centerHeight <= locCeilingHeight &&
			    ((dup_viewz > cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (dup_viewz < cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap);



					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   true,
					                       *rover->topheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
				{
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);

					HWR_AddTransparentFloor(levelflats[*rover->toppic].lumpnum,
					                        &extrasubsectors[num],
											true,
					                        *rover->topheight,
					                        *gr_frontsector->lightlist[light].lightlevel,
					                        rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Translucent,
					                        false, gr_frontsector->lightlist[light].extra_colormap);


				}
				else
				{
					HWR_GetFlat(levelflats[*rover->toppic].lumpnum);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_RenderPlane(sub, &extrasubsectors[num], true, *rover->topheight, (rover->flags & FF_RIPPLE ? PF_Ripple : 0)|PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->toppic].lumpnum,
					                  rover->master->frontsector, 255, false, gr_frontsector->lightlist[light].extra_colormap);
				}
			}
		}
	}
#endif
#endif //doplanes


	// Draw all the polyobjects in this subsector
	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;

		numpolys = 0;

		// Count all the polyobjects, reset the list, and recount them
		while (po)
		{
			++numpolys;
			po = (polyobj_t *)(po->link.next);
		}

		// for render stats
		ps_numpolyobjects.value.i += numpolys;

		// Sort polyobjects
		R_SortPolyObjects(sub);

		// Draw polyobject lines.
		HWR_AddPolyObjectSegs();


		if (sub->validcount != validcount) // This validcount situation seems to let us know that the floors have already been drawn.
		{
			// Draw polyobject planes
			HWR_AddPolyObjectPlanes();
		}
	}

// Hurder ici se passe les choses INT32�essantes!
// on vient de tracer le sol et le plafond
// on trace �pr�ent d'abord les sprites et ensuite les murs
// hurdler: faux: on ajoute seulement les sprites, le murs sont trac� d'abord
	if (line)
	{
		// draw sprites first, coz they are clipped to the solidsegs of
		// subsectors more 'in front'
		HWR_AddSprites(gr_frontsector);

		//Hurdler: at this point validcount must be the same, but is not because
		//         gr_frontsector doesn't point anymore to sub->sector due to
		//         the call gr_frontsector = R_FakeFlat(...)
		//         if it's not done, the sprite is drawn more than once,
		//         what looks really bad with translucency or dynamic light,
		//         without talking about the overdraw of course.
		sub->sector->validcount = validcount;/// \todo fix that in a better way

		while (count--)
		{
				if (!line->polyseg) // ignore segs that belong to polyobjects
				HWR_AddLine(line);
				line++;
		}
	}

	sub->validcount = validcount;
}

//
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.

#ifdef coolhack
//t;b;l;r
static fixed_t hackbbox[4];
//BOXTOP,
//BOXBOTTOM,
//BOXLEFT,
//BOXRIGHT
static boolean HWR_CheckHackBBox(fixed_t *bb)
{
	if (bb[BOXTOP] < hackbbox[BOXBOTTOM]) //y up
		return false;
	if (bb[BOXBOTTOM] > hackbbox[BOXTOP])
		return false;
	if (bb[BOXLEFT] > hackbbox[BOXRIGHT])
		return false;
	if (bb[BOXRIGHT] < hackbbox[BOXLEFT])
		return false;
	return true;
}
#endif

static void HWR_RenderBSPNode(INT32 bspnum)
{
	node_t *bsp;
	INT32 side;

	ps_numbspcalls.value.i++;

	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		side = R_PointOnSide(viewx, viewy, bsp);
		// Recursively divide front space.
		HWR_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space.
		if (!HWR_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}

	HWR_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

/*
//
// Clear 'stack' of subsectors to draw
//
static void HWR_ClearDrawSubsectors(void)
{
	gr_drawsubsector_p = gr_drawsubsectors;
}

//
// Draw subsectors pushed on the drawsubsectors 'stack', back to front
//
static void HWR_RenderSubsectors(void)
{
	while (gr_drawsubsector_p > gr_drawsubsectors)
	{
		HWR_RenderBSPNode(
		lastsubsec->nextsubsec = bspnum & (~NF_SUBSECTOR);
	}
}
*/

// ==========================================================================
//                                                              FROM R_MAIN.C
// ==========================================================================

//BP : exactely the same as R_InitTextureMapping
void HWR_InitTextureMapping(void)
{
	angle_t i;
	INT32 x;
	INT32 t;
	fixed_t focallength;
	fixed_t grcenterx;
	fixed_t grcenterxfrac;
	INT32 grviewwidth;

#define clipanglefov (FIELDOFVIEW>>ANGLETOFINESHIFT)

	grviewwidth = vid.width;
	grcenterx = grviewwidth/2;
	grcenterxfrac = grcenterx<<FRACBITS;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(grcenterxfrac,
		FINETANGENT(FINEANGLES/4+clipanglefov/2));

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > FRACUNIT*2)
			t = -1;
		else if (FINETANGENT(i) < -FRACUNIT*2)
			t = grviewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (grcenterxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > grviewwidth+1)
				t = grviewwidth+1;
		}
		gr_viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	for (x = 0; x <= grviewwidth; x++)
	{
		i = 0;
		while (gr_viewangletox[i]>x)
			i++;
		gr_xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (gr_viewangletox[i] == -1)
			gr_viewangletox[i] = 0;
		else if (gr_viewangletox[i] == grviewwidth+1)
			gr_viewangletox[i]  = grviewwidth;
	}

	gr_clipangle = gr_xtoviewangle[0];
}

// ==========================================================================
// gr_things.c
// ==========================================================================

// sprites are drawn after all wall and planes are rendered, so that
// sprite translucency effects apply on the rendered view (instead of the background sky!!)

static UINT32 gr_visspritecount;
static gr_vissprite_t *gr_visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};

// --------------------------------------------------------------------------
// HWR_ClearSprites
// Called at frame start.
// --------------------------------------------------------------------------
static void HWR_ClearSprites(void)
{
	gr_visspritecount = 0;
}

// --------------------------------------------------------------------------
// HWR_NewVisSprite
// --------------------------------------------------------------------------
static gr_vissprite_t gr_overflowsprite;

static gr_vissprite_t *HWR_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;

		// Allocate chunk if necessary
		if (!gr_visspritechunks[chunk])
			Z_Malloc(sizeof(gr_vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &gr_visspritechunks[chunk]);

		return gr_visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static gr_vissprite_t *HWR_NewVisSprite(void)
{
	if (gr_visspritecount == MAXVISSPRITES)
		return &gr_overflowsprite;

	return HWR_GetVisSprite(gr_visspritecount++);
}

// Finds a floor through which light does not pass.
static fixed_t HWR_OpaqueFloorAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height)
{
	const sector_t *sec = R_PointInSubsector(x, y)->sector;
	fixed_t floorz = sec->floorheight;

	if (sec->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		const fixed_t thingtop = z + height;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS)
			|| !(rover->flags & FF_RENDERPLANES)
			|| rover->flags & FF_TRANSLUCENT
			|| rover->flags & FF_FOG
			|| rover->flags & FF_INVERTPLANES)
				continue;

			delta1 = z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > floorz && abs(delta1) < abs(delta2))
				floorz = *rover->topheight;
		}
	}

	return floorz;
}

//
// HWR_DoCulling
// Hardware version of R_DoCulling
// (see r_main.c)
static boolean HWR_DoCulling(line_t *cullheight, line_t *viewcullheight, float vz, float bottomh, float toph)
{
	float cullplane;

	if (!cullheight)
		return false;

	cullplane = FIXED_TO_FLOAT(cullheight->frontsector->floorheight);
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

static void HWR_DrawSpriteShadow(gr_vissprite_t *spr, GLPatch_t *gpatch, float this_scale)
{
	FOutVector swallVerts[4];
	FSurfaceInfo sSurf;
	fixed_t floorheight, mobjfloor;
	float offset = 0;
	pslope_t *floorslope;
	fixed_t slopez;
	FBITFIELD blendmode = PF_Translucent|PF_Modulated;
	INT32 shader = SHADER_DEFAULT;

	R_GetShadowZ(spr->mobj, &floorslope);



    interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation())
	{
	  R_InterpolateMobjState(spr->mobj, rendertimefrac, &interp);
	}
	else
	{
	  R_InterpolateMobjState(spr->mobj, FRACUNIT, &interp);
	}

	mobjfloor = HWR_OpaqueFloorAtPos(
		interp.x, interp.y,
		interp.z, spr->mobj->height);
	if (cv_shadowoffs.value)
	{
		angle_t shadowdir;

		// Set direction
		if (splitscreen && stplyr == &players[secondarydisplayplayer])
			shadowdir = localangle2 + FixedAngle(cv_cam2_rotate.value);
		else
			shadowdir = localangle + FixedAngle(cv_cam_rotate.value);

		// Find floorheight
		floorheight = HWR_OpaqueFloorAtPos(
			interp.x + P_ReturnThrustX(spr->mobj, shadowdir, interp.z - mobjfloor),
			interp.y + P_ReturnThrustY(spr->mobj, shadowdir, interp.z - mobjfloor),
			interp.z, spr->mobj->height);

		// The shadow is falling ABOVE it's mobj?
		// Don't draw it, then!
		if (spr->mobj->z < floorheight)
			return;
		else
		{
			fixed_t floorz;
			floorz = HWR_OpaqueFloorAtPos(
				interp.x + P_ReturnThrustX(spr->mobj, shadowdir, interp.z - floorheight),
				interp.y + P_ReturnThrustY(spr->mobj, shadowdir, interp.z - floorheight),
				interp.z, spr->mobj->height);
			// The shadow would be falling on a wall? Don't draw it, then.
			// Would draw midair otherwise.
			if (floorz < floorheight)
				return;
		}

		floorheight = FixedInt(interp.z - floorheight);

		offset = floorheight;
	}
	else
		floorheight = FixedInt(interp.z - mobjfloor);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	// x1/x2 were already scaled in HWR_ProjectSprite
	// First match the normal sprite
	swallVerts[0].x = swallVerts[3].x = spr->x1;
	swallVerts[2].x = swallVerts[1].x = spr->x2;
	swallVerts[0].z = swallVerts[3].z = spr->z1;
	swallVerts[2].z = swallVerts[1].z = spr->z2;

	if (spr->mobj && fabsf(this_scale - 1.0f) > 1.0E-36f)
	{
		// Always a pixel above the floor, perfectly flat.
		swallVerts[0].y = swallVerts[1].y = swallVerts[2].y = swallVerts[3].y = spr->ty - gpatch->topoffset * this_scale - (floorheight+3);

		// Now transform the TOP vertices along the floor in the direction of the camera
		swallVerts[3].x = spr->x1 + ((gpatch->height * this_scale) + offset) * gr_viewcos;
		swallVerts[2].x = spr->x2 + ((gpatch->height * this_scale) + offset) * gr_viewcos;
		swallVerts[3].z = spr->z1 + ((gpatch->height * this_scale) + offset) * gr_viewsin;
		swallVerts[2].z = spr->z2 + ((gpatch->height * this_scale) + offset) * gr_viewsin;
	}
	else
	{
		// Always a pixel above the floor, perfectly flat.
		swallVerts[0].y = swallVerts[1].y = swallVerts[2].y = swallVerts[3].y = spr->ty - gpatch->topoffset - (floorheight+3);

		// Now transform the TOP vertices along the floor in the direction of the camera
		swallVerts[3].x = spr->x1 + (gpatch->height + offset) * gr_viewcos;
		swallVerts[2].x = spr->x2 + (gpatch->height + offset) * gr_viewcos;
		swallVerts[3].z = spr->z1 + (gpatch->height + offset) * gr_viewsin;
		swallVerts[2].z = spr->z2 + (gpatch->height + offset) * gr_viewsin;
	}

	// We also need to move the bottom ones away when shadowoffs is on
	if (cv_shadowoffs.value)
	{
		swallVerts[0].x = spr->x1 + offset * gr_viewcos;
		swallVerts[1].x = spr->x2 + offset * gr_viewcos;
		swallVerts[0].z = spr->z1 + offset * gr_viewsin;
		swallVerts[1].z = spr->z2 + offset * gr_viewsin;
	}

	if (floorslope)
	for (int i = 0; i < 4; i++)
	{
		slopez = P_GetZAt(floorslope, FLOAT_TO_FIXED(swallVerts[i].x), FLOAT_TO_FIXED(swallVerts[i].z));
		swallVerts[i].y = FIXED_TO_FLOAT(slopez) + 0.05f;
	}

	if (spr->flip)
	{
		swallVerts[0].s = swallVerts[3].s = gpatch->max_s;
		swallVerts[2].s = swallVerts[1].s = 0;
	}
	else
	{
		swallVerts[0].s = swallVerts[3].s = 0;
		swallVerts[2].s = swallVerts[1].s = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		swallVerts[3].t = swallVerts[2].t = gpatch->max_t;
		swallVerts[0].t = swallVerts[1].t = 0;
	}
	else
	{
		swallVerts[3].t = swallVerts[2].t = 0;
		swallVerts[0].t = swallVerts[1].t = gpatch->max_t;
	}

	sSurf.PolyColor.s.red = 0x01;
	sSurf.PolyColor.s.blue = 0x01;
	sSurf.PolyColor.s.green = 0x01;


	/*if (spr->mobj->frame & FF_TRANSMASK || spr->mobj->flags2 & MF2_SHADOW)
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light = R_GetPlaneLight(sector, spr->mobj->floorz, false);

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel;

			if (sector->lightlist[light].extra_colormap)
				colormap = sector->lightlist[light].extra_colormap;
		}
		else
		{
			lightlevel = sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (colormap)
			sSurf.PolyColor.rgba = HWR_Lighting(lightlevel/2, colormap->rgba, colormap->fadergba, false, true);
		else
			sSurf.PolyColor.rgba = HWR_Lighting(lightlevel/2, NORMALFOG, FADEFOG, false, true);
	}*/

	// shadow is always half as translucent as the sprite itself
	if (!cv_translucency.value) // use default translucency (main sprite won't have any translucency)
		sSurf.PolyColor.s.alpha = 0x80; // default
	else if (spr->mobj->flags2 & MF2_SHADOW)
		sSurf.PolyColor.s.alpha = 0x20;
	else if (spr->mobj->frame & FF_TRANSMASK)
	{
		HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &sSurf);
		sSurf.PolyColor.s.alpha /= 2; //cut alpha in half!
	}
	else
		sSurf.PolyColor.s.alpha = 0x80; // default


	if (sSurf.PolyColor.s.alpha > floorheight/4)
	{
		sSurf.PolyColor.s.alpha = (UINT8)(sSurf.PolyColor.s.alpha - floorheight/4);
		if (HWR_UseShader())
		{
			shader = SHADER_NONE;
			blendmode |= PF_ColorMapped;
		}
		HWR_ProcessPolygon(&sSurf, swallVerts, 4, blendmode, shader, false);
	}
}

// This is expecting a pointer to an array containing 4 wallVerts for a sprite
static void HWR_RotateSpritePolyToAim(gr_vissprite_t *spr, FOutVector *wallVerts)
{
	if (cv_grspritebillboarding.value && spr && spr->mobj && wallVerts)
	{

		// uncapped/interpolation
		interpmobjstate_t interp = {0};


		// do interpolation
	    if (R_UsingFrameInterpolation())
	    {
	      R_InterpolateMobjState(spr->mobj, rendertimefrac, &interp);
	    }
	    else
	    {
	      R_InterpolateMobjState(spr->mobj, FRACUNIT, &interp);
	    }

        float basey = FIXED_TO_FLOAT(interp.z);
		float lowy = wallVerts[0].y;
		if (P_MobjFlip(spr->mobj) == -1)
		{
			basey = FIXED_TO_FLOAT(interp.z + spr->mobj->height);
		}
		// Rotate sprites to fully billboard with the camera
		// X, Y, AND Z need to be manipulated for the polys to rotate around the
		// origin, because of how the origin setting works I believe that should
		// be mobj->z or mobj->z + mobj->height
		wallVerts[2].y = wallVerts[3].y = (spr->ty - basey) * gr_viewludsin + basey;
		wallVerts[0].y = wallVerts[1].y = (lowy - basey) * gr_viewludsin + basey;
		// translate back to be around 0 before translating back
		wallVerts[3].x += ((spr->ty - basey) * gr_viewludcos) * gr_viewcos;
		wallVerts[2].x += ((spr->ty - basey) * gr_viewludcos) * gr_viewcos;

		wallVerts[0].x += ((lowy - basey) * gr_viewludcos) * gr_viewcos;
		wallVerts[1].x += ((lowy - basey) * gr_viewludcos) * gr_viewcos;

		wallVerts[3].z += ((spr->ty - basey) * gr_viewludcos) * gr_viewsin;
		wallVerts[2].z += ((spr->ty - basey) * gr_viewludcos) * gr_viewsin;

		wallVerts[0].z += ((lowy - basey) * gr_viewludcos) * gr_viewsin;
		wallVerts[1].z += ((lowy - basey) * gr_viewludcos) * gr_viewsin;
	}
}

static void HWR_SplitSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	FOutVector baseWallVerts[4]; // This is what the verts should end up as
	GLPatch_t *gpatch;
	FSurfaceInfo Surf;
	const boolean hires = (spr->mobj && spr->mobj->skin && ((skin_t *)spr->mobj->skin)->flags & SF_HIRES);
	extracolormap_t *colormap;
	FUINT lightlevel;
	FBITFIELD blend = 0;
	INT32 shader = SHADER_DEFAULT;
	UINT8 alpha;

	INT32 i;
	float realtop, realbot, top, bot;
	float ttop, tbot, tmult;
	float bheight;
	float realheight, heightmult;
	const sector_t *sector = spr->mobj->subsector->sector;
	const lightlist_t *list = sector->lightlist;
	float endrealtop, endrealbot, endtop, endbot;
	float endbheight;
	float endrealheight;
	fixed_t temp;
	fixed_t v1x, v1y, v2x, v2y;

	this_scale = FIXED_TO_FLOAT(spr->mobj->scale);

	if (hires)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)spr->mobj->skin)->highresscale);

	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// Draw shadow BEFORE sprite
	if (cv_shadow.value // Shadows enabled
		&& (spr->mobj->flags & (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY)) != (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY) // Ceiling scenery have no shadow.
		&& !(spr->mobj->flags2 & MF2_DEBRIS) // Debris have no corona or shadow.
		&& (spr->mobj->z >= spr->mobj->floorz)) // Without this, your shadow shows on the floor, even after you die and fall through the ground.
	{
		////////////////////
		// SHADOW SPRITE! //
		////////////////////
		HWR_DrawSpriteShadow(spr, gpatch, this_scale);
	}

	baseWallVerts[0].x = baseWallVerts[3].x = spr->x1;
	baseWallVerts[2].x = baseWallVerts[1].x = spr->x2;
	baseWallVerts[0].z = baseWallVerts[3].z = spr->z1;
	baseWallVerts[1].z = baseWallVerts[2].z = spr->z2;

	baseWallVerts[2].y = baseWallVerts[3].y = spr->ty;
	if (spr->mobj && fabsf(this_scale - 1.0f) > 1.0E-36f)
		baseWallVerts[0].y = baseWallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		baseWallVerts[0].y = baseWallVerts[1].y = spr->ty - gpatch->height;

	v1x = FLOAT_TO_FIXED(spr->x1);
	v1y = FLOAT_TO_FIXED(spr->z1);
	v2x = FLOAT_TO_FIXED(spr->x2);
	v2y = FLOAT_TO_FIXED(spr->z2);

	if (spr->flip)
	{
		baseWallVerts[0].s = baseWallVerts[3].s = gpatch->max_s;
		baseWallVerts[2].s = baseWallVerts[1].s = 0;
	}
	else
	{
		baseWallVerts[0].s = baseWallVerts[3].s = 0;
		baseWallVerts[2].s = baseWallVerts[1].s = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		baseWallVerts[3].t = baseWallVerts[2].t = gpatch->max_t;
		baseWallVerts[0].t = baseWallVerts[1].t = 0;
	}
	else
	{
		baseWallVerts[3].t = baseWallVerts[2].t = 0;
		baseWallVerts[0].t = baseWallVerts[1].t = gpatch->max_t;
	}



	// Let dispoffset work first since this adjust each vertex
	HWR_RotateSpritePolyToAim(spr, baseWallVerts);

	// push it toward the camera to mitigate floor-clipping sprites
	if (spr->mobj->type != MT_SIGN) // but not for goalposts
	{
		float sprdist = sqrtf((spr->x1 - gr_viewx)*(spr->x1 - gr_viewx) + (spr->z1 - gr_viewy)*(spr->z1 - gr_viewy) + (spr->ty - gr_viewz)*(spr->ty - gr_viewz));
		float distfact = ((2.0f*spr->dispoffset) + 20.0f) / sprdist;
		for (i = 0; i < 4; i++)
		{
				baseWallVerts[i].x += (gr_viewx - baseWallVerts[i].x)*distfact;
				baseWallVerts[i].z += (gr_viewy - baseWallVerts[i].z)*distfact;
				baseWallVerts[i].y += (gr_viewz - baseWallVerts[i].y)*distfact;
		}
	}

	realtop = top = baseWallVerts[3].y;
	realbot = bot = baseWallVerts[0].y;
	ttop = baseWallVerts[3].t;
	tbot = baseWallVerts[0].t;
	tmult = (tbot - ttop) / (top - bot);
	endrealtop = endtop = baseWallVerts[2].y;
	endrealbot = endbot = baseWallVerts[1].y;

	// copy the contents of baseWallVerts into the drawn wallVerts array
	// baseWallVerts is used to know the final shape to easily get the vertex
	// co-ordinates
	memcpy(wallVerts, baseWallVerts, sizeof(baseWallVerts));

	if (!cv_translucency.value) // translucency disabled
	{
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}
	else if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.PolyColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}
	alpha = Surf.PolyColor.s.alpha;
	// Start with the lightlevel and colormap from the top of the sprite
	lightlevel = *list[sector->numlights - 1].lightlevel;
	colormap = list[sector->numlights - 1].extra_colormap;
	i = 0;
	temp = FLOAT_TO_FIXED(realtop);

	if (spr->mobj->frame & FF_FULLBRIGHT)
		lightlevel = 255;


	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t h = sector->lightlist[i].slope ? P_GetZAt(sector->lightlist[i].slope, spr->mobj->x, spr->mobj->y)
					: sector->lightlist[i].height;
		if (h <= temp)
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *list[i-1].lightlevel;
			colormap = list[i-1].extra_colormap;
			break;
		}
	}

	for (i = 0; i < sector->numlights; i++)
	{
		if (endtop < endrealbot)
		if (top < realbot)
			return;

		// even if we aren't changing colormap or lightlevel, we still need to continue drawing down the sprite
		if (!(list[i].flags & FF_NOSHADE) && (list[i].flags & FF_CUTSPRITES))
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *list[i].lightlevel;
			colormap = list[i].extra_colormap;
		}


		if (i + 1 < sector->numlights)
		{
			if (list[i+1].slope)
			{
				temp = P_GetZAt(list[i+1].slope, v1x, v1y);
				bheight = FIXED_TO_FLOAT(temp);
				temp = P_GetZAt(list[i+1].slope, v2x, v2y);
				endbheight = FIXED_TO_FLOAT(temp);
			}
			else
				bheight = endbheight = FIXED_TO_FLOAT(list[i+1].height);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}


		if (endbheight >= endtop)
		if (bheight >= top)
			continue;

		bot = bheight;

		if (bot < realbot)
			bot = realbot;


		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;



		wallVerts[3].t = ttop + ((realtop - top) * tmult);
		wallVerts[2].t = ttop + ((endrealtop - endtop) * tmult);
		wallVerts[0].t = ttop + ((realtop - bot) * tmult);
		wallVerts[1].t = ttop + ((endrealtop - endbot) * tmult);
		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;

		// The x and y only need to be adjusted in the case that it's not a papersprite
		if (cv_grspritebillboarding.value && spr->mobj)
		{
			// Get the x and z of the vertices so billboarding draws correctly
			realheight = realbot - realtop;
			endrealheight = endrealbot - endrealtop;
			heightmult = (realtop - top) / realheight;
			wallVerts[3].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[3].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endtop) / endrealheight;
			wallVerts[2].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[2].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;

			heightmult = (realtop - bot) / realheight;
			wallVerts[0].x = baseWallVerts[3].x + (baseWallVerts[3].x - baseWallVerts[0].x) * heightmult;
			wallVerts[0].z = baseWallVerts[3].z + (baseWallVerts[3].z - baseWallVerts[0].z) * heightmult;

			heightmult = (endrealtop - endbot) / endrealheight;
			wallVerts[1].x = baseWallVerts[2].x + (baseWallVerts[2].x - baseWallVerts[1].x) * heightmult;
			wallVerts[1].z = baseWallVerts[2].z + (baseWallVerts[2].z - baseWallVerts[1].z) * heightmult;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);

		if (HWR_UseShader())
		{
			shader = SHADER_SPRITE;
			blend |= PF_ColorMapped;
		}

		Surf.PolyColor.s.alpha = alpha;

		HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false); // sprite shader

		top = bot;
		endtop = endbot;
	}

	bot = realbot;
	endbot = endrealbot;
	if (endtop <= endrealbot)
	if (top <= realbot)
		return;

	// If we're ever down here, somehow the above loop hasn't draw all the light levels of sprite
	wallVerts[3].t = ttop + ((realtop - top) * tmult);
	wallVerts[2].t = ttop + ((endrealtop - endtop) * tmult);
	wallVerts[0].t = ttop + ((realtop - bot) * tmult);
	wallVerts[1].t = ttop + ((endrealtop - endbot) * tmult);
	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;


	HWR_Lighting(&Surf, lightlevel, colormap);

	HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false); // sprite shader
}

// -----------------+
// HWR_DrawSprite   : Draw flat sprites
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
static void HWR_DrawSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;
	INT32 shader = SHADER_DEFAULT;

	if (spr->mobj)
		this_scale = spr->scale;

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	if (spr->mobj->subsector->sector->numlights)
	{
		HWR_SplitSprite(spr);
		return;
	}

	// cache sprite graphics
	//12/12/99: Hurdler:
	//          OK, I don't change anything for MD2 support because I want to be
	//          sure to do it the right way. So actually, we keep normal sprite
	//          in memory and we add the md2 model if it exists for that sprite

	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	// these were already scaled in HWR_ProjectSprite
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	if (spr->mobj && fabsf(this_scale - 1.0f) > 1.0E-36f)
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	if (spr->flip)
	{
		wallVerts[0].s = wallVerts[3].s = gpatch->max_s;
		wallVerts[2].s = wallVerts[1].s = 0;
	}
	else
	{
		wallVerts[0].s = wallVerts[3].s = 0;
		wallVerts[2].s = wallVerts[1].s = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		wallVerts[3].t = wallVerts[2].t = gpatch->max_t;
		wallVerts[0].t = wallVerts[1].t = 0;
	}else{
		wallVerts[3].t = wallVerts[2].t = 0;
		wallVerts[0].t = wallVerts[1].t = gpatch->max_t;
	}

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// Draw shadow BEFORE sprite
	if (cv_shadow.value // Shadows enabled
		&& (spr->mobj->flags & (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY)) != (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY) // Ceiling scenery have no shadow.
		&& !(spr->mobj->flags2 & MF2_DEBRIS) // Debris have no corona or shadow.
		&& (spr->mobj->z >= spr->mobj->floorz)) // Without this, your shadow shows on the floor, even after you die and fall through the ground.
	{
		////////////////////
		// SHADOW SPRITE! //
		////////////////////
		HWR_DrawSpriteShadow(spr, gpatch, this_scale);
	}

	// Let dispoffset work first since this adjust each vertex
	HWR_RotateSpritePolyToAim(spr, wallVerts);

	// push it toward the camera to mitigate floor-clipping sprites
	if (spr->mobj->type != MT_SIGN)
	{
		float sprdist = sqrtf((spr->x1 - gr_viewx)*(spr->x1 - gr_viewx) + (spr->z1 - gr_viewy)*(spr->z1 - gr_viewy) + (spr->ty - gr_viewz)*(spr->ty - gr_viewz));
		float distfact = ((2.0f*spr->dispoffset) + 20.0f) / sprdist;
		size_t i;
		for (i = 0; i < 4; i++)
		{
			wallVerts[i].x += (gr_viewx - wallVerts[i].x)*distfact;
			wallVerts[i].z += (gr_viewy - wallVerts[i].z)*distfact;
			wallVerts[i].y += (gr_viewz - wallVerts[i].y)*distfact;
		}
	}


	// This needs to be AFTER the shadows so that the regular sprites aren't drawn completely black.
	// sprite lighting by modulating the RGB components
	/// \todo coloured

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (!(spr->mobj->frame & FF_FULLBRIGHT))
			lightlevel = sector->lightlevel;

		HWR_Lighting(&Surf, lightlevel, colormap);

	}

	{
		FBITFIELD blend = 0;
		if (!cv_translucency.value) // translucency disabled
		{
			Surf.PolyColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}
		else if (spr->mobj->flags2 & MF2_SHADOW)
		{
			Surf.PolyColor.s.alpha = 0x40;
			blend = PF_Translucent;
		}
		else if (spr->mobj->frame & FF_TRANSMASK)
			blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
		{
			// BP: i agree that is little better in environement but it don't
			//     work properly under glide nor with fogcolor to ffffff :(
			// Hurdler: PF_Environement would be cool, but we need to fix
			//          the issue with the fog before
			Surf.PolyColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}

	if (HWR_UseShader())
	{
		shader = SHADER_SPRITE;
		blend |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
	}
}

#ifdef HWPRECIP
// Sprite drawer for precipitation
static inline void HWR_DrawPrecipitationSprite(gr_vissprite_t *spr)
{
	INT32 shader = SHADER_DEFAULT;
	FBITFIELD blend = 0;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;

	if (P_MobjWasRemoved(spr->mobj))
		return;

	if (!spr->mobj->subsector)
		return;

	// cache sprite graphics
	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	// Let dispoffset work first since this adjust each vertex
	HWR_RotateSpritePolyToAim(spr, wallVerts);

	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = gpatch->max_s;

	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = gpatch->max_t;

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel;

			if (sector->lightlist[light].extra_colormap)
				colormap = sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

	HWR_Lighting(&Surf, lightlevel, colormap);
	}

	if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.PolyColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.PolyColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}

	if (HWR_UseShader())
	{
		shader = SHADER_SPRITE;
		blend |= PF_ColorMapped;
	}

	HWR_ProcessPolygon(&Surf, wallVerts, 4, blend|PF_Modulated, shader, false);
}
#endif

// --------------------------------------------------------------------------
// Sort vissprites by distance
// --------------------------------------------------------------------------

gr_vissprite_t* gr_vsprorder[MAXVISSPRITES];

// For more correct transparency the transparent sprites would need to be
// sorted and drawn together with transparent surfaces.
static int CompareVisSprites(const void *p1, const void *p2)
{
	gr_vissprite_t* spr1 = *(gr_vissprite_t*const*)p1;
	gr_vissprite_t* spr2 = *(gr_vissprite_t*const*)p2;
	int idiff;
	float fdiff;

	// make transparent sprites last
	// "boolean to int"

	int transparency1 = (spr1->mobj->flags2 & MF2_SHADOW) || (spr1->mobj->frame & FF_TRANSMASK);
	int transparency2 = (spr2->mobj->flags2 & MF2_SHADOW) || (spr2->mobj->frame & FF_TRANSMASK);
	idiff = transparency1 - transparency2;
	if (idiff != 0) return idiff;

	fdiff = spr2->tz - spr1->tz;// this order seems correct when checking with apitrace. Back to front.
	if (fabsf(fdiff) < 1.0E-36f)
		return spr1->dispoffset - spr2->dispoffset;// smallest dispoffset first if sprites are at (almost) same location.
	else if (fdiff > 0)
		return 1;
	else
		return -1;
}


static void HWR_SortVisSprites(void)
{
	UINT32 i;
	for (i = 0; i < gr_visspritecount; i++)
	{
		gr_vsprorder[i] = HWR_GetVisSprite(i);
	}
	qsort(gr_vsprorder, gr_visspritecount, sizeof(gr_vissprite_t*), CompareVisSprites);
}

// A drawnode is something that points to a 3D floor, 3D side, or masked
// middle texture. This is used for sorting with sprites.
typedef struct
{
	FOutVector    wallVerts[4];
	FSurfaceInfo  Surf;
	INT32         texnum;
	FBITFIELD     blend;
	INT32         drawcount;
	boolean fogwall;
	INT32 lightlevel;
	extracolormap_t *wallcolormap; // Doing the lighting in HWR_RenderWall now for correct fog after sorting
} wallinfo_t;

static wallinfo_t *wallinfo = NULL;
static size_t numwalls = 0; // a list of transparent walls to be drawn

void HWR_RenderWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);

#define MAX_TRANSPARENTWALL 256

typedef struct
{
	extrasubsector_t *xsub;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	lumpnum_t lumpnum;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	boolean fogplane;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} planeinfo_t;

static size_t numplanes = 0; // a list of transparent floors to be drawn
static planeinfo_t *planeinfo = NULL;

typedef struct
{
	polyobj_t *polysector;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	lumpnum_t lumpnum;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} polyplaneinfo_t;

static size_t numpolyplanes = 0; // a list of transparent poyobject floors to be drawn
static polyplaneinfo_t *polyplaneinfo = NULL;


//static floorinfo_t *floorinfo = NULL;
//static size_t numfloors = 0;
//Hurdler: 3D water sutffs
typedef struct gr_drawnode_s
{
	planeinfo_t *plane;
	polyplaneinfo_t *polyplane;
	wallinfo_t *wall;
	gr_vissprite_t *sprite;

//	struct gr_drawnode_s *next;
//	struct gr_drawnode_s *prev;
} gr_drawnode_t;

static INT32 drawcount = 0;

#define MAX_TRANSPARENTFLOOR 512

// This will likely turn into a copy of HWR_Add3DWater and replace it.

void HWR_AddTransparentFloor(lumpnum_t lumpnum, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap)
{
	static size_t allocedplanes = 0;

	// Force realloc if buffer has been freed
	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numplanes + 1)
	{
		allocedplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}

	planeinfo[numplanes].isceiling = isceiling;
	planeinfo[numplanes].fixedheight = fixedheight;
	if (planecolormap && (planecolormap->fog & 1))
		planeinfo[numplanes].lightlevel = lightlevel;
	else
		planeinfo[numplanes].lightlevel = 255;
	planeinfo[numplanes].lumpnum = lumpnum;
	planeinfo[numplanes].xsub = xsub;
	planeinfo[numplanes].alpha = alpha;
	planeinfo[numplanes].FOFSector = FOFSector;
	planeinfo[numplanes].blend = blend;
	planeinfo[numplanes].fogplane = fogplane;
	planeinfo[numplanes].planecolormap = planecolormap;
	planeinfo[numplanes].drawcount = drawcount++;

	numplanes++;
}

// Adding this for now until I can create extrasubsector info for polyobjects
// When that happens it'll just be done through HWR_AddTransparentFloor and HWR_RenderPlane
void HWR_AddTransparentPolyobjectFloor(lumpnum_t lumpnum, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap)
{
	static size_t allocedpolyplanes = 0;

	// Force realloc if buffer has been freed
	if (!polyplaneinfo)
		allocedpolyplanes = 0;

	if (allocedpolyplanes < numpolyplanes + 1)
	{
		allocedpolyplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(polyplaneinfo, allocedpolyplanes * sizeof (*polyplaneinfo), PU_LEVEL, &polyplaneinfo);
	}

	polyplaneinfo[numpolyplanes].isceiling = isceiling;
	polyplaneinfo[numpolyplanes].fixedheight = fixedheight;
	if (planecolormap && (planecolormap->fog & 1))
		polyplaneinfo[numpolyplanes].lightlevel = lightlevel;
	else
		polyplaneinfo[numpolyplanes].lightlevel = 255;
	polyplaneinfo[numpolyplanes].lumpnum = lumpnum;
	polyplaneinfo[numpolyplanes].polysector = polysector;
	polyplaneinfo[numpolyplanes].alpha = alpha;
	polyplaneinfo[numpolyplanes].FOFSector = FOFSector;
	polyplaneinfo[numpolyplanes].blend = blend;
	polyplaneinfo[numpolyplanes].planecolormap = planecolormap;
	polyplaneinfo[numpolyplanes].drawcount = drawcount++;
	numpolyplanes++;
}

// putting sortindex and sortnode here so the comparator function can see them
gr_drawnode_t *sortnode;
size_t *sortindex;

static int CompareDrawNodes(const void *p1, const void *p2)
{
	size_t n1 = *(const size_t*)p1;
	size_t n2 = *(const size_t*)p2;
	INT32 v1 = 0;
	INT32 v2 = 0;
	INT32 diff;
	if (sortnode[n1].plane)
		v1 = sortnode[n1].plane->drawcount;
	else if (sortnode[n1].polyplane)
		v1 = sortnode[n1].polyplane->drawcount;
	else if (sortnode[n1].wall)
		v1 = sortnode[n1].wall->drawcount;
	else I_Error("CompareDrawNodes: n1 unknown");

	if (sortnode[n2].plane)
		v2 = sortnode[n2].plane->drawcount;
	else if (sortnode[n2].polyplane)
		v2 = sortnode[n2].polyplane->drawcount;
	else if (sortnode[n2].wall)
		v2 = sortnode[n2].wall->drawcount;
	else I_Error("CompareDrawNodes: n2 unknown");

	diff = v2 - v1;
	if (diff == 0) I_Error("CompareDrawNodes: diff is zero");
	return diff;
}

static int CompareDrawNodePlanes(const void *p1, const void *p2)
{
	size_t n1 = *(const size_t*)p1;
	size_t n2 = *(const size_t*)p2;
	if (!sortnode[n1].plane) I_Error("CompareDrawNodePlanes: Uh.. This isn't a plane! (n1)");
	if (!sortnode[n2].plane) I_Error("CompareDrawNodePlanes: Uh.. This isn't a plane! (n2)");
	return ABS(sortnode[n2].plane->fixedheight - viewz) - ABS(sortnode[n1].plane->fixedheight - viewz);
}

//
// HWR_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static void HWR_CreateDrawNodes(void)
{
	UINT32 i = 0, p = 0;
	size_t run_start = 0;

	// Dump EVERYTHING into a huge drawnode list. Then we'll sort it!
	// Could this be optimized into _AddTransparentWall/_AddTransparentPlane?
	// Hell yes! But sort algorithm must be modified to use a linked list.
	sortnode = Z_Calloc((sizeof(planeinfo_t)*numplanes)
					+ (sizeof(polyplaneinfo_t)*numpolyplanes)
					+ (sizeof(wallinfo_t)*numwalls)
					,PU_STATIC, NULL);
	// todo:
	// However, in reality we shouldn't be re-copying and shifting all this information
	// that is already lying around. This should all be in some sort of linked list or lists.
	sortindex = Z_Calloc(sizeof(size_t) * (numplanes + numpolyplanes + numwalls), PU_STATIC, NULL);

	PS_START_TIMING(ps_hw_nodesorttime);

	for (i = 0; i < numplanes; i++, p++)
	{
		sortnode[p].plane = &planeinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numpolyplanes; i++, p++)
	{
		sortnode[p].polyplane = &polyplaneinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numwalls; i++, p++)
	{
		sortnode[p].wall = &wallinfo[i];
		sortindex[p] = p;
	}

	ps_numdrawnodes.value.i = p;

	// p is the number of stuff to sort


	// sort the list based on the value of the 'drawcount' member of the drawnodes.
	qsort(sortindex, p, sizeof(size_t), CompareDrawNodes);

	// an additional pass is needed to correct the order of consecutive planes in the list.
	// for each consecutive run of planes in the list, sort that run based on plane height and view height.
	while (run_start < p-1)// p-1 because a 1 plane run at the end of the list does not count
	{
		// locate run start
		if (sortnode[sortindex[run_start]].plane)
		{
			// found it, now look for run end
			size_t run_end;// (inclusive)
			for (i = run_start+1; i < p; i++)// size_t and UINT32 being used mixed here... shouldnt break anything though..
			{
				if (!sortnode[sortindex[i]].plane) break;
			}
			run_end = i-1;
			if (run_end > run_start)// if there are multiple consecutive planes, not just one
			{
				// consecutive run of planes found, now sort it
				qsort(sortindex + run_start, run_end - run_start + 1, sizeof(size_t), CompareDrawNodePlanes);
			}
			run_start = run_end + 1;// continue looking for runs coming right after this one
		}
		else
		{
			// this wasnt the run start, try next one
			run_start++;
		}
	}


	PS_STOP_TIMING(ps_hw_nodesorttime);

	PS_START_TIMING(ps_hw_nodedrawtime);

	// Okay! Let's draw it all! Woo!
	HWD.pfnSetTransform(&atransform);
	HWD.pfnSetShader(0);
	for (i = 0; i < p; i++)
	{
		if (sortnode[sortindex[i]].plane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].plane->blend & PF_NoTexture))
				HWR_GetFlat(sortnode[sortindex[i]].plane->lumpnum);
HWR_RenderPlane(NULL, sortnode[sortindex[i]].plane->xsub, sortnode[sortindex[i]].plane->isceiling, sortnode[sortindex[i]].plane->fixedheight, sortnode[sortindex[i]].plane->blend, sortnode[sortindex[i]].plane->lightlevel,
				sortnode[sortindex[i]].plane->lumpnum, sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->alpha, sortnode[sortindex[i]].plane->fogplane, sortnode[sortindex[i]].plane->planecolormap);

		}
		else if (sortnode[sortindex[i]].polyplane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].polyplane->blend & PF_NoTexture))
				HWR_GetFlat(sortnode[sortindex[i]].polyplane->lumpnum);
			HWR_RenderPolyObjectPlane(sortnode[sortindex[i]].polyplane->polysector, sortnode[sortindex[i]].polyplane->isceiling, sortnode[sortindex[i]].polyplane->fixedheight, sortnode[sortindex[i]].polyplane->blend, sortnode[sortindex[i]].polyplane->lightlevel,
				sortnode[sortindex[i]].polyplane->lumpnum, sortnode[sortindex[i]].polyplane->FOFSector, sortnode[sortindex[i]].polyplane->alpha, sortnode[sortindex[i]].polyplane->planecolormap);
		}
		else if (sortnode[sortindex[i]].wall)
		{
			if (!(sortnode[sortindex[i]].wall->blend & PF_NoTexture))
				HWR_GetTexture(sortnode[sortindex[i]].wall->texnum);
			HWR_RenderWall(sortnode[sortindex[i]].wall->wallVerts, &sortnode[sortindex[i]].wall->Surf, sortnode[sortindex[i]].wall->blend, sortnode[sortindex[i]].wall->fogwall,
				sortnode[sortindex[i]].wall->lightlevel, sortnode[sortindex[i]].wall->wallcolormap);
		}
	}

	PS_STOP_TIMING(ps_hw_nodedrawtime);

	numwalls = 0;
	numplanes = 0;
	numpolyplanes = 0;

	// No mem leaks, please.
	Z_Free(sortnode);
	Z_Free(sortindex);
}


// --------------------------------------------------------------------------
//  Draw all vissprites
// --------------------------------------------------------------------------
// added the stransform so they can be switched as drawing happenes so MD2s and sprites are sorted correctly with each other
static void HWR_DrawSprites(void)
{
	UINT32 i;
	for (i = 0; i < gr_visspritecount; i++)
	{
		gr_vissprite_t *spr = gr_vsprorder[i];
		if (spr->precip)
			HWR_DrawPrecipitationSprite(spr);
		else
			if (spr->mobj && spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			{
				// 8/1/19: Only don't display player models if no default SPR_PLAY is found.
				if (!cv_grmd2.value || ((md2_playermodels[(skin_t*)spr->mobj->skin-skins].notfound || md2_playermodels[(skin_t*)spr->mobj->skin-skins].scale < 0.0f) && (md2_models[SPR_PLAY].notfound || md2_models[SPR_PLAY].scale < 0.0f)))
					HWR_DrawSprite(spr);
				else
					HWR_DrawMD2(spr);
			}
			else
			{
				if (!cv_grmd2.value || md2_models[spr->mobj->sprite].notfound || md2_models[spr->mobj->sprite].scale < 0.0f)
					HWR_DrawSprite(spr);
				else
					HWR_DrawMD2(spr);
			}
	}
}

// --------------------------------------------------------------------------
// HWR_AddSprites
// During BSP traversal, this adds sprites by sector.
// --------------------------------------------------------------------------
static UINT8 sectorlight;
static void HWR_AddSprites(sector_t *sec)
{
	mobj_t *thing;
#ifdef HWPRECIP
	precipmobj_t *precipthing;
#endif
	fixed_t limit_dist, hoop_limit_dist;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	// sprite lighting
	sectorlight = sec->lightlevel & 0xff;

	// Handle all things in sector.
	// If a limit exists, handle things a tiny bit different.
	limit_dist = (fixed_t)(cv_drawdist.value) << FRACBITS;
	hoop_limit_dist = (fixed_t)(cv_drawdist_nights.value) << FRACBITS;
	for (thing = sec->thinglist; thing; thing = thing->snext)
	{
		if (R_ThingVisibleWithinDist(thing, limit_dist, hoop_limit_dist))
			HWR_ProjectSprite(thing);
	}

#ifdef HWPRECIP
	// Someone seriously wants infinite draw distance for precipitation?
	if ((limit_dist = (fixed_t)cv_drawdist_precip.value << FRACBITS))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (R_PrecipThingVisible(precipthing, limit_dist))
				HWR_ProjectPrecipitationSprite(precipthing);
		}
	}
#endif
}

// --------------------------------------------------------------------------
// HWR_ProjectSprite
//  Generates a vissprite for a thing if it might be visible.
// --------------------------------------------------------------------------
// BP why not use xtoviexangle/viewangletox like in bsp ?....
static void HWR_ProjectSprite(mobj_t *thing)
{
	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	float this_scale;
	float gz, gzt;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	md2_t *md2;
	size_t lumpoff;
	unsigned rot;
	UINT8 flip;
	angle_t ang;
	INT32 heightsec, phs;

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	if (!thing)
		return;
	else

	if (R_UsingFrameInterpolation())
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	this_scale = FIXED_TO_FLOAT(interp.scale);

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(interp.x) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(interp.y) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?

	if (tz < ZCLIP_PLANE)
	{
		if (cv_grmd2.value) //Yellow: Only MD2's dont disappear
		{
			if (thing->skin && thing->sprite == SPR_PLAY)
				md2 = &md2_playermodels[( (skin_t *)thing->skin - skins )];
			else
				md2 = &md2_models[thing->sprite];

			if (md2->notfound || md2->scale < 0.0f)
				return;
		}
		else
			return;
	}

	// The above can stay as it works for cutting sprites that are too close
	tr_x = FIXED_TO_FLOAT(interp.x);
	tr_y = FIXED_TO_FLOAT(interp.y);

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= numsprites)
		I_Error("HWR_ProjectSprite: invalid sprite number %i ", thing->sprite);
#endif

	rot = thing->frame&FF_FRAMEMASK;

	//Fab : 02-08-98: 'skin' override spritedef currently used for skin
	if (thing->skin && thing->sprite == SPR_PLAY)
		sprdef = &((skin_t *)thing->skin)->spritedef;
	else
		sprdef = &sprites[thing->sprite];

	if (rot >= sprdef->numframes)
	{
		CONS_Alert(CONS_ERROR, M_GetText("HWR_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
			sizeu1(rot), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
		thing->sprite = states[S_UNKNOWN].sprite;
		thing->frame = states[S_UNKNOWN].frame;
		sprdef = &sprites[thing->sprite];
		rot = thing->frame&FF_FRAMEMASK;
		thing->state->sprite = thing->sprite;
		thing->state->frame = thing->frame;
	}

	sprframe = &sprdef->spriteframes[rot];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

	if (sprframe->rotate)
	{
		// choose a different rotation based on player view
		ang = R_PointToAngle (interp.x, interp.y);
		rot = (ang-interp.angle+ANGLE_202h)>>29;
		//Fab: lumpid is the index for spritewidth,spriteoffset... tables
		lumpoff = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);
	}
	else
	{
		// use single rotation for all views
		rot = 0;                        //Fab: for vis->patch below
		lumpoff = sprframe->lumpid[0];     //Fab: see note above
		flip = sprframe->flip; // Will only be 0x00 or 0xFF
	}

	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)thing->skin)->highresscale);

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset) * this_scale);
	}
	else
	{
		x1 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset) * this_scale);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

	if (thing->eflags & MFE_VERTICALFLIP)
	{
		gz = FIXED_TO_FLOAT(interp.z + thing->height) - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gzt = gz + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
	}
	else
	{
		gzt = FIXED_TO_FLOAT(interp.z) + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gz = gzt - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
	}

	if (thing->subsector->sector->cullheight)
	{
		if (HWR_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, gr_viewz, gz, gzt))
			return;
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		if (gr_viewz < FIXED_TO_FLOAT(sectors[phs].floorheight) ?
		FIXED_TO_FLOAT(thing->z) >= FIXED_TO_FLOAT(sectors[heightsec].floorheight) :
		gzt < FIXED_TO_FLOAT(sectors[heightsec].floorheight))
			return;
		if (gr_viewz > FIXED_TO_FLOAT(sectors[phs].ceilingheight) ?
		gzt < FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) && gr_viewz >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) :
		FIXED_TO_FLOAT(thing->z) >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight))
			return;
	}

	// store information in a vissprite
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz; // Keep tz for the simple sprite sorting that happens
	vis->dispoffset = thing->info->dispoffset; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	vis->patchlumpnum = sprframe->lumppat[rot];
	vis->flip = flip;
	vis->mobj = thing;
	vis->scale = this_scale;

	//Hurdler: 25/04/2000: now support colormap in hardware mode
	if ((vis->mobj->flags & MF_BOSS) && (vis->mobj->flags2 & MF2_FRET) && (leveltime & 1)) // Bosses "flash"
	{
		if (vis->mobj->type == MT_CYBRAKDEMON)
			vis->colormap = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		else if (vis->mobj->type == MT_METALSONIC_BATTLE)
			vis->colormap = R_GetTranslationColormap(TC_METALSONIC, 0, GTC_CACHE);
		else
			vis->colormap = R_GetTranslationColormap(TC_BOSS, 0, GTC_CACHE);
	}
	else if (thing->color)
	{
		// New colormap stuff for skins Tails 06-07-2002
		if (thing->skin && thing->sprite == SPR_PLAY) // This thing is a player!
		{
			size_t skinnum = (skin_t*)thing->skin-skins;
			vis->colormap = R_GetTranslationColormap((INT32)skinnum, thing->color, GTC_CACHE);
		}
		else
			vis->colormap = R_GetTranslationColormap(TC_DEFAULT, vis->mobj->color ? vis->mobj->color : SKINCOLOR_CYAN, GTC_CACHE);
	}
	else
		vis->colormap = colormaps;

	// set top/bottom coords
	vis->ty = gzt;

	//CONS_Debug(DBG_RENDER, "------------------\nH: sprite  : %d\nH: frame   : %x\nH: type    : %d\nH: sname   : %s\n\n",
	//            thing->sprite, thing->frame, thing->type, sprnames[thing->sprite]);

	if (thing->eflags & MFE_VERTICALFLIP)
		vis->vflip = true;
	else
		vis->vflip = false;

	vis->precip = false;
}

#ifdef HWPRECIP
// Precipitation projector for hardware mode
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lumpoff;
	unsigned rot = 0;
	UINT8 flip;

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	if (!thing)
		return;

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolatePrecipMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolatePrecipMobjState(thing, FRACUNIT, &interp);
	}

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(interp.x) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(interp.y) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?
	if (tz < ZCLIP_PLANE)
		return;

	tr_x = FIXED_TO_FLOAT(interp.x);
	tr_y = FIXED_TO_FLOAT(interp.y);

	// decide which patch to use for sprite relative to player
	if ((unsigned)thing->sprite >= numsprites)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite number %i ",
		        thing->sprite);
#else
		return;
#endif

	sprdef = &sprites[thing->sprite];

	if ((size_t)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite frame %i : %i for %s",
		        thing->sprite, thing->frame, sprnames[thing->sprite]);
#else
		return;
#endif

	sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

	// use single rotation for all views
	lumpoff = sprframe->lumpid[0];
	flip = sprframe->flip; // Will only be 0x00 or 0xFF

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
	}
	else
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

	// okay, we can't return now... this is a hack, but weather isn't networked, so it should be ok
	if (!(thing->precipflags & PCF_THUNK))
	{
		if (thing->precipflags & PCF_RAIN)
			P_RainThinker(thing);
		else
			P_SnowThinker(thing);
		thing->precipflags |= PCF_THUNK;
	}

	//
	// store information in a vissprite
	//
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz;
	vis->dispoffset = 0; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	vis->patchlumpnum = sprframe->lumppat[rot];
	vis->flip = flip;
	vis->mobj = (mobj_t *)thing;

	vis->colormap = colormaps;

	// set top/bottom coords
	vis->ty = FIXED_TO_FLOAT(thing->z + spritecachedinfo[lumpoff].topoffset);

	vis->precip = true;
}
#endif

// ==========================================================================
// Sky dome rendering, ported from PrBoom+
// ==========================================================================

static gl_sky_t gl_sky;

static void HWR_SkyDomeVertex(gl_sky_t *sky, gl_skyvertex_t *vbo, int r, int c, signed char yflip, float delta, boolean foglayer)
{
	const float radians = (float)(M_PIl / 180.0f);
	const float scale = 10000.0f;
	const float maxSideAngle = 60.0f;

	float topAngle = (c / (float)sky->columns * 360.0f);
	float sideAngle = (maxSideAngle * (sky->rows - r) / sky->rows);
	float height = (float)(sin(sideAngle * radians));
	float realRadius = (float)(scale * cos(sideAngle * radians));
	float x = (float)(realRadius * cos(topAngle * radians));
	float y = (!yflip) ? scale * height : -scale * height;
	float z = (float)(realRadius * sin(topAngle * radians));
	float timesRepeat = (4 * (256.0f / sky->width));
	if (fpclassify(timesRepeat) == FP_ZERO)
		timesRepeat = 1.0f;

	if (!foglayer)
	{
		vbo->r = 255;
		vbo->g = 255;
		vbo->b = 255;
		vbo->a = (r == 0 ? 0 : 255);

		// And the texture coordinates.
		vbo->u = (-timesRepeat * c / (float)sky->columns);
		if (!yflip)	// Flipped Y is for the lower hemisphere.
			vbo->v = (r / (float)sky->rows) + 0.5f;
		else
			vbo->v = 1.0f + ((sky->rows - r) / (float)sky->rows) + 0.5f;
	}

	if (r != 4)
		y += 300.0f;

	// And finally the vertex.
	vbo->x = x;
	vbo->y = y + delta;
	vbo->z = z;
}

// Clears the sky dome.
void HWR_ClearSkyDome(void)
{
	gl_sky_t *sky = &gl_sky;

	if (sky->loops)
		free(sky->loops);
	if (sky->data)
		free(sky->data);

	sky->loops = NULL;
	sky->data = NULL;

	sky->vbo = 0;
	sky->rows = sky->columns = 0;
	sky->loopcount = 0;

	sky->detail = 0;
	sky->texture = -1;
	sky->width = sky->height = 0;

	sky->rebuild = true;
}

void HWR_BuildSkyDome(void)
{
	int c, r;
	signed char yflip;
	int row_count = 4;
	int col_count = 4;
	float delta;

	gl_sky_t *sky = &gl_sky;
	gl_skyvertex_t *vertex_p;
	texture_t *texture = textures[texturetranslation[skytexture]];

	sky->detail = 16;
	col_count *= sky->detail;

	if ((sky->columns != col_count) || (sky->rows != row_count))
		HWR_ClearSkyDome();

	sky->columns = col_count;
	sky->rows = row_count;
	sky->vertex_count = 2 * sky->rows * (sky->columns * 2 + 2) + sky->columns * 2;

	if (!sky->loops)
		sky->loops = malloc((sky->rows * 2 + 2) * sizeof(sky->loops[0]));

	// create vertex array
	if (!sky->data)
		sky->data = malloc(sky->vertex_count * sizeof(sky->data[0]));

	sky->texture = texturetranslation[skytexture];
	sky->width = texture->width;
	sky->height = texture->height;

	vertex_p = &sky->data[0];
	sky->loopcount = 0;

	for (yflip = 0; yflip < 2; yflip++)
	{
		sky->loops[sky->loopcount].mode = HWD_SKYLOOP_FAN;
		sky->loops[sky->loopcount].vertexindex = vertex_p - &sky->data[0];
		sky->loops[sky->loopcount].vertexcount = col_count;
		sky->loops[sky->loopcount].use_texture = false;
		sky->loopcount++;

		delta = 0.0f;

		for (c = 0; c < col_count; c++)
		{
			HWR_SkyDomeVertex(sky, vertex_p, 1, c, yflip, 0.0f, true);
			vertex_p->r = 255;
			vertex_p->g = 255;
			vertex_p->b = 255;
			vertex_p->a = 255;
			vertex_p++;
		}

		delta = (yflip ? 5.0f : -5.0f) / 128.0f;

		for (r = 0; r < row_count; r++)
		{
			sky->loops[sky->loopcount].mode = HWD_SKYLOOP_STRIP;
			sky->loops[sky->loopcount].vertexindex = vertex_p - &sky->data[0];
			sky->loops[sky->loopcount].vertexcount = 2 * col_count + 2;
			sky->loops[sky->loopcount].use_texture = true;
			sky->loopcount++;

			for (c = 0; c <= col_count; c++)
			{
				HWR_SkyDomeVertex(sky, vertex_p++, r + (yflip ? 1 : 0), (c ? c : 0), yflip, delta, false);
				HWR_SkyDomeVertex(sky, vertex_p++, r + (yflip ? 0 : 1), (c ? c : 0), yflip, delta, false);
			}
		}
	}
}

static void HWR_DrawSkyBackground(player_t *player)
{
	if (HWR_IsWireframeMode())
		return;

	HWD.pfnSetBlend(PF_Translucent|PF_NoDepthTest|PF_Modulated);

	if (cv_skydome.value)
	{
		FTransform dometransform;
		const float fpov = FixedToFloat(R_GetPlayerFov(player));
		postimg_t *type;

		if (splitscreen && player == &players[secondarydisplayplayer])
			type = &postimgtype2;
		else
			type = &postimgtype;

		memset(&dometransform, 0x00, sizeof(FTransform));

		//04/01/2000: Hurdler: added for T&L
		//                     It should replace all other gr_viewxxx when finished
		HWR_SetTransformAiming(&dometransform, player, false);
		dometransform.angley = (float)((viewangle-ANGLE_270)>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

		if (*type == postimg_flip)
			dometransform.flip = true;
		else
			dometransform.flip = false;

		dometransform.scalex = 1;
		dometransform.scaley = (float)vid.width/vid.height;
		dometransform.scalez = 1;
		dometransform.fovxangle = fpov; // Tails
		dometransform.fovyangle = fpov; // Tails
		if (player->viewrollangle != 0)
		{
			fixed_t rol = AngleFixed(player->viewrollangle);
			dometransform.rollangle = FIXED_TO_FLOAT(rol);
			dometransform.roll = true;
		}
		dometransform.splitscreen = splitscreen;

		HWR_GetTexture(texturetranslation[skytexture]);

		if (gl_sky.texture != texturetranslation[skytexture])
		{
			HWR_ClearSkyDome();
			HWR_BuildSkyDome();
		}

		HWD.pfnSetShader(SHADER_SKY); // sky shader
		HWD.pfnSetTransform(&dometransform);
		HWD.pfnRenderSkyDome(&gl_sky);
	}
	else
	{
		FOutVector v[4];
		angle_t angle;
		float dimensionmultiply;
		float aspectratio;
		float angleturn;

		HWR_GetTexture(texturetranslation[skytexture]);
		aspectratio = (float)vid.width/(float)vid.height;

		//Hurdler: the sky is the only texture who need 4.0f instead of 1.0
		//         because it's called just after clearing the screen
		//         and thus, the near clipping plane is set to 3.99
		// Sryder: Just use the near clipping plane value then

		//  3--2
		//  | /|
		//  |/ |
		//  0--1
		v[0].x = v[3].x = -ZCLIP_PLANE-1;
		v[1].x = v[2].x =  ZCLIP_PLANE+1;
		v[0].y = v[1].y = -ZCLIP_PLANE-1;
		v[2].y = v[3].y =  ZCLIP_PLANE+1;

		v[0].z = v[1].z = v[2].z = v[3].z = ZCLIP_PLANE+1;

		// X

		// NOTE: This doesn't work right with texture widths greater than 1024
		// software doesn't draw any further than 1024 for skies anyway, but this doesn't overlap properly
		// The only time this will probably be an issue is when a sky wider than 1024 is used as a sky AND a regular wall texture

		angle = (dup_viewangle + gr_xtoviewangle[0]);

		dimensionmultiply = ((float)textures[texturetranslation[skytexture]]->width/256.0f);

		v[0].s = v[3].s = (-1.0f * angle) / (((float)ANGLE_90-1.0f)*dimensionmultiply); // left
		v[2].s = v[1].s = v[0].s + (1.0f/dimensionmultiply); // right (or left + 1.0f)
		// use +angle and -1.0f above instead if you wanted old backwards behavior

		// Y
		angle = aimingangle;
		dimensionmultiply = ((float)textures[texturetranslation[skytexture]]->height/(128.0f*aspectratio));

		if (splitscreen)
		{
			dimensionmultiply *= 2;
			angle *= 2;
		}

		// Middle of the sky should always be at angle 0
		// need to keep correct aspect ratio with X
		if (atransform.flip)
		{
			// During vertical flip the sky should be flipped and it's y movement should also be flipped obviously
			v[3].t = v[2].t = -(0.5f-(0.5f/dimensionmultiply)); // top
			v[0].t = v[1].t = v[3].t - (1.0f/dimensionmultiply); // bottom (or top - 1.0f)
		}
		else
		{
			v[0].t = v[1].t = -(0.5f-(0.5f/dimensionmultiply)); // bottom
			v[3].t = v[2].t = v[0].t - (1.0f/dimensionmultiply); // top (or bottom - 1.0f)
		}

		angleturn = (((float)ANGLE_45-1.0f)*aspectratio)*dimensionmultiply;

		if (angle > ANGLE_180) // Do this because we don't want the sky to suddenly teleport when crossing over 0 to 360 and vice versa
		{
			angle = InvAngle(angle);
			v[3].t = v[2].t += ((float) angle / angleturn);
			v[0].t = v[1].t += ((float) angle / angleturn);
		}
		else
		{
			v[3].t = v[2].t -= ((float) angle / angleturn);
			v[0].t = v[1].t -= ((float) angle / angleturn);
		}

		HWD.pfnUnSetShader();
		HWD.pfnDrawPolygon(NULL, v, 4, 0);
	}

	HWD.pfnSetShader(0);
}




// -----------------+
// HWR_ClearView : clear the viewwindow, with maximum z value
// -----------------+
static inline void HWR_ClearView(void)
{
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	/// \bug faB - enable depth mask, disable color mask

	HWD.pfnGClipRect((INT32)gr_viewwindowx,
	                 (INT32)gr_viewwindowy,
	                 (INT32)(gr_viewwindowx + gr_viewwidth),
	                 (INT32)(gr_viewwindowy + gr_viewheight),
	                 ZCLIP_PLANE);
	HWD.pfnClearBuffer(false, true, 0);

	//disable clip window - set to full size
	// rem by Hurdler
	// HWD.pfnGClipRect(0, 0, vid.width, vid.height);
}


// -----------------+
// HWR_SetViewSize  : set projection and scaling values
// -----------------+
void HWR_SetViewSize(void)
{
	// setup view size
	gr_viewwidth = (float)vid.width;
	gr_viewheight = (float)vid.height;

	if (splitscreen)
		gr_viewheight /= 2;

	gr_centerx = gr_viewwidth / 2;
	gr_basecentery = gr_viewheight / 2; //note: this is (gr_centerx * gr_viewheight / gr_viewwidth)

	gr_viewwindowx = (vid.width - gr_viewwidth) / 2;
	gr_windowcenterx = (float)(vid.width / 2);
	if (fabsf(gr_viewwidth - vid.width) < 1.0E-36f)
	{
		gr_baseviewwindowy = 0;
		gr_basewindowcentery = gr_viewheight / 2;               // window top left corner at 0,0
	}
	else
	{
		gr_baseviewwindowy = (vid.height-gr_viewheight) / 2;
		gr_basewindowcentery = (float)(vid.height / 2);
	}

	gr_pspritexscale = gr_viewwidth / BASEVIDWIDTH;
	gr_pspriteyscale = ((vid.height*gr_pspritexscale*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width;

	HWD.pfnFlushScreenTextures();
}

// Set view aiming, for the sky dome, the skybox,
// and the normal view, all with a single function.
static void HWR_SetTransformAiming(FTransform *trans, player_t *player, boolean skybox)
{
	// 1 = always on
	// 2 = chasecam only
	if (cv_grshearing.value == 1 || (cv_grshearing.value == 2 && R_IsViewpointThirdPerson(player, skybox)))
	{
		fixed_t fixedaiming = AIMINGTODY(aimingangle);
		trans->viewaiming = FIXED_TO_FLOAT(fixedaiming) * ((float)vid.width / vid.height) / ((float)BASEVIDWIDTH / BASEVIDHEIGHT);
		if (splitscreen)
			trans->viewaiming *= 2.125; // splitscreen adjusts fov with 0.8, so compensate (but only halfway, since splitscreen means only half the screen is used)
		trans->shearing = true;
		gr_aimingangle = 0;
	}
	else
	{
		trans->shearing = false;
		gr_aimingangle = aimingangle;
	}
	trans->anglex = (float)(gr_aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
}


// ==========================================================================
// Same as rendering the player view, but from the skybox object
// ==========================================================================
void HWR_RenderSkyboxView(INT32 viewnumber, player_t *player)
{
	const float fpov = FixedToFloat(R_GetPlayerFov(player));
	postimg_t *type;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	{
		// do we really need to save player (is it not the same)?
		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
	}

	// note: sets viewangle, viewx, viewy, viewz
	R_SkyboxFrame(player);

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;

	// set window position
	gr_centery = gr_basecentery;
	gr_viewwindowy = gr_baseviewwindowy;
	gr_windowcentery = gr_basewindowcentery;
	if (splitscreen && viewnumber == 1)
	{
		gr_viewwindowy += (vid.height/2);
		gr_windowcentery += (vid.height/2);
	}

	// check for new console commands.
	NetUpdate();

	gr_viewx = FIXED_TO_FLOAT(dup_viewx);
	gr_viewy = FIXED_TO_FLOAT(dup_viewy);
	gr_viewz = FIXED_TO_FLOAT(dup_viewz);
	gr_viewsin = FIXED_TO_FLOAT(viewsin);
	gr_viewcos = FIXED_TO_FLOAT(viewcos);



	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	HWR_SetTransformAiming(&atransform, player, true);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);



	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(gr_aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(gr_aimingangle>>ANGLETOFINESHIFT));


	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gr_viewx;  // FIXED_TO_FLOAT(viewx)
	atransform.y      = gr_viewy;  // FIXED_TO_FLOAT(viewy)
	atransform.z      = gr_viewz;  // FIXED_TO_FLOAT(viewz)
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;
	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
	if (player->viewrollangle != 0)
	{
		fixed_t rol = AngleFixed(player->viewrollangle);
		atransform.rollangle = FIXED_TO_FLOAT(rol);
		atransform.roll = true;
	}
	atransform.splitscreen = splitscreen;

	gr_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));

	//------------------------------------------------------------------------
	HWR_ClearView();

	if (drawsky)
		HWR_DrawSkyBackground(player);

	//Hurdler: it doesn't work in splitscreen mode
	drawsky = splitscreen;

	HWR_ClearSprites();

	drawcount = 0;

#ifdef NEWCLIP
	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle(gr_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}
#else
	HWR_ClearClipSegs();
#endif

	//04/01/2000: Hurdler: added for T&L
	//                     Actually it only works on Walls and Planes
	HWD.pfnSetTransform(&atransform);

	// Reset the shader state.
	HWD.pfnSetSpecialState(HWD_SET_SHADERS, cv_grshaders.value);
	HWD.pfnSetShader(0);

	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 1);

	validcount++;

	if (cv_grbatching.value)
		HWR_StartBatching();


	HWR_RenderBSPNode((INT32)numnodes-1);

	if (cv_grbatching.value)
		HWR_RenderBatches();

	// Check for new console commands.
	NetUpdate();

	// Draw MD2 and sprites

	HWR_SortVisSprites();

	HWR_DrawSprites();

#ifdef NEWCORONAS
	//Hurdler: they must be drawn before translucent planes, what about gl fog?
	HWR_DrawCoronas();
#endif


	if (numplanes || numpolyplanes || numwalls) //Hurdler: render 3D water and transparent walls after everything
	{
		HWR_CreateDrawNodes();
	}



	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 0);

	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();


	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}

// ==========================================================================
//
// ==========================================================================
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player)
{

	const float fpov = FixedToFloat(R_GetPlayerFov(player));
	postimg_t *type;

	const boolean skybox = (skyboxmo[0] && cv_skybox.value); // True if there's a skybox object and skyboxes are on

	FRGBAFloat ClearColor;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	ClearColor.red = 0.0f;
	ClearColor.green = 0.0f;
	ClearColor.blue = 0.0f;
	ClearColor.alpha = 1.0f;

	if (cv_grshaders.value)
		HWD.pfnSetShaderInfo(HWD_SHADERINFO_LEVELTIME, (INT32)leveltime); // The water surface shader needs the leveltime.

	if (viewnumber == 0) // Only do it if it's the first screen being rendered
		HWD.pfnClearBuffer(true, false, &ClearColor); // Clear the Color Buffer, stops HOMs. Also seems to fix the skybox issue on Intel GPUs.

	PS_START_TIMING(ps_skyboxtime);
	if (skybox && drawsky) // If there's a skybox and we should be drawing the sky, draw the skybox
		HWR_RenderSkyboxView(viewnumber, player); // This is drawn before everything else so it is placed behind
	PS_STOP_TIMING(ps_skyboxtime);

	{
		// do we really need to save player (is it not the same)?
		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
	}

	// note: sets viewangle, viewx, viewy, viewz
	R_SetupFrame(player, false); // This can stay false because it is only used to set viewsky in r_main.c, which isn't used here

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;

	// set window position
	gr_centery = gr_basecentery;
	gr_viewwindowy = gr_baseviewwindowy;
	gr_windowcentery = gr_basewindowcentery;
	if (splitscreen && viewnumber == 1)
	{
		gr_viewwindowy += (vid.height/2);
		gr_windowcentery += (vid.height/2);
	}

	// check for new console commands.
	NetUpdate();

	gr_viewx = FIXED_TO_FLOAT(dup_viewx);
	gr_viewy = FIXED_TO_FLOAT(dup_viewy);
	gr_viewz = FIXED_TO_FLOAT(dup_viewz);
	gr_viewsin = FIXED_TO_FLOAT(viewsin);
	gr_viewcos = FIXED_TO_FLOAT(viewcos);



	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	HWR_SetTransformAiming(&atransform, player, false);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);



	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(gr_aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(gr_aimingangle>>ANGLETOFINESHIFT));


	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gr_viewx;  // FIXED_TO_FLOAT(viewx)
	atransform.y      = gr_viewy;  // FIXED_TO_FLOAT(viewy)
	atransform.z      = gr_viewz;  // FIXED_TO_FLOAT(viewz)
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;
	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
	if (player->viewrollangle != 0)
	{
		fixed_t rol = AngleFixed(player->viewrollangle);
		atransform.rollangle = FIXED_TO_FLOAT(rol);
		atransform.roll = true;
	}
	atransform.splitscreen = splitscreen;

	gr_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));

	//------------------------------------------------------------------------
	HWR_ClearView(); // Clears the depth buffer and resets the view I believe


	if (!skybox && drawsky) // Don't draw the regular sky if there's a skybox
		HWR_DrawSkyBackground(player);

	//Hurdler: it doesn't work in splitscreen mode
	drawsky = splitscreen;

	HWR_ClearSprites();


	drawcount = 0;
#ifdef NEWCLIP
	if (rendermode == render_opengl)
	{
		angle_t a1 = gld_FrustumAngle(gr_aimingangle);
		gld_clipper_Clear();
		gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
#ifdef HAVE_SPHEREFRUSTRUM
		gld_FrustrumSetup();
#endif
	}
#else
	HWR_ClearClipSegs();
#endif

	//04/01/2000: Hurdler: added for T&L
	//                     Actually it only works on Walls and Planes
	HWD.pfnSetTransform(&atransform);


	// Reset the shader state.
	HWD.pfnSetSpecialState(HWD_SET_SHADERS, cv_grshaders.value);
	HWD.pfnSetShader(0);


	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 1);

	ps_numbspcalls.value.i = 0;
	ps_numpolyobjects.value.i = 0;
	PS_START_TIMING(ps_bsptime);

	validcount++;

	if (cv_grbatching.value)
		HWR_StartBatching();

	HWR_RenderBSPNode((INT32)numnodes-1);

	PS_STOP_TIMING(ps_bsptime);


	if (cv_grbatching.value)
		HWR_RenderBatches();

	// Check for new console commands.
	NetUpdate();

	// Draw MD2 and sprites
	ps_numsprites.value.i = gr_visspritecount;
	PS_START_TIMING(ps_hw_spritesorttime);
	HWR_SortVisSprites();
	PS_STOP_TIMING(ps_hw_spritesorttime);
	PS_START_TIMING(ps_hw_spritedrawtime);
	HWR_DrawSprites();
	PS_STOP_TIMING(ps_hw_spritedrawtime);

#ifdef NEWCORONAS
	//Hurdler: they must be drawn before translucent planes, what about gl fog?
	HWR_DrawCoronas();
#endif

	ps_numdrawnodes.value.i = 0;
	ps_hw_nodesorttime.value.p = 0;
	ps_hw_nodedrawtime.value.p = 0;
	if (numplanes || numpolyplanes || numwalls) //Hurdler: render 3D water and transparent walls after everything
	{
		HWR_CreateDrawNodes();
	}


	if (HWR_IsWireframeMode())
		HWD.pfnSetSpecialState(HWD_SET_WIREFRAME, 0);

	HWD.pfnSetTransform(NULL);
	HWD.pfnUnSetShader();


	HWR_DoPostProcessor(player);

	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}


// ==========================================================================
//                                                         3D ENGINE COMMANDS
// ==========================================================================



static void Command_GrStats_f(void)
{
	Z_CheckHeap(9875); // debug

	CONS_Printf(M_GetText("Patch info headers: %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRPATCHINFO)>>10));
	CONS_Printf(M_GetText("3D Texture cache  : %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRCACHE)>>10));
	CONS_Printf(M_GetText("Plane polygon     : %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRPLANE)>>10));
}



// --------------------------------------------------------------------------
// Add hardware engine commands & consvars
// --------------------------------------------------------------------------
//added by Hurdler: console varibale that are saved
void HWR_AddCommands(void)
{
	CV_RegisterVar(&cv_grrounddown);
	CV_RegisterVar(&cv_grfiltermode);
	CV_RegisterVar(&cv_granisotropicmode);
	CV_RegisterVar(&cv_grcorrecttricks);
	CV_RegisterVar(&cv_grsolvetjoin);
	CV_RegisterVar(&cv_grbatching);
	CV_RegisterVar(&cv_grwireframe);
	CV_RegisterVar(&cv_grmodellighting);
	CV_RegisterVar(&cv_glloadingscreen);
	CV_RegisterVar(&cv_grshearing);
	CV_RegisterVar(&cv_grshaders);
	CV_RegisterVar(&cv_grfakecontrast);
	CV_RegisterVar(&cv_grslopecontrast);
	CV_RegisterVar(&cv_grmd2);
	CV_RegisterVar(&cv_grmodelinterpolation);
	CV_RegisterVar(&cv_grspritebillboarding);
}

static inline void HWR_AddEngineCommands(void)
{
	// engine state variables
	//CV_RegisterVar(&cv_grzbuffer);
#ifndef NEWCLIP
	CV_RegisterVar(&cv_grclipwalls);
#endif

	// engine development mode variables
	// - usage may vary from version to version..
	CV_RegisterVar(&cv_gralpha);
	CV_RegisterVar(&cv_grbeta);

	// engine commands
	COM_AddCommand("gr_stats", Command_GrStats_f);
}


// --------------------------------------------------------------------------
// Setup the hardware renderer
// --------------------------------------------------------------------------
void HWR_Startup(void)
{
	static boolean startupdone = false;


	// do this once
	if (!startupdone)
	{
		CONS_Printf("HWR_Startup()...\n");

		HWR_InitPolyPool();
		// add console cmds & vars
		HWR_AddEngineCommands();
		HWR_InitTextureCache();

		HWR_InitMD2();


	HWR_LoadAllCustomShaders();
	if (!HWR_CompileShaders())
			gr_shadersavailable = false;
	}

	if (rendermode == render_opengl)
		textureformat = patchformat =
#ifdef _NDS
			GR_TEXFMT_P_8;
#else
			GR_RGBA;
#endif

	startupdone = true;
}


// --------------------------------------------------------------------------
// Free resources allocated by the hardware renderer
// --------------------------------------------------------------------------
void HWR_Shutdown(void)
{
	CONS_Printf("HWR_Shutdown()\n");
	HWR_FreeExtraSubsectors();
	HWR_FreePolyPool();
	HWR_FreeTextureCache();
	HWD.pfnFlushScreenTextures();
}

void transform(float *cx, float *cy, float *cz)
{
	float tr_x,tr_y;
	// translation
	tr_x = *cx - gr_viewx;
	tr_y = *cz - gr_viewy;
//	*cy = *cy;

	// rotation around vertical y axis
	*cx = (tr_x * gr_viewsin) - (tr_y * gr_viewcos);
	tr_x = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	//look up/down ----TOTAL SUCKS!!!--- do the 2 in one!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	tr_y = *cy - gr_viewz;

	*cy = (tr_x * gr_viewludcos) + (tr_y * gr_viewludsin);
	*cz = (tr_x * gr_viewludsin) - (tr_y * gr_viewludcos);

	//scale y before frustum so that frustum can be scaled to screen height
	*cy *= ORIGINAL_ASPECT * gr_fovlud;
	*cx *= gr_fovlud;
}






void HWR_AddTransparentWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	static size_t allocedwalls = 0;

	// Force realloc if buffer has been freed
	if (!wallinfo)
		allocedwalls = 0;

	if (allocedwalls < numwalls + 1)
	{
		allocedwalls += MAX_TRANSPARENTWALL;
		Z_Realloc(wallinfo, allocedwalls * sizeof (*wallinfo), PU_LEVEL, &wallinfo);
	}

	M_Memcpy(wallinfo[numwalls].wallVerts, wallVerts, sizeof (wallinfo[numwalls].wallVerts));
	M_Memcpy(&wallinfo[numwalls].Surf, pSurf, sizeof (FSurfaceInfo));
	wallinfo[numwalls].texnum = texnum;
	wallinfo[numwalls].blend = blend;
	wallinfo[numwalls].drawcount = drawcount++;
	wallinfo[numwalls].fogwall = fogwall;
	wallinfo[numwalls].lightlevel = lightlevel;
	wallinfo[numwalls].wallcolormap = wallcolormap;
	numwalls++;
}


void HWR_RenderWall(FOutVector *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	FBITFIELD blendmode = blend;
	UINT8 alpha = pSurf->PolyColor.s.alpha; // retain the alpha

	INT32 shader = SHADER_DEFAULT;

	// Lighting is done here instead so that fog isn't drawn incorrectly on transparent walls after sorting
	HWR_Lighting(pSurf, lightlevel, wallcolormap);

	pSurf->PolyColor.s.alpha = alpha; // put the alpha back after lighting


	if (blend & PF_Environment)
		blendmode |= PF_Occlude;	// PF_Occlude must be used for solid objects

	if (HWR_UseShader())
	{
		if (fogwall)
		shader = SHADER_FOG;
	else
		shader = SHADER_WALL;

	blendmode |= PF_ColorMapped;
	}

	if (fogwall)
		blendmode |= PF_Fog;

	blendmode |= PF_Modulated;	// No PF_Occlude means overlapping (incorrect) transparency

	HWR_ProcessPolygon(pSurf, wallVerts, 4, blendmode, shader, false);


#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);
#endif
}

INT32 HWR_GetTextureUsed(void)
{
	return HWD.pfnGetTextureUsed();
}

void HWR_DoPostProcessor(player_t *player)
{
	postimg_t *type;

	HWD.pfnUnSetShader();


	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	// Armageddon Blast Flash!
	// Could this even be considered postprocessor?
	if (player->flashcount)
	{
		FOutVector      v[4];
		FSurfaceInfo Surf;

		v[0].x = v[2].y = v[3].x = v[3].y = -4.0f;
		v[0].y = v[1].x = v[1].y = v[2].x = 4.0f;
		v[0].z = v[1].z = v[2].z = v[3].z = 4.0f; // 4.0 because of the same reason as with the sky, just after the screen is cleared so near clipping plane is 3.99

		// This won't change if the flash palettes are changed unfortunately, but it works for its purpose
		if (player->flashpal == PAL_NUKE)
		{
			Surf.PolyColor.s.red = 0xff;
			Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0x7F; // The nuke palette is kind of pink-ish
		}
		else
			Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		Surf.PolyColor.s.alpha = 0xc0; // match software mode

		HWD.pfnDrawPolygon(&Surf, v, 4, PF_Modulated|PF_Additive|PF_NoTexture|PF_NoDepthTest);
	}

	// Capture the screen for intermission and screen waving
	if(gamestate != GS_INTERMISSION)
		HWD.pfnMakeScreenTexture();

	if (splitscreen) // Not supported in splitscreen - someone want to add support?
		return;

	// Drunken vision! WooOOooo~
	if (*type == postimg_water || *type == postimg_heat)
	{
		// 10 by 10 grid. 2 coordinates (xy)
		float v[SCREENVERTS][SCREENVERTS][2];
		float disStart = (leveltime-1) + FIXED_TO_FLOAT(rendertimefrac);
		UINT8 x, y;
		INT32 WAVELENGTH;
		INT32 AMPLITUDE;
		INT32 FREQUENCY;

		// Modifies the wave.
		if (*type == postimg_water)
		{
			WAVELENGTH = 5; // Lower is longer
			AMPLITUDE = 40; // Lower is bigger
			FREQUENCY = 8; // Lower is faster
		}
		else
		{
			WAVELENGTH = 10; // Lower is longer
			AMPLITUDE = 60; // Lower is bigger
			FREQUENCY = 4; // Lower is faster
		}

		for (x = 0; x < SCREENVERTS; x++)
		{
			for (y = 0; y < SCREENVERTS; y++)
			{
				// Change X position based on its Y position.
				v[x][y][0] = (x/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f + (float)sin((disStart+(y*WAVELENGTH))/FREQUENCY)/AMPLITUDE;
				v[x][y][1] = (y/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f;
			}
		}
		HWD.pfnPostImgRedraw(v);


		// Capture the screen again for screen waving on the intermission
		if(gamestate != GS_INTERMISSION)
			HWD.pfnMakeScreenTexture();
	}
	// Flipping of the screen isn't done here anymore
}

void HWR_StartScreenWipe(void)
{
	//CONS_Debug(DBG_RENDER, "In HWR_StartScreenWipe()\n");
	HWD.pfnStartScreenWipe();
}

void HWR_EndScreenWipe(void)
{
	HWRWipeCounter = 0.0f;
	//CONS_Debug(DBG_RENDER, "In HWR_EndScreenWipe()\n");
	HWD.pfnEndScreenWipe();
}

void HWR_DrawIntermissionBG(void)
{
	HWD.pfnDrawIntermissionBG();
}

void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum)
{
	static char lumpname[9] = "FADEmmss";
	lumpnum_t lumpnum;
	size_t lsize;

	if (wipenum > 99 || scrnnum > 99) // not a valid wipe number
		return; // shouldn't end up here really, the loop should've stopped running beforehand

	// puts the numbers into the lumpname
	sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)wipenum, (UINT16)scrnnum);
	lumpnum = W_CheckNumForName(lumpname);

	if (lumpnum == LUMPERROR) // again, shouldn't be here really
		return;

	lsize = W_LumpLength(lumpnum);

	if (!(lsize == 256000 || lsize == 64000 || lsize == 16000 || lsize == 4000))
	{
		CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		return; // again, shouldn't get here if it is a bad size
	}

	HWR_GetFadeMask(lumpnum);

	HWD.pfnDoScreenWipe();

	HWRWipeCounter += 0.05f; // increase opacity of end screen

	if (HWRWipeCounter > 1.0f)
		HWRWipeCounter = 1.0f;
}

void HWR_MakeScreenFinalTexture(void)
{
    HWD.pfnMakeScreenFinalTexture();
}

void HWR_DrawScreenFinalTexture(int width, int height)
{
    HWD.pfnDrawScreenFinalTexture(width, height);
}
// jimita 18032019
static inline UINT16 HWR_FindShaderDefs(UINT16 wadnum)
{
	UINT16 i;
	lumpinfo_t *lump_p;

	lump_p = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < wadfiles[wadnum]->numlumps; i++, lump_p++)
		if (memcmp(lump_p->name, "SHADERS", 7) == 0)
			return i;

	return INT16_MAX;
}

boolean HWR_CompileShaders(void)
{
	return HWD.pfnCompileShaders();
}

customshaderxlat_t shaderxlat[] =
{
	{"Flat", SHADER_FLOOR},
	{"WallTexture", SHADER_WALL},
	{"Sprite", SHADER_SPRITE},
	{"Model", SHADER_MODEL},
	{"ModelLighting", SHADER_MODEL_LIGHTING},
	{"WaterRipple", SHADER_WATER},
	{"Fog", SHADER_FOG},
	{"Sky", SHADER_SKY},
	{NULL, 0},
};

void HWR_LoadAllCustomShaders(void)
{
	INT32 i;

	// read every custom shader
	for (i = 0; i < numwadfiles; i++)
		HWR_LoadCustomShadersFromFile(i, (wadfiles[i]->type == RET_PK3));
}

void HWR_LoadCustomShadersFromFile(UINT16 wadnum, boolean PK3)
{
	UINT16 lump;
	char *shaderdef, *line;
	char *stoken;
	char *value;
	size_t size;
	int linenum = 1;
	int shadertype = 0;
	int i;

	lump = HWR_FindShaderDefs(wadnum);
	if (lump == INT16_MAX)
		return;

	shaderdef = W_CacheLumpNumPwad(wadnum, lump, PU_CACHE);
	size = W_LumpLengthPwad(wadnum, lump);

	line = Z_Malloc(size+1, PU_STATIC, NULL);
	M_Memcpy(line, shaderdef, size);
	line[size] = '\0';

	stoken = strtok(line, "\r\n ");
	while (stoken)
	{
		if ((stoken[0] == '/' && stoken[1] == '/')
			|| (stoken[0] == '#'))// skip comments
		{
			stoken = strtok(NULL, "\r\n");
			goto skip_field;
		}

		if (!stricmp(stoken, "GLSL"))
		{
			value = strtok(NULL, "\r\n ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_lump;
			}

			if (!stricmp(value, "VERTEX"))
				shadertype = 1;
			else if (!stricmp(value, "FRAGMENT"))
				shadertype = 2;

skip_lump:
			stoken = strtok(NULL, "\r\n ");
			linenum++;
		}
		else
		{
			value = strtok(NULL, "\r\n= ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader target (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_field;
			}

			if (!shadertype)
			{
				CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				Z_Free(line);
				return;
			}

			for (i = 0; shaderxlat[i].type; i++)
			{
				if (!stricmp(shaderxlat[i].type, stoken))
				{
					size_t shader_size;
					char *shader_source;
					char *shader_lumpname;
					UINT16 shader_lumpnum;

					if (PK3)
					{
						shader_lumpname = Z_Malloc(strlen(value) + 12, PU_STATIC, NULL);
						strcpy(shader_lumpname, "Shaders/sh_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForFullNamePK3(shader_lumpname, wadnum, 0);
					}
					else
					{
						shader_lumpname = Z_Malloc(strlen(value) + 4, PU_STATIC, NULL);
						strcpy(shader_lumpname, "SH_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForNamePwad(shader_lumpname, wadnum, 0);
					}

					if (shader_lumpnum == INT16_MAX)
					{
						CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader source %s (file %s, line %d)\n", shader_lumpname, wadfiles[wadnum]->filename, linenum);
						Z_Free(shader_lumpname);
						continue;
					}

					shader_size = W_LumpLengthPwad(wadnum, shader_lumpnum);
					shader_source = Z_Malloc(shader_size, PU_STATIC, NULL);
					W_ReadLumpPwad(wadnum, shader_lumpnum, shader_source);

					HWD.pfnLoadCustomShader(shaderxlat[i].id, shader_source, shader_size, (shadertype == 2));

					Z_Free(shader_source);
					Z_Free(shader_lumpname);
				}
			}

skip_field:
			stoken = strtok(NULL, "\r\n= ");
			linenum++;
		}
	}

	Z_Free(line);
	return;
}

const char *HWR_GetShaderName(INT32 shader)
{
	INT32 i;

	if (shader)
	{
		for (i = 0; shaderxlat[i].type; i++)
		{
			if (shaderxlat[i].id == shader)
				return shaderxlat[i].type;
		}

		return "Unknown";
	}

	return "Default";
}


#endif // HWRENDER
