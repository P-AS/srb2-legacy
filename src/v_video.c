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
/// \file  v_video.c
/// \brief Gamma correction LUT stuff
///        Functions to draw patches (by post) directly to screen.
///        Functions to blit a block to the screen.

#include "doomdef.h"
#include "r_local.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "r_draw.h"
#include "console.h"

#include "i_video.h" // rendermode
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "doomstat.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

// Each screen is [vid.width*vid.height];
UINT8 *screens[5];
// screens[0] = main display window
// screens[1] = back screen, alternative blitting
// screens[2] = screenshot buffer, gif movie buffer
// screens[3] = fade screen start
// screens[4] = fade screen end, postimage tempoarary buffer

static CV_PossibleValue_t ticrate_cons_t[] = { {0, "No"}, {1, "Full"}, {2, "Compact"}, {0, NULL} };
static CV_PossibleValue_t tpscounter_cons_t[] = { {0, "No"}, {1, "Full"}, {2, "Compact"}, {0, NULL} };
consvar_t cv_ticrate = { "showfps", "No", "Displays the current framerate, in the style specified", CV_SAVE, ticrate_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL };
consvar_t cv_tpscounter = { "showtps", "No", "Displays the current ticrate, in the style specified", CV_SAVE, tpscounter_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL };
static CV_PossibleValue_t fpssize_cons_t[] = { {0, "Normal"}, {1, "Thin"}, {2, "Small"}, {0, NULL} };
consvar_t cv_fpssize = { "fpssize", "Normal", "Size of the FPS and TPS counter", CV_SAVE, fpssize_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL };

static void CV_palette_OnChange(void);

