// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_main.h
/// \brief 3D render mode functions

#ifndef __HWR_MAIN_H__
#define __HWR_MAIN_H__

#include "hw_data.h"
#include "hw_defs.h"

#include "../am_map.h"
#include "../d_player.h"
#include "../r_defs.h"
#include "../m_perfstats.h"

// Startup & Shutdown the hardware mode renderer
void HWR_Startup(void);
void HWR_Switch(void);
void HWR_Shutdown(void);

void HWR_drawAMline(const fline_t *fl, INT32 color);
void HWR_FadeScreenMenuBack(UINT16 color, UINT8 strength);
void HWR_DrawConsoleBack(UINT32 color, INT32 height);
void HWR_RenderSkyboxView(INT32 viewnumber, player_t *player);
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player);
void HWR_ClearSkyDome(void);
void HWR_BuildSkyDome(void);
void HWR_DrawViewBorder(INT32 clearlines);
void HWR_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum);
void HWR_InitTextureMapping(void);
void HWR_SetViewSize(void);
void HWR_DrawPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option);
void HWR_DrawStretchyFixedPatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap);
void HWR_DrawCroppedPatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, fixed_t scale, INT32 option, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);
void HWR_DrawCroppedPatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, INT32 option, fixed_t scale, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);
void HWR_CreatePlanePolygons(INT32 bspnum);
void HWR_CreateStaticLightmaps(INT32 bspnum);
void HWR_LoadTextures(size_t pnumtextures);
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color);
void HWR_DrawConsoleFill(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 color, INT32 options);	// Lat: separate flags from color since color needs to be an uint to work right.

UINT8 *HWR_GetScreenshot(void);
boolean HWR_Screenshot(const char *pathname);

void HWR_AddCommands(void);
void HWR_AddSessionCommands(void);
void transform(float *cx, float *cy, float *cz);
FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf);
INT32 HWR_GetTextureUsed(void);
void HWR_DoPostProcessor(player_t *player);
void HWR_StartScreenWipe(void);
void HWR_EndScreenWipe(void);
void HWR_DrawIntermissionBG(void);
void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum);
void HWR_MakeScreenFinalTexture(void);
void HWR_DrawScreenFinalTexture(int width, int height);

// This stuff is put here so MD2's can use them
boolean HWR_UseShader(void);
void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap);
UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap); // Let's see if this can work


extern CV_PossibleValue_t glanisotropicmode_cons_t[];

boolean HWR_ShouldUsePaletteRendering(void);
extern consvar_t cv_glshaders, cv_glallowshaders;
extern consvar_t cv_glmd2;
extern consvar_t cv_glmodelinterpolation;
extern consvar_t cv_glfiltermode;
extern consvar_t cv_glanisotropicmode;
extern consvar_t cv_glsolvetjoin;
extern consvar_t cv_glshearing;
extern consvar_t cv_glspritebillboarding;
extern consvar_t cv_glmodellighting;
extern consvar_t cv_glloadingscreen;
extern consvar_t cv_glfakecontrast;
extern consvar_t cv_glslopecontrast;
extern consvar_t cv_glbatching;
extern consvar_t cv_glwireframe;
extern consvar_t cv_glpaletterendering;
extern consvar_t cv_glpalettedepth;
extern consvar_t cv_glcurveshader; 
extern consvar_t cv_gllightdither;

extern float gl_viewwidth, gl_viewheight, gl_baseviewwindowy;

extern float gl_viewwindowx, gl_basewindowcentery;

// BP: big hack for a test in lighting ref : 1249753487AB
extern fixed_t *hwbbox;
extern FTransform atransform;

// Render stats
extern ps_metric_t ps_hw_nodesorttime;
extern ps_metric_t ps_hw_nodedrawtime;
extern ps_metric_t ps_hw_spritesorttime;
extern ps_metric_t ps_hw_spritedrawtime;

// Render stats for batching
extern ps_metric_t ps_hw_numpolys;
extern ps_metric_t ps_hw_numverts;
extern ps_metric_t ps_hw_numcalls;
extern ps_metric_t ps_hw_numshaders;
extern ps_metric_t ps_hw_numtextures;
extern ps_metric_t ps_hw_numpolyflags;
extern ps_metric_t ps_hw_numcolors;
extern ps_metric_t ps_hw_batchsorttime;
extern ps_metric_t ps_hw_batchdrawtime;

//bye bye floorinfo

extern boolean gl_shadersavailable;

#endif