static CV_PossibleValue_t gamma_cons_t[] = {{-15, "MIN"}, {5, "MAX"}, {0, NULL}};
consvar_t cv_globalgamma = CVAR_INIT ("gamma", "0", "Sets the game brightness", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

static CV_PossibleValue_t saturation_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_globalsaturation = CVAR_INIT ("saturation", "10", "Sets the game saturation, or how bright the colors are", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);

#define huecoloursteps 4

static CV_PossibleValue_t hue_cons_t[] = {{0, "MIN"}, {(huecoloursteps*6)-1, "MAX"}, {0, NULL}};
consvar_t cv_rhue = CVAR_INIT ("rhue",  "0", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_yhue = CVAR_INIT ("yhue",  "4", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_ghue = CVAR_INIT ("ghue",  "8", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_chue = CVAR_INIT ("chue", "12", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_bhue = CVAR_INIT ("bhue", "16", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_mhue = CVAR_INIT ("mhue", "20", NULL, CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);

consvar_t cv_rgamma = CVAR_INIT ("rgamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ygamma = CVAR_INIT ("ygamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ggamma = CVAR_INIT ("ggamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_cgamma = CVAR_INIT ("cgamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_bgamma = CVAR_INIT ("bgamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_mgamma = CVAR_INIT ("mgamma", "0", NULL, CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

consvar_t cv_rsaturation = CVAR_INIT ("rsaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_ysaturation = CVAR_INIT ("ysaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_gsaturation = CVAR_INIT ("gsaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_csaturation = CVAR_INIT ("csaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_bsaturation = CVAR_INIT ("bsaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_msaturation = CVAR_INIT ("msaturation", "10", NULL, CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);


consvar_t cv_allcaps = CVAR_INIT ("allcaps", "Off", "Whether or not to show text in the console buffer in all-capital letters", 0, CV_OnOff, NULL);

static CV_PossibleValue_t constextsize_cons_t[] = {
	{V_NOSCALEPATCH, "Small"}, {V_SMALLSCALEPATCH, "Medium"}, {V_MEDSCALEPATCH, "Large"}, {0, "Huge"},
	{0, NULL}};
static void CV_constextsize_OnChange(void);
consvar_t cv_constextsize = CVAR_INIT ("con_textsize", "Medium", "Size of text in the console", CV_SAVE|CV_CALL, constextsize_cons_t, CV_constextsize_OnChange);

// local copy of the palette for V_GetColor()
RGBA_t *pLocalPalette = NULL;

/*
The following was an extremely helpful resource when developing my Colour Cube LUT.
http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter24.html
Please check it out if you're trying to maintain this.
toast 18/04/17
*/
float Cubepal[2][2][2][3];
boolean Cubeapply = false;

// returns whether to apply cube, selectively avoiding expensive operations
static boolean InitCube(void)
{
	boolean apply = false;
	UINT8 q;
	float working[2][2][2][3] = // the initial positions of the corners of the colour cube!
	{
		{
			{
				{0.0, 0.0, 0.0}, // black corner
				{0.0, 0.0, 1.0}  // blue corner
			},
			{
				{0.0, 1.0, 0.0}, // green corner
				{0.0, 1.0, 1.0}  // cyan corner
			}
		},
		{
			{
				{1.0, 0.0, 0.0}, // red corner
				{1.0, 0.0, 1.0}  // magenta corner
			},
			{
				{1.0, 1.0, 0.0}, // yellow corner
				{1.0, 1.0, 1.0}  // white corner
			}
		}
	};
	float desatur[3]; // grey
	float globalgammamul, globalgammaoffs;
	boolean doinggamma;

#define diffcons(cv) (cv.value != atoi(cv.defaultvalue))

	doinggamma = diffcons(cv_globalgamma);

#define gammascale 8
	globalgammamul = (cv_globalgamma.value ? ((255 - (gammascale*abs(cv_globalgamma.value)))/255.0) : 1.0);
	globalgammaoffs = ((cv_globalgamma.value > 0) ? ((gammascale*cv_globalgamma.value)/255.0) : 0.0);
	desatur[0] = desatur[1] = desatur[2] = globalgammaoffs + (0.33*globalgammamul);

	if (doinggamma
		|| diffcons(cv_rhue)
		|| diffcons(cv_yhue)
		|| diffcons(cv_ghue)
		|| diffcons(cv_chue)
		|| diffcons(cv_bhue)
		|| diffcons(cv_mhue)
		|| diffcons(cv_rgamma)
		|| diffcons(cv_ygamma)
		|| diffcons(cv_ggamma)
		|| diffcons(cv_cgamma)
		|| diffcons(cv_bgamma)
		|| diffcons(cv_mgamma)) // set the gamma'd/hued positions (saturation is done later)
	{
		float mod, tempgammamul, tempgammaoffs;

		apply = true;

		working[0][0][0][0] = working[0][0][0][1] = working[0][0][0][2] = globalgammaoffs;
		working[1][1][1][0] = working[1][1][1][1] = working[1][1][1][2] = globalgammaoffs+globalgammamul;

#define dohue(hue, gamma, loc) \
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		mod = ((hue % huecoloursteps)*(tempgammamul)/huecoloursteps);\
		switch (hue/huecoloursteps)\
		{\
			case 0:\
			default:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs+mod;\
				loc[2] = tempgammaoffs;\
				break;\
			case 1:\
				loc[0] = tempgammaoffs+tempgammamul-mod;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs;\
				break;\
			case 2:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs+mod;\
				break;\
			case 3:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul-mod;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 4:\
				loc[0] = tempgammaoffs+mod;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 5:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul-mod;\
				break;\
		}
		dohue(cv_rhue.value, cv_rgamma.value, working[1][0][0]);
		dohue(cv_yhue.value, cv_ygamma.value, working[1][1][0]);
		dohue(cv_ghue.value, cv_ggamma.value, working[0][1][0]);
		dohue(cv_chue.value, cv_cgamma.value, working[0][1][1]);
		dohue(cv_bhue.value, cv_bgamma.value, working[0][0][1]);
		dohue(cv_mhue.value, cv_mgamma.value, working[1][0][1]);
#undef dohue
	}

#define dosaturation(a, e) a = ((1 - work)*e + work*a)
#define docvsat(cv_sat, hue, gamma, r, g, b) \
	if diffcons(cv_sat)\
	{\
		float work, mod, tempgammamul, tempgammaoffs;\
		apply = true;\
		work = (cv_sat.value/10.0);\
		mod = ((hue % huecoloursteps)*(1.0)/huecoloursteps);\
		if (hue & huecoloursteps)\
			mod = 2-mod;\
		else\
			mod += 1;\
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		for (q = 0; q < 3; q++)\
			dosaturation(working[r][g][b][q], (tempgammaoffs+(desatur[q]*mod*tempgammamul)));\
	}

	docvsat(cv_rsaturation, cv_rhue.value, cv_rgamma.value, 1, 0, 0);
	docvsat(cv_ysaturation, cv_yhue.value, cv_ygamma.value, 1, 1, 0);
	docvsat(cv_gsaturation, cv_ghue.value, cv_ggamma.value, 0, 1, 0);
	docvsat(cv_csaturation, cv_chue.value, cv_cgamma.value, 0, 1, 1);
	docvsat(cv_bsaturation, cv_bhue.value, cv_bgamma.value, 0, 0, 1);
	docvsat(cv_msaturation, cv_mhue.value, cv_mgamma.value, 1, 0, 1);

#undef gammascale

	if diffcons(cv_globalsaturation)
	{
		float work = (cv_globalsaturation.value/10.0);

		apply = true;

		for (q = 0; q < 3; q++)
		{
			dosaturation(working[1][0][0][q], desatur[q]);
			dosaturation(working[0][1][0][q], desatur[q]);
			dosaturation(working[0][0][1][q], desatur[q]);

			dosaturation(working[1][1][0][q], 2*desatur[q]);
			dosaturation(working[0][1][1][q], 2*desatur[q]);
			dosaturation(working[1][0][1][q], 2*desatur[q]);
		}
	}

#undef dosaturation

#undef diffcons

	if (!apply)
		return false;

#define dowork(i, j, k, l) \
	if (working[i][j][k][l] > 1.0)\
		working[i][j][k][l] = 1.0;\
	else if (working[i][j][k][l] < 0.0)\
		working[i][j][k][l] = 0.0;\
	Cubepal[i][j][k][l] = working[i][j][k][l]
	for (q = 0; q < 3; q++)
	{
		dowork(0, 0, 0, q);
		dowork(1, 0, 0, q);
		dowork(0, 1, 0, q);
		dowork(1, 1, 0, q);
		dowork(0, 0, 1, q);
		dowork(1, 0, 1, q);
		dowork(0, 1, 1, q);
		dowork(1, 1, 1, q);
	}
#undef dowork

	return true;
}

/*
So it turns out that the way gamma was implemented previously, the default
colour profile of the game was messed up. Since this bad decision has been
around for a long time, and the intent is to keep the base game looking the
same, I'm not gonna be the one to remove this base modification.
toast 20/04/17
*/
const UINT8 correctiontable[256] =
	{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
	65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
	81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
	208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

// keep a copy of the palette so that we can get the RGB value for a color index at any time.
static void LoadPalette(const char *lumpname)
{
	Cubeapply = InitCube();
	lumpnum_t lumpnum = W_GetNumForName(lumpname);
	size_t i, palsize = W_LumpLength(lumpnum)/3;
	UINT8 *pal;

	Z_Free(pLocalPalette);

	pLocalPalette = Z_Malloc(sizeof (*pLocalPalette)*palsize, PU_STATIC, NULL);

	pal = W_CacheLumpNum(lumpnum, PU_CACHE);
	for (i = 0; i < palsize; i++)
	{
		pLocalPalette[i].s.red = correctiontable[*pal++];
		pLocalPalette[i].s.green = correctiontable[*pal++];
		pLocalPalette[i].s.blue = correctiontable[*pal++];
		pLocalPalette[i].s.alpha = 0xFF;

		// lerp of colour cubing! if you want, make it smoother yourself
		if (Cubeapply)
			V_CubeApply(&pLocalPalette[i].s.red, &pLocalPalette[i].s.green, &pLocalPalette[i].s.blue);
	}
}

void V_CubeApply(UINT8 *red, UINT8 *green, UINT8 *blue)
{
	float working[4][3];
	float linear;
	UINT8 q;

	if (!Cubeapply)
		return;

	linear = (*red/255.0);
#define dolerp(e1, e2) ((1 - linear)*e1 + linear*e2)
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(Cubepal[0][0][0][q], Cubepal[1][0][0][q]);
		working[1][q] = dolerp(Cubepal[0][1][0][q], Cubepal[1][1][0][q]);
		working[2][q] = dolerp(Cubepal[0][0][1][q], Cubepal[1][0][1][q]);
		working[3][q] = dolerp(Cubepal[0][1][1][q], Cubepal[1][1][1][q]);
	}
	linear = (*green/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(working[0][q], working[1][q]);
		working[1][q] = dolerp(working[2][q], working[3][q]);
	}
	linear = (*blue/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = 255*dolerp(working[0][q], working[1][q]);
		if (working[0][q] > 255.0)
			working[0][q] = 255.0;
		else if (working[0][q]  < 0.0)
			working[0][q] = 0.0;
	}
#undef dolerp

	*red = (UINT8)(working[0][0]);
	*green = (UINT8)(working[0][1]);
	*blue = (UINT8)(working[0][2]);
}

const char *R_GetPalname(UINT16 num)
{
	static char palname[8+1];
	char newpal[9] = "PLAYPAL\0";

	if (num > 0 && num <= 10000)
		snprintf(newpal, 8, "PAL%04u", num-1);

	strncpy(palname, newpal, sizeof(palname)-1);
	palname[8] = 0;
	return palname;
}

const char *GetPalette(void)
{
	if (gamestate == GS_LEVEL)
		return R_GetPalname(mapheaderinfo[gamemap-1]->palette);
	return "PLAYPAL";
}

static void LoadMapPalette(void)
{
	LoadPalette(GetPalette());
}

// -------------+
// V_SetPalette : Set the current palette to use for palettized graphics
//              :
// -------------+
void V_SetPalette(INT32 palettenum)
{
	if (!pLocalPalette)
		LoadMapPalette();

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_SetPalette(&pLocalPalette[palettenum*256]);
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(&pLocalPalette[palettenum*256]);
}

void V_SetPaletteLump(const char *pal)
{
	LoadPalette(pal);
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_SetPalette(pLocalPalette);
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(pLocalPalette);
}

static void CV_palette_OnChange(void)
{
	// reload palette
	LoadMapPalette();
	V_SetPalette(0);
}

static void CV_constextsize_OnChange(void)
{
	con_recalc = true;
}


// --------------------------------------------------------------------------
// Copy a rectangular area from one bitmap to another (8bpp)
// --------------------------------------------------------------------------
void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes)
{
	if (srcrowbytes == destrowbytes)
		M_Memcpy(destptr, srcptr, srcrowbytes * height);
	else
	{
		while (height--)
		{
			M_Memcpy(destptr, srcptr, width);

			destptr += destrowbytes;
			srcptr += srcrowbytes;
		}
	}
}

static UINT8 hudplusalpha[11]  = { 10,  8,  6,  4,  2,  0,  0,  0,  0,  0,  0};
static UINT8 hudminusalpha[11] = { 10,  9,  9,  8,  8,  7,  7,  6,  6,  5,  5};

static const UINT8 *v_colormap = NULL;
static const UINT8 *v_translevel = NULL;

static inline UINT8 standardpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return source[ofs>>FRACBITS];
}
static inline UINT8 mappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return *(v_colormap + source[ofs>>FRACBITS]);
}
static inline UINT8 translucentpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + ((source[ofs>>FRACBITS]<<8)&0xff00) + (*dest&0xff));
}
static inline UINT8 transmappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + (((*(v_colormap + source[ofs>>FRACBITS]))<<8)&0xff00) + (*dest&0xff));
}

// Draws a patch scaled to arbitrary size.
void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap)
{
	UINT8 (*patchdrawfunc)(const UINT8*, const UINT8*, fixed_t);
	UINT32 alphalevel = 0;

	fixed_t col, ofs, colfrac, rowfrac, fdup, vdup;
	INT32 dupx, dupy;
	const column_t *column;
	UINT8 *desttop, *dest, *deststart, *destend;
	const UINT8 *source, *deststop;
	fixed_t pwidth; // patch width
	fixed_t offx = 0; // x offset

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	//if (rendermode != render_soft && !con_startup)		// Why?
	if (rendermode != render_soft)
	{
		HWR_DrawStretchyFixedPatch((GLPatch_t *)patch, x, y, pscale, vscale, scrn, colormap);
		return;
	}
#endif

	patchdrawfunc = standardpdraw;

	v_translevel = NULL;
	if ((alphalevel = ((scrn & V_ALPHAMASK) >> V_ALPHASHIFT)))
	{
		if (alphalevel == 13)
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 14)
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 15)
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return; // invis
	}
	if (alphalevel)
	{
		v_translevel = transtables + ((alphalevel-1)<<FF_TRANSSHIFT);
		patchdrawfunc = translucentpdraw;
	}

	v_colormap = NULL;
	if (colormap)
	{
		v_colormap = colormap;
		patchdrawfunc = (v_translevel) ? transmappedpdraw : mappedpdraw;
	}

	dupx = vid.dupx;
	dupy = vid.dupy;
	if (scrn & V_SCALEPATCHMASK) switch ((scrn & V_SCALEPATCHMASK) >> V_SCALEPATCHSHIFT)
	{
		case 1: // V_NOSCALEPATCH
			dupx = dupy = 1;
			break;
		case 2: // V_SMALLSCALEPATCH
			dupx = vid.smalldupx;
			dupy = vid.smalldupy;
			break;
		case 3: // V_MEDSCALEPATCH
			dupx = vid.meddupx;
			dupy = vid.meddupy;
			break;
		default:
			break;
	}

	// only use one dup, to avoid stretching (har har)
	dupx = dupy = (dupx < dupy ? dupx : dupy);
	fdup = vdup = FixedMul(dupx<<FRACBITS, pscale);
	if (vscale != pscale)
		vdup = FixedMul(dupx<<FRACBITS, vscale);
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, vdup);

	// So it turns out offsets aren't scaled in V_NOSCALESTART unless V_OFFSET is applied ...poo, that's terrible
	// For now let's just at least give V_OFFSET the ability to support V_FLIP
	// I'll probably make a better fix for 2.2 where I don't have to worry about breaking existing support for stuff
	// -- Monster Iestyn 29/10/18
	{
		fixed_t offsetx = 0, offsety = 0;

		// left offset
		if (scrn & V_FLIP)
			offsetx = FixedMul((SHORT(patch->width) - SHORT(patch->leftoffset))<<FRACBITS, pscale) + 1;
		else
			offsetx = FixedMul(SHORT(patch->leftoffset)<<FRACBITS, pscale);

		// top offset
		// TODO: make some kind of vertical version of V_FLIP, maybe by deprecating V_OFFSET in future?!?
		offsety = FixedMul(SHORT(patch->topoffset)<<FRACBITS, vscale);

		if ((scrn & (V_NOSCALESTART|V_OFFSET)) == (V_NOSCALESTART|V_OFFSET)) // Multiply by dupx/dupy for crosshairs
		{
			offsetx = FixedMul(offsetx, dupx<<FRACBITS);
			offsety = FixedMul(offsety, dupy<<FRACBITS);
		}

		// Subtract the offsets from x/y positions
		x -= offsetx;
		y -= offsety;
	}

	if (scrn & V_SPLITSCREEN)
		y>>=1;

	desttop = screens[scrn&V_PARAMMASK];

	if (!desttop)
		return;

	deststop = desttop + vid.rowbytes * vid.height;

	if (scrn & V_NOSCALESTART)
	{
		x >>= FRACBITS;
		y >>= FRACBITS;
		desttop += (y*vid.width) + x;
	}
	else
	{
		x = FixedMul(x,dupx<<FRACBITS);
		y = FixedMul(y,dupy<<FRACBITS);
		x >>= FRACBITS;
		y >>= FRACBITS;

		// Center it if necessary
		if (!(scrn & V_SCALEPATCHMASK))
		{
			// if it's meant to cover the whole screen, black out the rest
			if (x == 0 && SHORT(patch->width) == BASEVIDWIDTH && y == 0 && SHORT(patch->height) == BASEVIDHEIGHT)
			{
				column = (const column_t *)((const UINT8 *)(patch) + LONG(patch->columnofs[0]));
				source = (const UINT8 *)(column) + 3;
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, (column->topdelta == 0xff ? 31 : source[0]));
			}
			if (vid.width != BASEVIDWIDTH * dupx)
			{
				// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
				// so center this imaginary screen
				if (scrn & V_SNAPTORIGHT)
					x += (vid.width - (BASEVIDWIDTH * dupx));
				else if (!(scrn & V_SNAPTOLEFT))
					x += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
			}
			if (vid.height != BASEVIDHEIGHT * dupy)
			{
				// same thing here
				if ((scrn & (V_SPLITSCREEN|V_SNAPTOBOTTOM)) == (V_SPLITSCREEN|V_SNAPTOBOTTOM))
					y += (vid.height/2 - (BASEVIDHEIGHT/2 * dupy));
				else if (scrn & V_SNAPTOBOTTOM)
					y += (vid.height - (BASEVIDHEIGHT * dupy));
				else if (!(scrn & V_SNAPTOTOP))
					y += (vid.height - (BASEVIDHEIGHT * dupy)) / 2;
			}

			desttop += (y*vid.width) + x;
		}
	}

	if (pscale != FRACUNIT) // scale width properly
	{
		pwidth = SHORT(patch->width)<<FRACBITS;
		pwidth = FixedMul(pwidth, pscale);
		pwidth = FixedMul(pwidth, dupx<<FRACBITS);
		pwidth >>= FRACBITS;
	}
	else
		pwidth = SHORT(patch->width) * dupx;

	deststart = desttop;
	destend = desttop + pwidth;

	for (col = 0; (col>>FRACBITS) < SHORT(patch->width); col += colfrac, ++offx, desttop++)
	{
		INT32 topdelta, prevdelta = -1;
		if (scrn & V_FLIP) // offx is measured from right edge instead of left
		{
			if (x+pwidth-offx < 0) // don't draw off the left of the screen (WRAP PREVENTION)
				break;
			if (x+pwidth-offx >= vid.width) // don't draw off the right of the screen (WRAP PREVENTION)
				continue;
		}
		else
		{
			if (x+offx < 0) // don't draw off the left of the screen (WRAP PREVENTION)
				continue;
			if (x+offx >= vid.width) // don't draw off the right of the screen (WRAP PREVENTION)
				break;
		}
		column = (const column_t *)((const UINT8 *)(patch) + LONG(patch->columnofs[col>>FRACBITS]));

		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (const UINT8 *)(column) + 3;
			dest = desttop;
			if (scrn & V_FLIP)
				dest = deststart + (destend - desttop);
			dest += FixedInt(FixedMul(topdelta<<FRACBITS,vdup))*vid.width;

			for (ofs = 0; dest < deststop && (ofs>>FRACBITS) < column->length; ofs += rowfrac)
			{
				if (dest >= screens[scrn&V_PARAMMASK]) // don't draw off the top of the screen (CRASH PREVENTION)
					*dest = patchdrawfunc(dest, source, ofs);
				dest += vid.width;
			}
			column = (const column_t *)((const UINT8 *)column + column->length + 4);
		}
	}
}

// Draws a patch cropped and scaled to arbitrary size.
void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, INT32 scrn, patch_t *patch, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	fixed_t col, ofs, colfrac, rowfrac, fdup;
	INT32 dupx, dupy;
	const column_t *column;
	UINT8 *desttop, *dest;
	const UINT8 *source, *deststop;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	// Done
	if (rendermode != render_soft && !con_startup)
	{
		HWR_DrawCroppedPatch((GLPatch_t*)patch,x,y,pscale,scrn,sx,sy,w,h);
		return;
	}
#endif

	// only use one dup, to avoid stretching (har har)
	dupx = dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	fdup = FixedMul(dupx<<FRACBITS, pscale);
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, fdup);

	y -= FixedMul(SHORT(patch->topoffset)<<FRACBITS, pscale);
	x -= FixedMul(SHORT(patch->leftoffset)<<FRACBITS, pscale);

	desttop = screens[scrn&V_PARAMMASK];

	if (!desttop)
		return;

	deststop = desttop + vid.rowbytes * vid.height;

	if (scrn & V_NOSCALESTART) {
		x >>= FRACBITS;
		y >>= FRACBITS;
		desttop += (y*vid.width) + x;
	}
	else
	{
		x = FixedMul(x,dupx<<FRACBITS);
		y = FixedMul(y,dupy<<FRACBITS);
		x >>= FRACBITS;
		y >>= FRACBITS;

		// Center it if necessary
		if (!(scrn & V_SCALEPATCHMASK))
		{
			// if it's meant to cover the whole screen, black out the rest
			if (x == 0 && SHORT(patch->width) == BASEVIDWIDTH && y == 0 && SHORT(patch->height) == BASEVIDHEIGHT)
			{
				column = (const column_t *)((const UINT8 *)(patch) + LONG(patch->columnofs[0]));
				source = (const UINT8 *)(column) + 3;
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, (column->topdelta == 0xff ? 31 : source[0]));
			}
			if (vid.width != BASEVIDWIDTH * dupx)
			{
				// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
				// so center this imaginary screen
				if (scrn & V_SNAPTORIGHT)
					x += (vid.width - (BASEVIDWIDTH * dupx));
				else if (!(scrn & V_SNAPTOLEFT))
					x += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
			}
			if (vid.height != BASEVIDHEIGHT * dupy)
			{
				// same thing here
				if (scrn & V_SNAPTOBOTTOM)
					y += (vid.height - (BASEVIDHEIGHT * dupy));
				else if (!(scrn & V_SNAPTOTOP))
					y += (vid.height - (BASEVIDHEIGHT * dupy)) / 2;
			}
		}

		desttop += (y*vid.width) + x;
	}

	for (col = sx<<FRACBITS; (col>>FRACBITS) < SHORT(patch->width) && (col>>FRACBITS) < w; col += colfrac, ++x, desttop++)
	{
		INT32 topdelta, prevdelta = -1;
		if (x < 0) // don't draw off the left of the screen (WRAP PREVENTION)
			continue;
		if (x >= vid.width) // don't draw off the right of the screen (WRAP PREVENTION)
			break;
		column = (const column_t *)((const UINT8 *)(patch) + LONG(patch->columnofs[col>>FRACBITS]));

		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (const UINT8 *)(column) + 3;
			dest = desttop;
			dest += FixedInt(FixedMul(topdelta<<FRACBITS,fdup))*vid.width;

			for (ofs = sy<<FRACBITS; dest < deststop && (ofs>>FRACBITS) < column->length && (ofs>>FRACBITS) < h; ofs += rowfrac)
			{
				if (dest >= screens[scrn&V_PARAMMASK]) // don't draw off the top of the screen (CRASH PREVENTION)
					*dest = source[ofs>>FRACBITS];
				dest += vid.width;
			}
			column = (const column_t *)((const UINT8 *)column + column->length + 4);
		}
	}
}

//
// V_DrawContinueIcon
// Draw a mini player!  If we can, that is.  Otherwise we draw a star.
//
void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT16 skincolor)
{
	if (skinnum < 0 || skinnum >= numskins || (skins[skinnum].flags & SF_HIRES))
		V_DrawScaledPatch(x - 10, y - 14, flags, W_CachePatchName("CONTINS", PU_CACHE)); // Draw a star
	else
	{ // Find front angle of the first waiting frame of the character's actual sprites
		spriteframe_t *sprframe = &skins[skinnum].spritedef.spriteframes[2 & FF_FRAMEMASK];
		patch_t *patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
		const UINT8 *colormap = R_GetTranslationColormap(skinnum, skincolor, GTC_CACHE);

		// No variant for translucency
		V_DrawTinyMappedPatch(x, y, flags, patch, colormap);
	}
}

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src)
{
	UINT8 *dest;
	const UINT8 *deststop;

#ifdef RANGECHECK
	if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned)scrn > 4)
		I_Error("Bad V_DrawBlock");
#endif

	dest = screens[scrn] + y*vid.width + x;
	deststop = screens[scrn] + vid.rowbytes * vid.height;

	while (height--)
	{
		M_Memcpy(dest, src, width);

		src += width;
		dest += vid.width;
		if (dest > deststop)
			return;
	}
}

//
// Fills a box of pixels with a single color, NOTE: scaled to screen size
//
void V_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT8 *dest;
	const UINT8 *deststop;
	UINT32 alphalevel = ((c & V_ALPHAMASK) >> V_ALPHASHIFT);

	if (rendermode == render_none)
		return;

	
	v_translevel = NULL;
	if (alphalevel)
	{
		if (alphalevel == 13)
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 14)
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 15)
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return; // invis

		v_translevel = transtables + ((alphalevel-1)<<FF_TRANSSHIFT);
	}

#ifdef HWRENDER
	if (rendermode != render_soft && !con_startup)
	{
		HWR_DrawFill(x, y, w, h, c);
		return;
	}
#endif

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{ // Clear the entire screen, from dest to deststop. Yes, this really works.
			memset(screens[0], (c&255), vid.width * vid.height * vid.bpp);
			return;
		}

		x *= dupx;
		y *= dupy;
		w *= dupx;
		h *= dupy;

		// Center it if necessary
		if (vid.width != BASEVIDWIDTH * dupx)
		{
			// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
			// so center this imaginary screen
			if (c & V_SNAPTORIGHT)
				x += (vid.width - (BASEVIDWIDTH * dupx));
			else if (!(c & V_SNAPTOLEFT))
				x += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
		}
		if (vid.height != BASEVIDHEIGHT * dupy)
		{
			// same thing here
			if (c & V_SNAPTOBOTTOM)
				y += (vid.height - (BASEVIDHEIGHT * dupy));
			else if (!(c & V_SNAPTOTOP))
				y += (vid.height - (BASEVIDHEIGHT * dupy)) / 2;
		}
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;


	// borrowing this from jimitia's new hud drawing functions rq
	if (alphalevel)
	{
		v_translevel += c<<8;
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			for (x = 0; x < w; x++)
				dest[x] = v_translevel[dest[x]];
		}
	}
	else
	{
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
			memset(dest, c, w * vid.bpp);
	}
}

#ifdef HWRENDER
// This is now a function since it's otherwise repeated 2 times and honestly looks retarded:
static UINT32 V_GetHWConsBackColor(void)
{
	UINT8 r, g, b;
	switch (cons_backcolor.value)
	{
		case 0:		r = 0xff; g = 0xff; b = 0xff;	break; 	// White
		case 1:		r = 0x01; g = 0x01; b = 0x01;	break; 	// Black -- it was gray, not totally black. Or is it? - Michael Stevens 2025
		case 2:		r = 0xde; g = 0xb8; b = 0x87;	break;	// Sepia
		case 3:		r = 0x40; g = 0x20; b = 0x10;	break; 	// Brown
		case 4:		r = 0xfa; g = 0x80; b = 0x72;	break; 	// Pink
		case 5:		r = 0xff; g = 0x69; b = 0xb4;	break; 	// Raspberry
		case 6:		r = 0xff; g = 0x00; b = 0x00;	break; 	// Red
		case 7:		r = 0xff; g = 0xd6; b = 0x83;	break;	// Creamsicle
		case 8:		r = 0xff; g = 0x80; b = 0x00;	break; 	// Orange
		case 9:		r = 0xda; g = 0xa5; b = 0x20;	break; 	// Gold
		case 10:	r = 0x80; g = 0x80; b = 0x00;	break; 	// Yellow
		case 11:	r = 0x00; g = 0xff; b = 0x00;	break; 	// Emerald
		case 12:	r = 0x00; g = 0x80; b = 0x00;	break; 	// Green
		case 13:	r = 0x40; g = 0x80; b = 0xff;	break; 	// Cyan
		case 14:	r = 0x46; g = 0x82; b = 0xb4;	break; 	// Steel
		case 15:	r = 0x1e; g = 0x90; b = 0xff;	break;	// Periwinkle
		case 16:	r = 0x00; g = 0x00; b = 0xff;	break; 	// Blue
		case 17:	r = 0xff; g = 0x00; b = 0xff;	break; 	// Purple
		case 18:	r = 0xee; g = 0x82; b = 0xee;	break; 	// Lavender
		case 19:	r = 0x80; g = 0x80; b = 0x80;	break; 	// Gray
		// Default green
		default:	r = 0x00; g = 0x80; b = 0x00;	break;
	}
	V_CubeApply(&r, &g, &b);
	return (r << 24) | (g << 16) | (b << 8);
}
#endif


// THANK YOU MPC!!!

void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT8 *dest;
	INT32 u, v;
	UINT8 *fadetable;
	UINT32 alphalevel = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleFill(x, y, w, h, hwcolor, c);	// we still use the regular color stuff but only for flags. actual draw color is "hwcolor" for this.
		return;
	}
#endif

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{ // Clear the entire screen, from dest to deststop. Yes, this really works.
			memset(screens[0], (UINT8)(c&255), vid.width * vid.height * vid.bpp);
			return;
		}

		x *= dupx;
		y *= dupy;
		w *= dupx;
		h *= dupy;

		// Center it if necessary
		if (vid.width != BASEVIDWIDTH * dupx)
		{
			// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
			// so center this imaginary screen
			if (c & V_SNAPTORIGHT)
				x += (vid.width - (BASEVIDWIDTH * dupx));
			else if (!(c & V_SNAPTOLEFT))
				x += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
		}
		if (vid.height != BASEVIDHEIGHT * dupy)
		{
			// same thing here
			if (c & V_SNAPTOBOTTOM)
				y += (vid.height - (BASEVIDHEIGHT * dupy));
			else if (!(c & V_SNAPTOTOP))
				y += (vid.height - (BASEVIDHEIGHT * dupy)) / 2;
		}
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
		w = vid.width-x;
	if (y + h > vid.height)
		h = vid.height-y;

	dest = screens[0] + y*vid.width + x;

	if ((alphalevel = ((c & V_ALPHAMASK) >> V_ALPHASHIFT)))
	{
		if (alphalevel == 13)
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 14)
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 15)
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10)
			return; // invis
	}

	c &= 255;

	// Jimita (12-04-2018)
	w = min(w, vid.width);
	h = min(h, vid.height);
	fadetable = ((UINT8 *)transtables + ((alphalevel-1)<<FF_TRANSSHIFT) + (c*256));
	for (v = 0; v < h; v++, dest += vid.width)
		for (u = 0; u < w; u++)
		{
			if (!alphalevel)
				dest[u] = consolebgmap[dest[u]];
			else
				dest[u] = fadetable[consolebgmap[dest[u]]];
		}
}

//
// Fills a box of pixels using a flat texture as a pattern, scaled to screen size.
//
void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum)
{
	INT32 u, v, dupx, dupy;
	fixed_t dx, dy, xfrac, yfrac;
	const UINT8 *src, *deststop;
	UINT8 *flat, *dest;
	size_t size, lflatsize, flatshift;

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		HWR_DrawFlatFill(x, y, w, h, flatnum);
		return;
	}
#endif

	size = W_LumpLength(flatnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			lflatsize = 2048;
			flatshift = 10;
			break;
		case 1048576: // 1024x1024 lump
			lflatsize = 1024;
			flatshift = 9;
			break;
		case 262144:// 512x512 lump
			lflatsize = 512;
			flatshift = 8;
			break;
		case 65536: // 256x256 lump
			lflatsize = 256;
			flatshift = 7;
			break;
		case 16384: // 128x128 lump
			lflatsize = 128;
			flatshift = 7;
			break;
		case 1024: // 32x32 lump
			lflatsize = 32;
			flatshift = 5;
			break;
		default: // 64x64 lump
			lflatsize = 64;
			flatshift = 6;
			break;
	}

	flat = W_CacheLumpNum(flatnum, PU_CACHE);

	dupx = dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	dest = screens[0] + y*dupy*vid.width + x*dupx;
	deststop = screens[0] + vid.rowbytes * vid.height;

	// from V_DrawScaledPatch
	if (vid.width != BASEVIDWIDTH * dupx)
	{
		// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
		// so center this imaginary screen
		dest += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
	}
	if (vid.height != BASEVIDHEIGHT * dupy)
	{
		// same thing here
		dest += (vid.height - (BASEVIDHEIGHT * dupy)) * vid.width / 2;
	}

	w *= dupx;
	h *= dupy;

	dx = FixedDiv(FRACUNIT, dupx<<(FRACBITS-2));
	dy = FixedDiv(FRACUNIT, dupy<<(FRACBITS-2));

	yfrac = 0;
	for (v = 0; v < h; v++, dest += vid.width)
	{
		xfrac = 0;
		src = flat + (((yfrac>>FRACBITS) & (lflatsize - 1)) << flatshift);
		for (u = 0; u < w; u++)
		{
			if (&dest[u] > deststop)
				return;
			dest[u] = src[(xfrac>>FRACBITS)&(lflatsize-1)];
			xfrac += dx;
		}
		yfrac += dy;
	}
}

//
// V_DrawPatchFill
//
void V_DrawPatchFill(patch_t *pat)
{
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	INT32 x, y, pw = SHORT(pat->width) * dupz, ph = SHORT(pat->height) * dupz;

	for (x = 0; x < vid.width; x += pw)
	{
		for (y = 0; y < vid.height; y += ph)
			V_DrawScaledPatch(x, y, V_NOSCALESTART, pat);
	}
}

//
// Fade all the screen buffer, so that the menu is more readable,
// especially now that we use the small hufont in the menus...
// If color is 0x00 to 0xFF, draw transtable (strength range 0-10).
// Else, use COLORMAP lump (strength range 0-31).
// IF YOU ARE NOT CAREFUL, THIS CAN AND WILL CRASH!
// I have kept the safety checks out of this function;
// the v.fadeScreen Lua interface handles those.
void V_DrawFadeScreen(UINT16 color, UINT8 strength)
{
	if (!strength)
		return;

	if (!(color & 0xFF00) && strength == 10)
	{
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, color);
		return;
	}

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		HWR_FadeScreenMenuBack(color, strength);
		return;
	}
#endif

	{
		const UINT8 *fadetable = ((color & 0xFF00) // Color is not palette index?
		? ((UINT8 *)colormaps + strength*256) // Do COLORMAP fade.
		: ((UINT8 *)transtables + ((10-strength)<<FF_TRANSSHIFT) + color*256)); // Else, do TRANSMAP** fade.
		const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;
		UINT8 *buf = screens[0];

		// heavily simplified -- we don't need to know x or y
		// position when we're doing a full screen fade
		for (; buf < deststop; ++buf)
			*buf = fadetable[*buf];
	}
}

// Simple translucency with one color, over a set number of lines starting from the top.
void V_DrawFadeConsBack(INT32 plines)
{
	UINT8 *deststop, *buf;

#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode != render_soft && rendermode != render_none)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleBack(hwcolor, plines);
		return;
	}
#endif

	// heavily simplified -- we don't need to know x or y position,
	// just the stop position
	deststop = screens[0] + vid.rowbytes * min(plines, vid.height);
	for (buf = screens[0]; buf < deststop; ++buf)
		*buf = consolebgmap[*buf];
}

// Gets string colormap, used for 0x80 color codes
//
UINT8 *V_GetStringColormap(INT32 colorflags)
{
	switch ((colorflags & V_CHARCOLORMASK) >> V_CHARCOLORSHIFT)
	{
	case 1: // 0x81, purple
		return purplemap;
	case 2: // 0x82, yellow
		return yellowmap;
	case 3: // 0x83, lgreen
		return lgreenmap;
	case 4: // 0x84, blue
		return bluemap;
	case 5: // 0x85, red
		return redmap;
	case 6: // 0x86, gray
		return graymap;
	case 7: // 0x87, orange
		return orangemap;
	case 8: // 0x88, sky
		return skymap;
	case 9: // 0x89, lavender
		return lavendermap;
	case 10:// 0x8A, gold
		return goldmap;
	case 11:// 0x8B, tea
		return teamap;
	case 12:// 0x8C, steel
		return steelmap;
	case 13:// 0x8D, pink
		return pinkmap;
	case 14:// 0x8E, brown
		return brownmap;
	case 15:// 0x8F, peach
		return peachmap;
	default: // reset
		return NULL;
	}
}

// Writes a single character (draw WHITE if bit 7 set)
//
void V_DrawCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed)
{
	INT32 w, flags;
	const UINT8 *colormap = V_GetStringColormap(c);

	flags = c & ~(V_CHARCOLORMASK | V_PARAMMASK);
	c &= 0x7f;
	if (lowercaseallowed)
		c -= HU_FONTSTART;
	else
		c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		return;

	w = SHORT(hu_font[c]->width);
	if (x + w > vid.width)
		return;

	if (colormap != NULL)
		V_DrawMappedPatch(x, y, flags, hu_font[c], colormap);
	else
		V_DrawScaledPatch(x, y, flags, hu_font[c]);
}

// Writes a single character for the chat. (draw WHITE if bit 7 set)
// Essentially the same as the above but it's small or big depending on what resolution you've chosen to huge..
//
void V_DrawChatCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, UINT8 *colormap)
{
	INT32 w, flags;
	//const UINT8 *colormap = V_GetStringColormap(c);

	flags = c & ~(V_CHARCOLORMASK | V_PARAMMASK);
	c &= 0x7f;
	if (lowercaseallowed)
		c -= HU_FONTSTART;
	else
		c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		return;

	w = (vid.width < 640 ) ? (SHORT(hu_font[c]->width)/2) : (SHORT(hu_font[c]->width));	// use normal sized characters if we're using a terribly low resolution.
	if (x + w > vid.width)
		return;

	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, (vid.width < 640) ? (FRACUNIT) : (FRACUNIT/2), flags, hu_font[c], colormap);


}

// Precompile a wordwrapped string to any given width.
// This is a muuuch better method than V_WORDWRAP.
char *V_WordWrap(INT32 x, INT32 w, INT32 option, const char *string)
{
	int c;
	size_t chw, i, lastusablespace = 0;
	size_t slen;
	char *newstring = Z_StrDup(string);
	INT32 spacewidth = 4, charwidth = 0;

	slen = strlen(string);

	if (w == 0)
		w = BASEVIDWIDTH;
	w -= x;
	x = 0;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (i = 0; i < slen; ++i)
	{
		c = newstring[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x89) //color parsing! -Inuyasha 2.16.09
			continue;

		if (c == '\n')
		{
			x = 0;
			lastusablespace = 0;
			continue;
		}

		if (!(option & V_ALLOWLOWERCASE))
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			chw = spacewidth;
			lastusablespace = i;
		}
		else
			chw = (charwidth ? charwidth : hu_font[c]->width);

		x += chw;

		if (lastusablespace != 0 && x > w)
		{
			newstring[lastusablespace] = '\n';
			i = lastusablespace;
			lastusablespace = 0;
			x = 0;
		}
	}
	return newstring;
}

//
// Write a string using the hu_font
// NOTE: the text is centered for screens larger than the base width
//
void V_DrawString(INT32 x, INT32 y, INT32 option, const char *string)
{
	INT32 w, c, cx = x, cy = y, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 4, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += 8*dupy;
			else
				cy += 12*dupy;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			cx += spacewidth * dupx;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - SHORT(hu_font[c]->width)*dupx/2;
		}
		else
			w = SHORT(hu_font[c]->width) * dupx;

		if (cx > scrwidth)
			break;
		if (cx+left + w < 0) //left boundary check
		{
			cx += w;
			continue;
		}

		colormap = V_GetStringColormap(charflags);
		V_DrawFixedPatch((cx + center)<<FRACBITS, cy<<FRACBITS, FRACUNIT, option, hu_font[c], colormap);

		cx += w;
	}
}

void V_DrawCenteredString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option)/2;
	V_DrawString(x, y, option, string);
}

void V_DrawRightAlignedString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option);
	V_DrawString(x, y, option, string);
}

//
// Write a string using the hu_font, 0.5x scale
// NOTE: the text is centered for screens larger than the base width
//
void V_DrawSmallString(INT32 x, INT32 y, INT32 option, const char *string)
{
	INT32 w, c, cx = x, cy = y, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 4;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 4;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += 4*dupy;
			else
				cy += 6*dupy;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			cx += spacewidth * dupx;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - SHORT(hu_font[c]->width)*dupx/4;
		}
		else
			w = SHORT(hu_font[c]->width) * dupx / 2;
		if (cx > scrwidth)
			break;
		if (cx+left + w < 0) //left boundary check
		{
			cx += w;
			continue;
		}

		colormap = V_GetStringColormap(charflags);
		V_DrawFixedPatch((cx + center)<<FRACBITS, cy<<FRACBITS, FRACUNIT/2, option, hu_font[c], colormap);

		cx += w;
	}
}

void V_DrawCenteredSmallString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_SmallStringWidth(string, option)/2;
	V_DrawSmallString(x, y, option, string);
}


void V_DrawRightAlignedSmallString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_SmallStringWidth(string, option);
	V_DrawSmallString(x, y, option, string);
}

//
// Write a string using the tny_font
// NOTE: the text is centered for screens larger than the base width
//
void V_DrawThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	INT32 w, c, cx = x, cy = y, dupx, dupy, scrwidth, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 5;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 5;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += 8*dupy;
			else
				cy += 12*dupy;

			continue;
		}

		c = *ch;
		if (!lowercase || !tny_font[c-HU_FONTSTART])
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !tny_font[c])
		{
			cx += spacewidth * dupx;
			continue;
		}

		if (charwidth)
			w = charwidth * dupx;
		else
			w = (SHORT(tny_font[c]->width) * dupx);

		if (cx > scrwidth)
			break;
		if (cx+left + w < 0) //left boundary check
		{
			cx += w;
			continue;
		}

		colormap = V_GetStringColormap(charflags);
		V_DrawFixedPatch(cx<<FRACBITS, cy<<FRACBITS, FRACUNIT, option, tny_font[c], colormap);

		cx += w;
	}
}

void V_DrawCenteredThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option)/2;
	V_DrawThinString(x, y, option, string);
}

void V_DrawRightAlignedThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option);
	V_DrawThinString(x, y, option, string);
}

//
// Write a string using the tny_font, 0.5x scale
// NOTE: the text is centered for screens larger than the base width
//
// Literally a wrapper. ~Golden
void V_DrawSmallThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x <<= FRACBITS;
	y <<= FRACBITS;
	V_DrawSmallThinStringAtFixed((fixed_t)x, (fixed_t)y, option, string);
}

void V_DrawCenteredSmallThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x <<= FRACBITS;
	y <<= FRACBITS;
	V_DrawCenteredSmallThinStringAtFixed((fixed_t)x, (fixed_t)y, option, string);
}

void V_DrawRightAlignedSmallThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x <<= FRACBITS;
	y <<= FRACBITS;
	V_DrawRightAlignedSmallThinStringAtFixed((fixed_t)x, (fixed_t)y, option, string);
}

// Draws a string at a fixed_t location.
void V_DrawStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 spacewidth = 4, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color ignoring
			continue;
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += (8*dupy)<<FRACBITS;
			else
				cy += (12*dupy)<<FRACBITS;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			cx += (spacewidth * dupx)<<FRACBITS;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - SHORT(hu_font[c]->width)*(dupx/2);
		}
		else
			w = SHORT(hu_font[c]->width) * dupx;

		if ((cx>>FRACBITS) > scrwidth)
			break;
		if ((cx>>FRACBITS)+left + w < 0) //left boundary check
		{
			cx += w<<FRACBITS;
			continue;
		}

		V_DrawSciencePatch(cx + (center<<FRACBITS), cy, option, hu_font[c], FRACUNIT);

		cx += w<<FRACBITS;
	}
}

void V_DrawCenteredStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_StringWidth(string, option) / 2)<<FRACBITS;
	V_DrawStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option)<<FRACBITS;
	V_DrawStringAtFixed(x, y, option, string);
}

// Draws a small string at a fixed_t location.
void V_DrawSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 4;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 4;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += (4*dupy)<<FRACBITS;
			else
				cy += (6*dupy)<<FRACBITS;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			cx += (spacewidth * dupx)<<FRACBITS;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - SHORT(hu_font[c]->width)*(dupx/4);
		}
		else
			w = SHORT(hu_font[c]->width) * dupx / 2;

		if ((cx>>FRACBITS) > scrwidth)
			break;
		if ((cx>>FRACBITS)+left + w < 0) //left boundary check
		{
			cx += w<<FRACBITS;
			continue;
		}

		colormap = V_GetStringColormap(charflags);

		V_DrawFixedPatch(cx + (center<<FRACBITS), cy, FRACUNIT/2, option, hu_font[c], colormap);

		cx += w<<FRACBITS;
	}
}

void V_DrawCenteredSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_SmallStringWidth(string, option) / 2)<<FRACBITS;
	V_DrawSmallStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_SmallStringWidth(string, option)<<FRACBITS;
	V_DrawSmallStringAtFixed(x, y, option, string);
}

// Draws a thin string at a fixed_t location.
void V_DrawThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += (8*dupy)<<FRACBITS;
			else
				cy += (12*dupy)<<FRACBITS;

			continue;
		}

		c = *ch;
		if (!lowercase || !tny_font[c-HU_FONTSTART])
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !tny_font[c])
		{
			cx += (spacewidth * dupx)<<FRACBITS;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - SHORT(tny_font[c]->width)*(dupx/2);
		}
		else
			w = SHORT(tny_font[c]->width) * dupx;

		if ((cx>>FRACBITS) > scrwidth)
			break;
		if ((cx>>FRACBITS)+left + w < 0) //left boundary check
		{
			cx += w<<FRACBITS;
			continue;
		}

		colormap = V_GetStringColormap(charflags);

		V_DrawFixedPatch(cx + (center<<FRACBITS), cy, FRACUNIT, option, tny_font[c], colormap);

		cx += w<<FRACBITS;
	}
}

void V_DrawCenteredThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_ThinStringWidth(string, option) / 2)<<FRACBITS;
	V_DrawThinStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option)<<FRACBITS;
	V_DrawThinStringAtFixed(x, y, option, string);
}

// Draws a small string at a fixed_t location.
void V_DrawSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2<<FRACBITS, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx<<FRACBITS;
		dupy = vid.dupy<<FRACBITS;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = FRACUNIT;
		scrwidth = FixedDiv(vid.width<<FRACBITS, vid.dupx);
		left = ((scrwidth - (BASEVIDWIDTH<<FRACBITS))/2);
		scrwidth -= left;
	}

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 4<<FRACBITS;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 4<<FRACBITS;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3<<FRACBITS;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += 4*dupy;
			else
				cy += 6*dupy;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !tny_font[c])
		{
			cx += FixedMul(spacewidth, dupx);
			continue;
		}

		if (charwidth)
		{
			w = FixedMul(charwidth, dupx);
			center = w/2 - SHORT(tny_font[c]->width)*(dupx/4);
		}
		else
			w = SHORT(tny_font[c]->width) * dupx / 2;

		if (cx > scrwidth)
			break;
		if (cx+left + w < 0) //left boundary check
		{
			cx += w;
			continue;
		}

		colormap = V_GetStringColormap(charflags);

		V_DrawFixedPatch(cx + center, cy, FRACUNIT/2, option, tny_font[c], colormap);

		cx += w;
	}
}

void V_DrawCenteredSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_SmallThinStringWidth(string, option)/4;
	V_DrawSmallThinStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_SmallThinStringWidth(string, option)/2;
	V_DrawSmallThinStringAtFixed(x, y, option, string);
}

// Draws a tallnum.  Replaces two functions in y_inter and st_stuff
void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num)
{
	INT32 w = SHORT(tallnum[0]->width);
	boolean neg;

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	if ((neg = num < 0))
		num = -num;

	// draw the number
	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, tallnum[num % 10]);
		num /= 10;
	} while (num);

	// draw a minus sign if necessary
	if (neg)
		V_DrawScaledPatch(x - w, y, flags, tallminus); // Tails
}

// Draws a number with a set number of digits.
// Does not handle negative numbers in a special way, don't try to feed it any.
void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits)
{
	INT32 w = SHORT(tallnum[0]->width);

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	if (num < 0)
		num = -num;

	// draw the number
	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, tallnum[num % 10]);
		num /= 10;
	} while (--digits);
}

// Write a string using the credit font
// NOTE: the text is centered for screens larger than the base width
//
void V_DrawCreditString(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	INT32 w, c, dupx, dupy, scrwidth = BASEVIDWIDTH;
	fixed_t cx = x, cy = y;
	const char *ch = string;

	// It's possible for string to be a null pointer
	if (!string)
		return;

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
		dupx = dupy = 1;

	for (;;)
	{
		c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = x;
			cy += (12*dupy)<<FRACBITS;
			continue;
		}

		c = toupper(c) - CRED_FONTSTART;
		if (c < 0 || c >= CRED_FONTSIZE)
		{
			cx += (16*dupx)<<FRACBITS;
			continue;
		}

		w = SHORT(cred_font[c]->width) * dupx;
		if ((cx>>FRACBITS) > scrwidth)
			break;

		V_DrawSciencePatch(cx, cy, option, cred_font[c], FRACUNIT);
		cx += w<<FRACBITS;
	}
}

// Find string width from cred_font chars
//
INT32 V_CreditStringWidth(const char *string)
{
	INT32 c, w = 0;
	size_t i;

	// It's possible for string to be a null pointer
	if (!string)
		return 0;

	for (i = 0; i < strlen(string); i++)
	{
		c = toupper(string[i]) - CRED_FONTSTART;
		if (c < 0 || c >= CRED_FONTSIZE)
			w += 16;
		else
			w += SHORT(cred_font[c]->width);
	}

	return w;
}

// Write a string using the level title font
// NOTE: the text is centered for screens larger than the base width
//
void V_DrawLevelTitle(INT32 x, INT32 y, INT32 option, const char *string)
{
	INT32 w, c, cx = x, cy = y, dupx, dupy, scrwidth, left = 0;
	const char *ch = string;

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dupx;
		dupy = vid.dupy;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dupx;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	for (;;)
	{
		c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = x;
			cy += 12*dupy;
			continue;
		}

		c = toupper(c) - LT_FONTSTART;
		if (c < 0 || c >= LT_FONTSIZE || !lt_font[c])
		{
			cx += 16*dupx;
			continue;
		}

		w = SHORT(lt_font[c]->width) * dupx;
		if (cx > scrwidth)
			break;
		if (cx+left + w < 0) //left boundary check

		{
			cx += w;
			continue;
		}

		V_DrawScaledPatch(cx, cy, option, lt_font[c]);
		cx += w;
	}
}

// Find string width from lt_font chars
//
INT32 V_LevelNameWidth(const char *string)
{
	INT32 c, w = 0;
	size_t i;

	for (i = 0; i < strlen(string); i++)
	{
		c = toupper(string[i]) - LT_FONTSTART;
		if (c < 0 || c >= LT_FONTSIZE || !lt_font[c])
			w += 16;
		else
			w += SHORT(lt_font[c]->width);
	}

	return w;
}

// Find max height of the string
//
INT32 V_LevelNameHeight(const char *string)
{
	INT32 c, w = 0;
	size_t i;

	for (i = 0; i < strlen(string); i++)
	{
		c = toupper(string[i]) - LT_FONTSTART;
		if (c < 0 || c >= LT_FONTSIZE || !lt_font[c])
			continue;

		if (SHORT(lt_font[c]->height) > w)
			w = SHORT(lt_font[c]->height);
	}

	return w;
}

//
// Find string width from hu_font chars
//
INT32 V_StringWidth(const char *string, INT32 option)
{
	INT32 c, w = 0;
	INT32 spacewidth = 4, charwidth = 0;
	size_t i;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (i = 0; i < strlen(string); i++)
	{
		c = string[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x89) //color parsing! -Inuyasha 2.16.09
			continue;

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
			w += spacewidth;
		else
			w += (charwidth ? charwidth : SHORT(hu_font[c]->width));
	}

	return w;
}

//
// Find string width from hu_font chars, 0.5x scale
//
INT32 V_SmallStringWidth(const char *string, INT32 option)
{
	INT32 c, w = 0;
	INT32 spacewidth = 2, charwidth = 0;
	size_t i;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 4;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 4;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3;
		default:
			break;
	}

	for (i = 0; i < strlen(string); i++)
	{
		c = string[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x89) //color parsing! -Inuyasha 2.16.09
			continue;

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
			w += spacewidth;
		else
			w += (charwidth ? charwidth : SHORT(hu_font[c]->width)/2);
	}

	return w;
}

//
// Find string width from tny_font chars
//
INT32 V_ThinStringWidth(const char *string, INT32 option)
{
	INT32 c, w = 0;
	INT32 spacewidth = 2, charwidth = 0;
	size_t i;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 5;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 5;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 3;
		default:
			break;
	}

	for (i = 0; i < strlen(string); i++)
	{
		c = string[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x89) //color parsing! -Inuyasha 2.16.09
			continue;

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE || !tny_font[c])
			w += spacewidth;
		else
			w += (charwidth ? charwidth : SHORT(tny_font[c]->width));
	}

	return w;
}

//
// Find string width from tny_font chars, 0.5x scale
//
INT32 V_SmallThinStringWidth(const char *string, INT32 option)
{
	INT32 w = V_ThinStringWidth(string, option)<<FRACBITS;
	return w/2 + FRACUNIT; // +FRACUNIT because otherwise it's offset wrong.
}

boolean *heatshifter = NULL;
INT32 lastheight = 0;
INT32 heatindex[2] = { 0, 0 };

//
// V_DoPostProcessor
//
// Perform a particular image postprocessing function.
//
#include "p_local.h"
void V_DoPostProcessor(INT32 view, postimg_t type, INT32 param)
{
#if NUMSCREENS < 5
	// do not enable image post processing for ARM, SH and MIPS CPUs
	(void)view;
	(void)type;
	(void)param;
#else
	INT32 height, yoffset;

#ifdef HWRENDER
	// draw a hardware converted patch
	if (rendermode != render_soft && rendermode != render_none)
		return;
#endif

	if (view < 0 || view >= 2 || (view == 1 && !splitscreen))
		return;

	if (splitscreen)
		height = vid.height/2;
	else
		height = vid.height;

	if (view == 1)
		yoffset = vid.height/2;
	else
		yoffset = 0;

	if (type == postimg_water)
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;
		// Set disStart to a range from 0 to FINEANGLE, incrementing by 128 per tic
		angle_t disStart = (((leveltime-1)*128) + (rendertimefrac / (FRACUNIT/128))) & FINEMASK;
		INT32 newpix;
		INT32 sine;
		//UINT8 *transme = transtables + ((tr_trans50-1)<<FF_TRANSSHIFT);

		for (y = yoffset; y < yoffset+height; y++)
		{
			sine = (FINESINE(disStart)*5)>>FRACBITS;
			newpix = abs(sine);

			if (sine < 0)
			{
				M_Memcpy(&tmpscr[y*vid.width+newpix], &srcscr[y*vid.width], vid.width-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[y*vid.width+newpix] = srcscr[y*vid.width];
					newpix--;
				}
			}
			else
			{
				M_Memcpy(&tmpscr[y*vid.width+0], &srcscr[y*vid.width+sine], vid.width-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[y*vid.width+vid.width-newpix] = srcscr[y*vid.width+(vid.width-1)];
					newpix--;
				}
			}

/*
Unoptimized version
			for (x = 0; x < vid.width*vid.bpp; x++)
			{
				newpix = (x + sine);

				if (newpix < 0)
					newpix = 0;
				else if (newpix >= vid.width)
					newpix = vid.width-1;

				tmpscr[y*vid.width + x] = srcscr[y*vid.width+newpix]; // *(transme + (srcscr[y*vid.width+x]<<8) + srcscr[y*vid.width+newpix]);
			}*/
			disStart += 22;//the offset into the displacement map, increment each game loop
			disStart &= FINEMASK; //clip it to FINEMASK
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_motion) // Motion Blur!
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 x, y;

		// TODO: Add a postimg_param so that we can pick the translucency level...
		UINT8 *transme = transtables + ((param-1)<<FF_TRANSSHIFT);

		for (y = yoffset; y < yoffset+height; y++)
		{
			for (x = 0; x < vid.width; x++)
			{
				tmpscr[y*vid.width + x]
					=     colormaps[*(transme     + (srcscr   [y*vid.width+x ] <<8) + (tmpscr[y*vid.width+x]))];
			}
		}
		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_flip) // Flip the screen upside-down
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y, y2;

		for (y = yoffset, y2 = yoffset+height - 1; y < yoffset+height; y++, y2--)
			M_Memcpy(&tmpscr[y2*vid.width], &srcscr[y*vid.width], vid.width);

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_heat) // Heat wave
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;

		// Make sure table is built
		if (heatshifter == NULL || lastheight != height)
		{
			if (heatshifter)
				Z_Free(heatshifter);

			heatshifter = Z_Calloc(height * sizeof(boolean), PU_STATIC, NULL);

			for (y = 0; y < height; y++)
			{
				if (M_RandomChance(FRACUNIT/8)) // 12.5%
					heatshifter[y] = true;
			}

			heatindex[0] = heatindex[1] = 0;
			lastheight = height;
		}

		for (y = yoffset; y < yoffset+height; y++)
		{
			if (heatshifter[heatindex[view]++])
			{
				// Shift this row of pixels to the right by 2
				tmpscr[y*vid.width] = srcscr[y*vid.width];
				M_Memcpy(&tmpscr[y*vid.width+vid.dupx], &srcscr[y*vid.width], vid.width-vid.dupx);
			}
			else
				M_Memcpy(&tmpscr[y*vid.width], &srcscr[y*vid.width], vid.width);

			heatindex[view] %= height;
		}


		if (renderisnewtic) // This isn't interpolated... but how do you interpolate a one-pixel shift?
		{
			heatindex[view]++;
			heatindex[view] %= vid.height;
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
#endif
}

// Taken from my videos-in-SRB2 project
// Generates a color look-up table
// which has up to 64 colors at each channel
// (see the defines in v_video.h)

UINT8 colorlookup[CLUTSIZE][CLUTSIZE][CLUTSIZE];

void InitColorLUT(RGBA_t *palette)
{
	UINT8 r, g, b;
	static boolean clutinit = false;
	static RGBA_t *lastpalette = NULL;
	if ((!clutinit) || (lastpalette != palette))
	{
		for (r = 0; r < CLUTSIZE; r++)
			for (g = 0; g < CLUTSIZE; g++)
				for (b = 0; b < CLUTSIZE; b++)
					colorlookup[r][g][b] = NearestPaletteColor(r << SHIFTCOLORBITS, g << SHIFTCOLORBITS, b << SHIFTCOLORBITS, palette);
		clutinit = true;
		lastpalette = palette;
	}
}

// V_Init
// old software stuff, buffers are allocated at video mode setup
// here we set the screens[x] pointers accordingly
// WARNING: called at runtime (don't init cvar here)
void V_Init(void)
{
	INT32 i;
	UINT8 *base = vid.buffer;
	const INT32 screensize = vid.rowbytes * vid.height;

	LoadMapPalette();
	for (i = 0; i < NUMSCREENS; i++)
		screens[i] = NULL;

	// start address of NUMSCREENS * width*height vidbuffers
	if (base)
	{
		for (i = 0; i < NUMSCREENS; i++)
			screens[i] = base + i*screensize;
	}

	if (vid.direct)
		screens[0] = vid.direct;

#ifdef DEBUG
	CONS_Debug(DBG_RENDER, "V_Init done:\n");
	for (i = 0; i < NUMSCREENS+1; i++)
		CONS_Debug(DBG_RENDER, " screens[%d] = %x\n", i, screens[i]);
#endif
}
